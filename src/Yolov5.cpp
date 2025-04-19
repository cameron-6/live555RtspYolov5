/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-09 21:59:36
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-11 16:02:34
 * @FilePath: /v4l2_mplane/src/Yolov5.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "Yolov5.h"
#include "log.h"
#include <fstream>

static int g_frame_end_id = 0;
static int g_frame_start_id = 0;

Yolov5::Yolov5(V4L2DeviceParameters param,std::string model_file)
{
    isRunning_ = true;

    //初始化图像格式转换的参数
    src_buf_size_ = param.width * param.height * get_bpp_from_format(RK_FORMAT_UYVY_422);
    dst_buf_size_ = param.width * param.height * get_bpp_from_format(RK_FORMAT_BGR_888);
   // prepareBuffers(src_buf_size_,dst_buf_size_);
    buffer_.src_buf = (char*)malloc(src_buf_size_);
    buffer_.dst_buf = (char*)malloc(dst_buf_size_);

    

    // //初始化图像resize参数
    // if (!initializeRGAResizeContext(rgaRsizeCtx_, 640, 640, RK_FORMAT_BGR_888, 640, 480, RK_FORMAT_BGR_888)) {
    //     std::cerr << "Failed to initialize RGA context!" << std::endl;
    //     return ;
    // }

    img_height_ = param.height;
    img_width_  = param.width;

    yoloThread_ = new YoloThread(model_file);
    initCameraAndEncoder(param);
}

Yolov5::~Yolov5()
{
    if(inframe)
    {
        free(inframe);
        inframe = NULL;
    }

    if(encodeData)
    {
        free(encodeData);
        encodeData = NULL;
    }

    releaseBuffers(buffer_);
    releaseRGAResizeContext(rgaRsizeCtx_);
}

void Yolov5::start()
{
    LOG(NOTICE, "Yolov5::start");
    std::thread t1(Yolov5::doCapture,this);
    std::thread t2(Yolov5::doPublish,this);
    this->t1 = std::move(t1);
    this->t2 = std::move(t2);
}

void Yolov5::stop()
{
  
    {
        std::lock_guard<std::mutex> lock(mtx_);
        isRunning_ = false;
        
    }
    t1.join();
    t2.join();
}

void Yolov5::join()
{
    t1.join();
    t2.join();
}
void Yolov5::detach()
{
    t1.detach();
    t2.detach();
}

bool Yolov5::initCameraAndEncoder(V4L2DeviceParameters param)
{
    MppFrameFormat mpp_fmt;
    mpp_fmt = MPP_FMT_YUV420P; //Mat数据格式

    videoCapture_ = new VideoCapture(param);
    LOG(NOTICE,"CamRTSPServer videoCapture create");

    //videoCapture_->start();

   // encodeData = (uint8_t *)malloc(videoCapture_->getBufferSize());
    //inframe = (uint8_t *)malloc(videoCapture_->getBufferSize());
    Encoder_Param_t encoder_param{
        mpp_fmt,
        480,
        640,
        0,
        0,
        0,
        30,  //fps
        23};

    rkEncoder_ = new RkEncoder(encoder_param);

    rkEncoder_->init();
    encodeData = (uint8_t *)malloc(videoCapture_->getBufferSize());
    inframe = (uint8_t *)malloc(videoCapture_->getBufferSize());

    return 0;
}

void Yolov5::doCapture(void* ptr)
{
    ((Yolov5*)ptr)->Capture();
}

// 将 YUV422p 转换为 Mat（BGR）
static cv::Mat yuv422pToMat(uint8_t* yuv_data, int width, int height) {
    // 创建 YUV422p 的三个平面
    cv::Mat y_plane(height, width, CV_8UC1, yuv_data);  // Y 平面
    cv::Mat u_plane(height, width / 2, CV_8UC1, yuv_data + width * height);  // U 平面
    cv::Mat v_plane(height, width / 2, CV_8UC1, yuv_data + width * height + (width / 2) * height);  // V 平面

    // 将 YUV422p 合并成一个 YUV 图像
    cv::Mat uv_plane(height, width / 2 * 2, CV_8UC1);
    for (int i = 0; i < height; i++) {
        uint8_t* uv_ptr = uv_plane.ptr<uint8_t>(i);
        const uint8_t* u_ptr = u_plane.ptr<uint8_t>(i);
        const uint8_t* v_ptr = v_plane.ptr<uint8_t>(i);
        for (int j = 0; j < width / 2; j++) {
            uv_ptr[j * 2] = u_ptr[j];
            uv_ptr[j * 2 + 1] = v_ptr[j];
        }
    }

    // 合并 Y 和 UV 平面
    cv::Mat yuv;
    cv::vconcat(y_plane, uv_plane, yuv);

    // 转换为 BGR 格式
    cv::Mat bgr;
    cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_Y422);

    return bgr;
}


static cv::Mat uyvyToBGR(uint8_t* uyvy_data, int width, int height) {
    // 创建一个 OpenCV Mat，表示源的 UYVY 图像
    cv::Mat uyvy_mat(height, width, CV_8UC2, uyvy_data);

    // 创建一个空的 Mat 用于存储 BGR 图像
    cv::Mat bgr_mat;

    // 使用 cvtColor 将 UYVY 转换为 BGR 格式
    cv::cvtColor(uyvy_mat, bgr_mat, cv::COLOR_YUV2BGR_UYVY);

    return bgr_mat;
}

static cv::Mat rotateImage(cv::Mat& src_img, int degree) {
    if (degree == 90) {
        cv::Mat srcCopy = cv::Mat(src_img.rows, src_img.cols, src_img.depth());
        cv::transpose(src_img, srcCopy);
        cv::flip(srcCopy, srcCopy, 1);
        return srcCopy;
    } else if (degree == 180) {
        cv::Mat srcCopy = cv::Mat(src_img.rows, src_img.cols, src_img.depth());
        cv::flip(src_img, srcCopy, -1);
        return srcCopy;
    } else if (degree == 270) {
        cv::Mat srcCopy = cv::Mat(src_img.rows, src_img.cols, src_img.depth());
        cv::transpose(src_img, srcCopy);
        cv::flip(srcCopy, srcCopy, 0);
        return srcCopy;
    } else {
        return src_img;
    }
}


// 使用 RGA 将 NV16 转换为 BGR 格式
static cv::Mat uyvyToBGRWithRGA(uint8_t* nv16_data, int width, int height) {

    int ret = 0;
    int src_width, src_height, src_format;
    int dst_width, dst_height, dst_format;
    char *src_buf, *dst_buf;
    int src_buf_size, dst_buf_size;

    rga_buffer_t src_img, dst_img;
    rga_buffer_handle_t src_handle, dst_handle;

    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));

    src_width = width;
    src_height = height;
    src_format = RK_FORMAT_UYVY_422;

    dst_width = width;
    dst_height = height;
    dst_format = RK_FORMAT_BGR_888;

    src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
    dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

    src_buf = (char *)malloc(src_buf_size);
    dst_buf = (char *)malloc(dst_buf_size);

    //LOG(NOTICE,"memcpy nv16_data");
    memcpy(src_buf,nv16_data,src_buf_size);
    //src_buf = (char*)nv16_data;
    //LOG(NOTICE,"memcpy nv16_data end");
    memset(dst_buf, 0x80, dst_buf_size);

    src_handle = importbuffer_virtualaddr(src_buf, src_buf_size);
    dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
    if (src_handle == 0 || dst_handle == 0) {
        printf("importbuffer failed!\n");
    }

    src_img = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
   // LOG(NOTICE,"wrapbuffer_handle end");
    ret = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
    }

    ret = imcvtcolor(src_img, dst_img, src_format, dst_format);
    if (ret == IM_STATUS_SUCCESS) {
        //printf("%s running success!\n");
    } else {
        printf("%s running failed, %s\n", imStrError((IM_STATUS)ret));
    }

    // 将输出数据加载到 OpenCV 的 Mat
    cv::Mat bgr(height, width, CV_8UC3, dst_buf);
    cv::Mat bgr_clone = bgr.clone();
   // LOG(NOTICE,"bgr end");
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (src_buf)
        free(src_buf);
    if (dst_buf)
        free(dst_buf);

    return bgr_clone;
}


void Yolov5::Capture()
{
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    videoCapture_->start();
    static bool flag = true;
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if(!isRunning_)
            break;
        }
        if(videoCapture_->isReadable(&tv) )
        {
            //inframe = (uint8_t *)malloc(videoCapture_->getBufferSize());
            int frameSize = videoCapture_->get_frame((char*)inframe,videoCapture_->getBufferSize());
            //LOG(NOTICE,"frameSize = %d",frameSize);
            //appendH264FrameToFile("output.h264", encodeData, frameSize);
            if (frameSize)
            {
                //cv::Mat bgr = uyvy2BGRWithRGA(inframe,img_width_,img_height_,buffer_);
               // cv::Mat bgr = uyvyToBGRWithRGA(inframe,img_width_,img_height_);
                cv::Mat bgr = uyvy2BGRWithRGA(inframe,img_width_,img_height_);
                //LOG(NOTICE,"Capture success");
                //cv::imwrite("output.jpg", bgr);
                bgr = rotateImage(bgr,270);
                auto bgrPtr = std::make_shared<cv::Mat>(bgr.clone());
                yoloThread_->submitTask(bgrPtr,g_frame_start_id++);
               // LOG(NOTICE,"submitTask success");
            }

            // free(inframe);
            // inframe = NULL;
        }
    }
    
}

void Yolov5::doPublish(void* ptr)
{
    ((Yolov5*)ptr)->publish();
}

// 假设 h264 数据流为 char* 类型（或 uint8_t*），并且有指定大小 size
static // 假设 h264 数据流为 uint8_t* 类型，帧数据为一帧一帧接收的
void appendH264FrameToFile(const char* filename, const uint8_t* h264Frame, size_t frameSize)
{
    // 以二进制模式打开文件，文件以追加模式打开
    std::ofstream outFile(filename, std::ios::binary | std::ios::app);
    
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // 将每一帧 H.264 数据写入文件
    outFile.write(reinterpret_cast<const char*>(h264Frame), frameSize);
    
    if (!outFile) {
        std::cerr << "Error writing to file: " << filename << std::endl;
    } else {
        std::cout << "H.264 frame saved to: " << filename << std::endl;
    }

    // 关闭文件
    outFile.close();
}



void Yolov5::publish()
{

    int startCode = 0;
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if(!isRunning_)
            break;
        }
        
        std::shared_ptr<cv::Mat> img_ptr;
        auto ret = yoloThread_->getResultAsImg(img_ptr,g_frame_end_id++);
        if(!ret || !img_ptr || img_ptr->empty())
        {
            LOG(ERROR,"getResultAsImg failed");
            break;
        }
        if (img_ptr->empty()) {
            LOG(ERROR, "Received empty image from getResultAsImg. Skipping frame.");
            continue;
        }
        if (img_ptr->rows <= 0 || img_ptr->cols <= 0) {
            LOG(ERROR, "Invalid image dimensions: rows=%d, cols=%d", img_ptr->rows, img_ptr->cols);
            break;
        }
        Mat frame;
        //LOG(NOTICE,"publish success");
        //LOG(NOTICE,"img_ptr->rows = %d, img_ptr->cols = %d",img_ptr->rows,img_ptr->cols);
        // frame = resizeImageWithRGA(rgaRsizeCtx_,img.clone().data,640,640);
        // LOG(NOTICE,"resizeImageWithRGA success");
        //cv::resize(*img_ptr,frame,cv::Size(640,480));
        cv::Mat yuv_img;
        cv::cvtColor(*img_ptr,yuv_img,cv::COLOR_BGR2YUV_I420);
        // if (yuv_img.empty()) {
        //     LOG(ERROR, "Converted YUV image is empty. Skipping encoding.");
        //     continue;
        // }
       // encodeData = (uint8_t *)malloc(videoCapture_->getBufferSize());

        auto size = rkEncoder_->encode(yuv_img.clone().data,yuv_img.total()*yuv_img.elemSize(),encodeData);
        //auto size = rkEncoder_->encode((*img_ptr).data,(*img_ptr).total()*(*img_ptr).elemSize(),encodeData);
        //auto size = rkEncoder_->encode(frame.data,frame.total()*frame.elemSize(),encodeData);
        if (rkEncoder_->startCode3(encodeData))
            startCode = 3;
        else
            startCode = 4;
        if(onEncoderDataCallback_ && size)
        {
            onEncoderDataCallback_(std::vector<uint8_t>(encodeData+startCode,encodeData+size));
            //appendH264FrameToFile("output.h264", encodeData, size);
        }
        // free(encodeData);
        // encodeData = NULL;
    }
    
}


cv::Mat Yolov5::uyvy2BGRWithRGA(uint8_t* uyvy_data, int width, int height)
{
    int src_format = RK_FORMAT_UYVY_422;
    int dst_format = RK_FORMAT_BGR_888;

    int src_buf_size = width * height * get_bpp_from_format(src_format);
    int dst_buf_size = width * height * get_bpp_from_format(dst_format);

    if (buffer_.src_buf && buffer_.dst_buf) 
    {
        buffer_.src_handle = importbuffer_virtualaddr(buffer_.src_buf, src_buf_size_);
        buffer_.dst_handle = importbuffer_virtualaddr(buffer_.dst_buf, dst_buf_size_);
    }
    if (buffer_.src_handle == 0 || buffer_.dst_handle == 0) 
    {
        std::cerr << "Error: Failed to import buffer!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 拷贝输入数据到源缓冲区
    memcpy(buffer_.src_buf, uyvy_data, src_buf_size);
    memset(buffer_.dst_buf, 0x80, dst_buf_size); // 初始化目标缓冲区

    // 包装 RGA 图像
    rga_buffer_t src_img = wrapbuffer_handle(buffer_.src_handle, width, height, src_format);
    rga_buffer_t dst_img = wrapbuffer_handle(buffer_.dst_handle, width, height, dst_format);

    // 校验输入输出
    int ret = imcheck(src_img, dst_img, {}, {});
    if (ret != IM_STATUS_NOERROR) {
        std::cerr << "RGA imcheck error: " << imStrError((IM_STATUS)ret) << std::endl;
        return cv::Mat();
    }

    // 使用 RGA 进行格式转换
    ret = imcvtcolor(src_img, dst_img, src_format, dst_format);
    if (ret != IM_STATUS_SUCCESS) {
        std::cerr << "RGA imcvtcolor failed: " << imStrError((IM_STATUS)ret) << std::endl;
        return cv::Mat();
    }
    //LOG(NOTICE,"imcvtcolor success");
    // 转换为 OpenCV Mat
    auto frame = cv::Mat(height, width, CV_8UC3, buffer_.dst_buf);
    auto img = frame.clone();
    return img; // 深拷贝
}




bool Yolov5::prepareBuffers(int src_buf_size, int dst_buf_size)
{
    buffer_.src_buf = (char*)malloc(src_buf_size);
    buffer_.dst_buf = (char*)malloc(dst_buf_size);
    if (buffer_.src_buf && buffer_.dst_buf) 
    {
        buffer_.src_handle = importbuffer_virtualaddr(buffer_.src_buf, src_buf_size);
        buffer_.dst_handle = importbuffer_virtualaddr(buffer_.dst_buf, dst_buf_size);
    }
    if (buffer_.src_handle == 0 || buffer_.dst_handle == 0) 
    {
        std::cerr << "Error: Failed to import buffer!" << std::endl;
        exit(EXIT_FAILURE);
    }

    return true;
}

// 初始化 RGAResizeContext
bool Yolov5::initializeRGAResizeContext(RGAResizeContext& ctx, int src_width, int src_height, int src_format,
                                 int dst_width, int dst_height, int dst_format) {
    int src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
    int dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

    // 分配缓冲区
    ctx.src_buf = (uint8_t*)malloc(src_buf_size);
    ctx.dst_buf = (uint8_t*)malloc(dst_buf_size);
    if (!ctx.src_buf || !ctx.dst_buf) {
        std::cerr << "Failed to allocate buffers!" << std::endl;
        return false;
    }

    // 初始化缓冲区内容
    memset(ctx.src_buf, 0, src_buf_size);
    memset(ctx.dst_buf, 0, dst_buf_size);

    // 创建 RGA 缓冲区句柄
    ctx.src_handle = importbuffer_virtualaddr(ctx.src_buf, src_buf_size);
    ctx.dst_handle = importbuffer_virtualaddr(ctx.dst_buf, dst_buf_size);
    if (ctx.src_handle == 0 || ctx.dst_handle == 0) {
        std::cerr << "Failed to create buffer handles!" << std::endl;
        return false;
    }

    // 包装为 RGA 图像
    ctx.src_img = wrapbuffer_handle(ctx.src_handle, src_width, src_height, src_format);
    ctx.dst_img = wrapbuffer_handle(ctx.dst_handle, dst_width, dst_height, dst_format);

    return true;
}



cv::Mat Yolov5::resizeImageWithRGA(RGAResizeContext& ctx, uint8_t* src_data, int src_width, int src_height)
{
    if (!ctx.src_buf || !ctx.dst_buf) {
        std::cerr << "Invalid RGA context!" << std::endl;
        return {};
    }

    // 拷贝输入数据到源缓冲区
    int src_buf_size = src_width * src_height * get_bpp_from_format(ctx.src_img.format);
    memcpy(ctx.src_buf, src_data, src_buf_size);

    // 执行 RGA 缩放
    int ret = imresize(ctx.src_img, ctx.dst_img);
    if (ret != IM_STATUS_SUCCESS) {
        std::cerr << "RGA resize failed: " << imStrError((IM_STATUS)ret) << std::endl;
        return {};
    }

    return cv::Mat(ctx.dst_img.height, ctx.dst_img.width, CV_8UC3, ctx.dst_buf).clone();
}

void Yolov5::releaseRGAResizeContext(RGAResizeContext& ctx)
{
    if (ctx.src_handle) releasebuffer_handle(ctx.src_handle);
    if (ctx.dst_handle) releasebuffer_handle(ctx.dst_handle);
    if (ctx.src_buf) free(ctx.src_buf);
    if (ctx.dst_buf) free(ctx.dst_buf);
    ctx.src_buf = nullptr;
    ctx.dst_buf = nullptr;
    ctx.src_handle = 0;
    ctx.dst_handle = 0;
}


void Yolov5::releaseBuffers(Buffer& buffer)
{
    if (buffer.src_handle) 
        releasebuffer_handle(buffer.src_handle);
    if (buffer.dst_handle) 
        releasebuffer_handle(buffer.dst_handle);
    free(buffer.src_buf);
    free(buffer.dst_buf);
}



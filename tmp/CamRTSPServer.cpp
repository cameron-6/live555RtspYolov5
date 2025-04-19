#include "CamRTSPServer.h"
#include "log.h"
#include "mpp_frame.h"
#include <opencv2/opencv.hpp>
#include "H264DemandSubsession.h"
#include <fstream>
#include <rga/im2d.h>
#include <rga/rga.h>


int CamRTSPServer::g_frame_start_id = 0;
int CamRTSPServer::g_frame_end_id = 0;

CamRTSPServer::CamRTSPServer(V4L2DeviceParameters param,std::string model_file) :
    watcher_(0),
    scheduler_(nullptr),
    env_(nullptr)
{

    yoloThread_ = new YoloThread(model_file);

    initCameraAndEncoder(param);

    scheduler_ = BasicTaskScheduler::createNew();
    env_ = BasicUsageEnvironment::createNew(*scheduler_);
}

CamRTSPServer::~CamRTSPServer()
{
    Medium::close(server_);

    if (scheduler_)
        delete scheduler_;
    
    watcher_ = 0;

    LOG(NOTICE, "RTSP server has been destructed!");
}


bool CamRTSPServer::initCameraAndEncoder(V4L2DeviceParameters param)
{
    MppFrameFormat mpp_fmt;
    mpp_fmt = MPP_FMT_YUV420P; //Mat数据格式

    videoCapture_ = new VideoCapture(param);
    LOG(NOTICE,"CamRTSPServer videoCapture create");

    videoCapture_->start();

    encodeData = (uint8_t *)malloc(videoCapture_->getBufferSize());
    inframe = (uint8_t *)malloc(videoCapture_->getBufferSize());
    Encoder_Param_t encoder_param{
        mpp_fmt,
        640,
        480,
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

void CamRTSPServer::doCapture(void* ptr)
{
    ((CamRTSPServer*)ptr)->Capture();
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

    LOG(NOTICE,"memcpy nv16_data");
    memcpy(src_buf,nv16_data,src_buf_size);
    //src_buf = (char*)nv16_data;
    LOG(NOTICE,"memcpy nv16_data end");
    memset(dst_buf, 0x80, dst_buf_size);

    src_handle = importbuffer_virtualaddr(src_buf, src_buf_size);
    dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);
    if (src_handle == 0 || dst_handle == 0) {
        printf("importbuffer failed!\n");
    }

    src_img = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
    dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
    LOG(NOTICE,"wrapbuffer_handle end");
    ret = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
    }

    ret = imcvtcolor(src_img, dst_img, src_format, dst_format);
    if (ret == IM_STATUS_SUCCESS) {
        printf("%s running success!\n");
    } else {
        printf("%s running failed, %s\n", imStrError((IM_STATUS)ret));
    }

    // 将输出数据加载到 OpenCV 的 Mat
    cv::Mat bgr(height, width, CV_8UC3, dst_buf);
    cv::Mat bgr_clone = bgr.clone();
    LOG(NOTICE,"bgr end");
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


void CamRTSPServer::Capture()
{
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    //videoCapture_->start();
    static bool flag = true;
    while (isRunning_)
    {
        if(videoCapture_->isReadable(&tv))
        {
            int frameSize = videoCapture_->get_frame((char*)inframe,videoCapture_->getBufferSize());
           // LOG(NOTICE,"frameSize = %d",frameSize);
            //appendH264FrameToFile("output.h264", encodeData, frameSize);
            if (frameSize)
            {
                cv::Mat bgr = uyvyToBGRWithRGA((uint8_t *)inframe,640,480);
                
                //cv::imwrite("output.jpg", bgr);
                flag = false;
                LOG(NOTICE,"uyvyToBGRWithRGA end");
                bgr = rotateImage(bgr,270);
                LOG(NOTICE,"rotateImage end");
                yoloThread_->submitTask(bgr.clone(),g_frame_start_id++);
                LOG(NOTICE,"submitTask end");
            }
            
        }
    }
    
}

void CamRTSPServer::doPublish(void* ptr)
{
    ((CamRTSPServer*)ptr)->publish();
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

void CamRTSPServer::publish()
{

    while (isRunning_)
    {
        
        cv::Mat img;
        auto ret = yoloThread_->getResultAsImg(img,g_frame_end_id++);
        if(!ret)
        {
            LOG(ERROR,"getResultAsImg failed");
            break;
        }
        Mat frame;
        cv::resize(img.clone(),frame,cv::Size(640,480));
        cv::Mat yuv_img;
        cv::cvtColor(frame,yuv_img,cv::COLOR_BGR2YUV_I420);
        auto size = rkEncoder_->encode(yuv_img.data,yuv_img.total()*yuv_img.elemSize(),encodeData);

        if(onEncoderDataCallback_ && size)
        {
            onEncoderDataCallback_(std::vector<uint8_t>(encodeData,encodeData+size));
            //appendH264FrameToFile("output.h264", encodeData, size);
        }
    }
    
}

void CamRTSPServer::stopServer()
{
    LOG(NOTICE, "Stop server is involed!");
    isRunning_ = false;
    videoCapture_->stop();
    yoloThread_->stopAll();
    watcher_ = 's';
}

void CamRTSPServer::run()
{
    // create server listening on the specified RTSP port
    server_ = RTSPServer::createNew(*env_,8554);
    if (server_ == NULL)
    {
        *env_ << "Failed to create RTSP server: "<<env_->getResultMsg()<<"\n";
        exit(1);
    }

    //auto replicator = StreamReplicator::createNew(*env_, framedSource, False);

    char const *descriptionString = "Session streamed by \"H264OnDemandRTSPServer\"";
    char const *streamName = "live";
    ServerMediaSession *sms = ServerMediaSession::createNew(*env_,streamName,streamName,descriptionString);

    sms->addSubsession(H264DemandSubsession::createNew(*env_,(FramedSource*)CamFrameSource::createNew(*env_,this),this));
    server_->addServerMediaSession(sms);
    announceStream(sms,"/dev/video0");

    isRunning_ = true;
    std::thread cap(CamRTSPServer::doCapture,this);
    std::thread pusher(CamRTSPServer::doPublish,this);


    env_->taskScheduler().doEventLoop(&watcher_); // do not return;
    cap.join();
    pusher.join();
}

void CamRTSPServer::announceStream(ServerMediaSession *sms, const std::string &deviceName) 
{
    auto url = server_->rtspURL(sms);
    LOG(NOTICE, "Play the stream of the %s, url: %s", deviceName.c_str(), url);   
    delete[] url;
}


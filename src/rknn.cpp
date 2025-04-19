#include "rknn.h"

#include "logging.h"
static unsigned char *load_data(FILE *fp, size_t ofst, size_t sz);
static unsigned char *load_model(const char *filename, int *model_size);


static void dump_tensor_attr(rknn_tensor_attr* attr)
{
  printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
         "zp=%d, scale=%f\n",
         attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
         attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
         get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

rknn_lite::rknn_lite(char *model_name)
{
    /* Create the neural network */
    printf("Loading mode...\n");
    int model_data_size = 0;
    // 读取模型文件数据
    model_data = load_model(model_name, &model_data_size);
    // 通过模型文件初始化rknn类
    ret = rknn_init(&ctx, model_data, model_data_size, 0, NULL);
    if (ret < 0)
    {
        printf("rknn_init error ret=%d\n", ret);
        exit(-1);
    }
    
    // 设置npu推流使用的核心 由于rk3568只有一个核心 所以不用设置
    // rknn_core_mask core_mask;
   
    // core_mask = RKNN_NPU_CORE_0_1_2;
    // int ret = rknn_set_core_mask(ctx, core_mask);
    // if (ret < 0)
    // {
    //     printf("rknn_init core error ret=%d\n", ret);
    //     exit(-1);
    // }

    // 初始化rknn类的版本
    ret = rknn_query(ctx, RKNN_QUERY_SDK_VERSION, &version, sizeof(rknn_sdk_version));
    if (ret < 0)
    {
        printf("rknn_init error ret=%d\n", ret);
        exit(-1);
    }

    // 获取模型的输入参数
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0)
    {
        printf("rknn_init error ret=%d\n", ret);
        exit(-1);
    }

    // 设置输入数组
    input_attrs = new rknn_tensor_attr[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret < 0)
        {
            printf("rknn_init error ret=%d\n", ret);
            exit(-1);
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // 设置输出数组
    output_attrs = new rknn_tensor_attr[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs) );
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        dump_tensor_attr(&(output_attrs[i]));
    }

    // 设置输入参数
    //N - Batch
    //C - Channel
    //H - Height
    //W - Width
    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        printf("model is NCHW input fmt\n");
        channel = input_attrs[0].dims[1];
        height = input_attrs[0].dims[2];
        width = input_attrs[0].dims[3];
    }
    else
    {
        printf("model is NHWC input fmt\n");
        height = input_attrs[0].dims[1];
        width = input_attrs[0].dims[2];
        channel = input_attrs[0].dims[3];
    }

    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = width * height * channel;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].pass_through = 0;
}

rknn_lite::~rknn_lite()
{
    ret = rknn_destroy(ctx);
    delete[] input_attrs;
    delete[] output_attrs;
    if (model_data)
        free(model_data);
}

static bool letterbox_opencv(const cv::Mat &img,cv::Mat &img_letterbox,float wh_ratio)
{
    // img has to be 3 channels
    if (img.channels() != 3)
    {
        NN_LOG_ERROR("img has to be 3 channels");
        exit(-1);
    }
    float img_width = img.cols;
    float img_height = img.rows;

    int letterbox_width = 0;
    int letterbox_height = 0;

    int padding_hor = 0;    //left和right填充
    int padding_ver = 0;    //top和bottom填充

    if (img_width / img_height > wh_ratio)
    {
        letterbox_width = img_width;
        letterbox_height = img_width / wh_ratio;
        padding_hor = 0;
        padding_ver = (letterbox_height - img_height) / 2.f;;
    }
    else
    {
        letterbox_width = img_height * wh_ratio;
        letterbox_height = img_height;
        padding_hor = (letterbox_width - img_width) / 2.f;
        padding_ver = 0;
    }
    /*
     * Padding an image.
                                    dst_img
        --------------      ----------------------------
        |            |      |       top_border         |
        |  src_image |  =>  |                          |
        |            |      |      --------------      |
        --------------      |left_ |            |right_|
                            |border|  dst_rect  |border|
                            |      |            |      |
                            |      --------------      |
                            |       bottom_border      |
                            ----------------------------
     */
    // 使用cv::copyMakeBorder函数进行填充边界
    cv::copyMakeBorder(img, img_letterbox, padding_ver, padding_ver, padding_hor, padding_hor, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return true;

}

int rknn_lite::interf(std::shared_ptr<cv::Mat>& ori_img_ptr)
{
    // 检查智能指针是否为空
    if (!ori_img_ptr || ori_img_ptr->empty()) {
        std::cerr << "Invalid input image!" << std::endl;
        return -1;
    }

    cv::Mat& ori_img = *ori_img_ptr; // 解引用智能指针获取原始图像

    cv::Mat img;
    // 获取图像宽高
    int img_width = ori_img.cols;
    int img_height = ori_img.rows;
    //BGR格式转RGB格式
    cv::cvtColor(ori_img, img, cv::COLOR_BGR2RGB);

    // init rga context
    // rga是rk自家的绘图库,绘图效率高于OpenCV，但是在多线程中有bug
    rga_buffer_t src;
    rga_buffer_t dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    im_rect src_rect;
    im_rect dst_rect;
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    // You may not need resize when src resulotion equals to dst resulotion
    void *resize_buf = nullptr;
    // 如果输入图像不是指定格式
    if (img_width !=  width || img_height !=  height)
    {
        // resize_buf = malloc( height *  width *  channel);
        // memset(resize_buf, 0x00,  height *  width *  channel);

        // src = wrapbuffer_virtualaddr((void *)img.data, img_width, img_height, RK_FORMAT_RGB_888);
        // dst = wrapbuffer_virtualaddr((void *)resize_buf,  width,  height, RK_FORMAT_RGB_888);
        // ret = imcheck(src, dst, src_rect, dst_rect);
        // if (IM_STATUS_NOERROR !=  ret)
        // {
        //     printf("%d, check error! %s\n", __LINE__, imStrError((IM_STATUS) ret));
        //     exit(-1);
        // }
        // IM_STATUS STATUS = imresize(src, dst);

        //cv::Mat resize_img(cv::Size( width,  height), CV_8UC3, resize_buf);
        

        //获取图像的宽高比   
        float wh_ratio = (float)width / (float)height;

        // letterbox后的图像
        cv::Mat image_letterbox;
        //采用letterbox填充图像,使其宽高比和模型要求的相同，由于使用多线程 所以使用opencv的方式 rga有bug
        letterbox_opencv(img,image_letterbox,wh_ratio);
        // letterbox(img,image_letterbox,wh_ratio);
        cv::resize(img, img, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
        inputs[0].buf = img.data;
    }
    else
    {
        inputs[0].buf = (void *)img.data;
    }

    // 设置rknn的输入数据
    rknn_inputs_set( ctx,  io_num.n_input,  inputs);

    // 设置输出
    rknn_output outputs[ io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i <  io_num.n_output; i++)
        outputs[i].want_float = 0;
    // 调用npu进行推演
     ret = rknn_run( ctx, NULL);
    // 获取npu的推演输出结果
     ret = rknn_outputs_get( ctx,  io_num.n_output, outputs, NULL);

    // 总之就是绘图部分
    // post process
    // width是模型需要的输入宽度, img_width是图片的实际宽度
    const float nms_threshold = NMS_THRESH;     
    const float box_conf_threshold = BOX_THRESH; //预测框的置信度阈值
    float scale_w = (float) width / img_width;
    float scale_h = (float) height / img_height;

    detect_result_group_t detect_result_group;
    std::vector<float> out_scales;
    std::vector<int32_t> out_zps;
    for (int i = 0; i <  io_num.n_output; ++i)
    {
        out_scales.push_back( output_attrs[i].scale);
        out_zps.push_back( output_attrs[i].zp);
    }
    post_process((int8_t *)outputs[0].buf, (int8_t *)outputs[1].buf, (int8_t *)outputs[2].buf,  height,  width,
                 box_conf_threshold, nms_threshold, scale_w, scale_h, out_zps, out_scales, &detect_result_group);

    // Draw Objects
    char text[256];
    for (int i = 0; i < detect_result_group.count; i++)
    {
        detect_result_t *det_result = &(detect_result_group.results[i]);
        sprintf(text, "%s %.1f%%", det_result->name, det_result->prop * 100);
        //获取矩形左上角的顶点坐标
        x1 = det_result->box.left;
        y1 = det_result->box.top;
        //获取矩形右下角的顶点坐标
        x2 = det_result->box.right;
        y2 = det_result->box.bottom;
        rectangle(ori_img, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(256,0,0,256), 3);
        putText(ori_img, text, cv::Point(x1, y1 + 12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255));
        //cv::Rect roi(det_result->box.left, det_result->box.top, det_result->box.right - det_result->box.left, det_result->box.bottom - det_result->box.top);
    }
     ret = rknn_outputs_release( ctx,  io_num.n_output, outputs);
    if (resize_buf)
    {
        free(resize_buf);
    }
    return 0;
}

static unsigned char *load_data(FILE *fp, size_t ofst, size_t sz)
{
    unsigned char *data;
    int ret;

    data = NULL;

    if (NULL == fp)
    {
        return NULL;
    }

    ret = fseek(fp, ofst, SEEK_SET);
    if (ret != 0)
    {
        printf("blob seek failure.\n");
        return NULL;
    }

    data = (unsigned char *)malloc(sz);
    if (data == NULL)
    {
        printf("buffer malloc failure.\n");
        return NULL;
    }
    ret = fread(data, 1, sz, fp);
    return data;
}

static unsigned char *load_model(const char *filename, int *model_size)
{
    FILE *fp;
    unsigned char *data;

    fp = fopen(filename, "rb");
    if (NULL == fp)
    {
        printf("Open file %s failed.\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);

    data = load_data(fp, 0, size);

    fclose(fp);

    *model_size = size;
    return data;
}
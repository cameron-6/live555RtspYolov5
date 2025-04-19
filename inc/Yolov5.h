/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-09 21:59:07
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-10 19:46:01
 * @FilePath: /v4l2_mplane/inc/Yolov5.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "yolo_thread.h"
#include "VideoCapture.h"
#include "RkEncoder.h"
#include <rga/im2d.h>
#include <rga/rga.h>

struct Buffer {
    char* src_buf;
    char* dst_buf;
    rga_buffer_handle_t src_handle;
    rga_buffer_handle_t dst_handle;
};

// 定义一个结构体，用于管理缓冲区和句柄
struct RGAResizeContext {
    uint8_t* src_buf = nullptr;
    uint8_t* dst_buf = nullptr;
    rga_buffer_handle_t src_handle = 0;
    rga_buffer_handle_t dst_handle = 0;
    rga_buffer_t src_img;
    rga_buffer_t dst_img;
};

class Yolov5
{

public:
    Yolov5(V4L2DeviceParameters param,std::string model_file);
    ~Yolov5();

    void setOnEncoderDataCallback(std::function<void(std::vector<uint8_t> &&)> callback)
    {
        onEncoderDataCallback_ = std::move(callback);
    }

    void setRunStatus(bool status)
    {
        isRunning_ = status;
    }

    void start();
    void stop();

    void join();
    void detach();
private:
    bool initCameraAndEncoder(V4L2DeviceParameters param);
    static void doCapture(void*);
    void Capture();

    static void doPublish(void*);
    void publish();

    //rga cvtColor
    cv::Mat uyvy2BGRWithRGA(uint8_t* uyvy_data, int width, int height);
    bool prepareBuffers(int src_buf_size, int dst_buf_size);
    void releaseBuffers(Buffer& buffer);

    //rga resize
    bool initializeRGAResizeContext(RGAResizeContext& ctx, int src_width, int src_height, int src_format,
                                 int dst_width, int dst_height, int dst_format) ;
    void releaseRGAResizeContext(RGAResizeContext& ctx);
    cv::Mat resizeImageWithRGA(RGAResizeContext& ctx, uint8_t* src_data, int src_width, int src_height);

private:
    YoloThread *yoloThread_;
    bool isRunning_;
    std::mutex mtx_;
    VideoCapture *videoCapture_ = NULL;
    RkEncoder *rkEncoder_ = NULL;
    std::function<void(std::vector<uint8_t> &&data)> onEncoderDataCallback_;
    uint8_t* encodeData;
    uint8_t* inframe;

    int img_width_;
    int img_height_;

    std::thread t1;
    std::thread t2;

    //rga图像格式转换相关
    int src_buf_size_;
    int dst_buf_size_;
    Buffer buffer_;

    RGAResizeContext rgaRsizeCtx_;
};



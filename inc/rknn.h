/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-09 13:52:24
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-10 21:13:38
 * @FilePath: /v4l2_mplane/inc/rknn.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _rknnPool_H
#define _rknnPool_H

#include <queue>
#include <vector>
#include <iostream>
#include "rga.h"
#include "im2d.h"
#include "RgaUtils.h"
#include "rknn_api.h"
#include "postprocess.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/freetype.hpp"
//#include "ThreadPool.hpp"
using cv::Mat;
using std::queue;
using std::vector;



class rknn_lite
{
private:
    rknn_context ctx;
    unsigned char *model_data;
    rknn_sdk_version version;
    rknn_input_output_num io_num;
    rknn_tensor_attr *input_attrs;
    rknn_tensor_attr *output_attrs;
    rknn_input inputs[1];
    int ret;
    int channel = 3;
    int width = 0;
    int height = 0;
public:
    //Mat ori_img;
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    int interf(std::shared_ptr<cv::Mat>& ori_img);
    bool preProcess(const cv::Mat &img,const std::string process_type,cv::Mat &img_letterbox);
    rknn_lite(char *dst);
    ~rknn_lite();
};


#endif
/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-10-24 13:53:03
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-10 21:20:48
 * @FilePath: /ai_demo/include/yolo_thread.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "rknn.h"
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

/*线程池类，用于多线程模型推流*/
class YoloThread{
public:

    YoloThread(std::string &model_path,int thread_num = 7);
    ~YoloThread();
    
   
    bool submitTask(std::shared_ptr<cv::Mat>& img,int id);
    bool getResultAsImg(std::shared_ptr<cv::Mat>& img_ptr,int id);
    void stopAll();


private:
    // 初始化：加载模型，创建线程，参数：模型路径，线程数量
    bool setUp(std::string &model_path,int thread_num = 7);

private:
    std::queue<std::pair<int,std::shared_ptr<cv::Mat>>> tasks;       //存放图片的帧号和图片数据
    std::vector<std::shared_ptr<rknn_lite>> yolov5_instance;    //模型推理实例
    std::map<int,std::shared_ptr<cv::Mat>> img_result;   //推理后的结果，画完框的图片
    std::vector<std::thread> threads;   //线程池
    std::mutex mtx1;
    std::mutex mtx2;
    std::condition_variable cv_task, cv_result;
    bool stop;



    void worker(int id);
};  
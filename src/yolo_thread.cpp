#include "yolo_thread.h"

YoloThread::YoloThread(std::string &model_path,int thread_num)
{
    stop = false;
    setUp(model_path,thread_num);
}

YoloThread::~YoloThread()
{
    stop = true;
    cv_task.notify_all();
    for(auto &thread : threads)
    {
        if(thread.joinable())
            thread.join();
    }
}

// 线程函数。参数：线程id(在创建线程时生成)
void YoloThread::worker(int id)
{
    while (!stop)
    {
        std::pair<int,std::shared_ptr<cv::Mat>> task;

        //取出对象(一个线程对应一个对象)，用于推理
        std::shared_ptr<rknn_lite> instance = yolov5_instance[id];

        {
            std::unique_lock<std::mutex> lock(mtx1);
            cv_task.wait(lock,[&]{
                    return !tasks.empty() || stop;
                }
            );
            if(stop)
                return;

            task = tasks.front();
            tasks.pop();
        }
        //推理 4帧推理3帧，推理的结果依然保存在task.second中
        if(task.first % 4 != 0)
        {
            instance->interf(task.second);
        }
        // instance->interf(task.second);
        {
            std::lock_guard<std::mutex> lock(mtx2);
            //将推理完成的图片数据放到结果数组中
            img_result.insert({task.first,task.second});
            cv_result.notify_one();
        }
    }
    
}

// 初始化：加载模型，创建线程，参数：模型路径，线程数量
bool YoloThread::setUp(std::string &model_path,int thread_num)
{
    for (size_t i = 0; i < thread_num; i++)
    {   
        //创建用于推理的对象
        std::shared_ptr<rknn_lite> yolov5 = std::make_shared<rknn_lite>((char *)model_path.c_str());
        yolov5_instance.push_back(yolov5);
    }

    //根据指定的线程数创建线程，并指定线程函数为worker，数量和推理对象相同
    for (size_t i = 0; i < thread_num; i++)
    {
        threads.emplace_back(&YoloThread::worker,this,i);
    }
    
    return true;
}


//id表示帧号（即第几帧）
bool YoloThread::submitTask(std::shared_ptr<cv::Mat>& img,int id)
{   
    // 如果任务队列中的任务数量大于10，等待，避免内存占用过多
    while (tasks.size() > 5)
    {
        // sleep 1ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    {
        std::lock_guard<std::mutex> lock(mtx1);
        tasks.push({id,img}); //向任务队列中放入图片
    }
    cv_task.notify_one();
    return true;
}

//id表示帧号
bool YoloThread::getResultAsImg(std::shared_ptr<cv::Mat>& img_ptr,int id)
{
    int loop = 0;
    //根据图片的帧号获取对应的推流后的图片
    while (img_result.find(id) == img_result.end())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        loop++;
        if(loop > 1000)
        {
            std::cerr<<"getResultAsImg timeout"<<std::endl;
            return false;
        }
    }
    
    std::lock_guard<std::mutex> lock(mtx2);
    img_ptr = img_result[id];
    img_result.erase(id); //取出图片后删除map中的图片
    return true;
}
void YoloThread::stopAll()
{
    stop = true;
    cv_task.notify_all();
}

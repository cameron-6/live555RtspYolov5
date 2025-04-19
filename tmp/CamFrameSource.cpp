/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-06 11:11:31
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-10 14:51:02
 * @FilePath: /liveRtsp/src/CamFrameSource.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "CamFrameSource.h"
#include "log.h"
#include <vector>
#include <fstream>
#include <iostream>


CamFrameSource* CamFrameSource::createNew(UsageEnvironment &env,CamRTSPServer *cam)
{
    
    return new CamFrameSource(env,cam);
}

CamFrameSource::CamFrameSource(UsageEnvironment &env,CamRTSPServer *cam) : FramedSource(env)
{
    videoCapture = NULL;
    rk_encoder = NULL;
    //encodedDataBuffer_.reserve(5);
    cam->setOnEncoderDataCallback(std::bind(&CamFrameSource::onEncodedData, this, std::placeholders::_1));
    eventTriggerId_ = envir().taskScheduler().createEventTrigger(CamFrameSource::doDeliverFrame);
    //initVideoEncoder();
}

CamFrameSource::~CamFrameSource()
{
    if (inframe)
    {
       free(inframe);
       inframe = NULL;
    }
    if (encodeData)
    {
        free(encodeData);
        encodeData = NULL;
    }
    
    
    envir().taskScheduler().unscheduleDelayedTask(m_token);
}

void CamFrameSource::doDeliverFrame(void* ptr)
{
    LOG(NOTICE,"doDeliverFrame into");
    ((CamFrameSource*)ptr)->deliverFrame();
}

void CamFrameSource::deliverFrame()
{
    //LOG(NOTICE,"deliverFrame into");
    if (!isCurrentlyAwaitingData()) {
        LOG(NOTICE,"isCurrentlyAwaitingData");
        return;
    }

    //LOG(NOTICE,"deliverFrame into");


    {
        std::unique_lock<std::mutex> lock(encodedDataMutex);
        encodedData_ = encodedDataBuffer_.front();
        encodedDataBuffer_.pop();
    }
    
    //LOG(NOTICE, "get encodedData_ end");
    if (encodedData_.size() > fMaxSize) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = static_cast<unsigned int>(encodedData_.size() - fMaxSize);
        LOG(WARN, "Exceeded max size, truncated: %d, size: %d", fNumTruncatedBytes, encodedData_.size());
    } else {
        fFrameSize = static_cast<unsigned int> (encodedData_.size());
    }
    //LOG(NOTICE,"fFrameSize = %d",fFrameSize);
     // can be changed to the actual frame's captured time
    gettimeofday(&fPresentationTime, nullptr);
    
    // DO NOT CHANGE ADDRESS, ONLY COPY (see Live555 docs)
    memmove(fTo, encodedData_.data(), fFrameSize);

    // should be invoked after successfully getting data
    //FramedSource::afterGetting(this);
    afterGetting(this);

}

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


void CamFrameSource::onEncodedData(std::vector<uint8_t> &&data)
{
    //当前流是否处于等待数据的状态。如果不是则直接返回
    if(!isCurrentlyAwaitingData())
    {
        //LOG(NOTICE,"isCurrentlyAwaitingData ,return!");

        return;
    }
    LOG(NOTICE, "publish");

    {
        std::unique_lock<std::mutex> lock(encodedDataMutex);
        encodedDataBuffer_.emplace(std::move(data));
    }
    
    

   // LOG(NOTICE, "encodedDataBuffer_.emplace(std::move(data)); end");
    //触发事件，即执行函数doDeliverFrame（在构造函数中已经绑定了该函数）
    envir().taskScheduler().triggerEvent(eventTriggerId_,this);

}

int CamFrameSource::initVideoEncoder()
{
    LOG(NOTICE,"initVideoEncoder into");
    if (videoCapture != NULL && rk_encoder != NULL)
    {
        return 0;
    }
    

    V4L2DeviceParameters param;
	param.buffer_cnt = 3;
	param.devName = "/dev/video0";
	param.width = 640;
	param.height = 480;
	param.fps = 30;

    MppFrameFormat mpp_fmt;
    mpp_fmt = MPP_FMT_YUV420SP;

    videoCapture = new VideoCapture(param);
    LOG(NOTICE,"videoCapture create");

    videoCapture->start();

    encodeData = (uint8_t *)malloc(videoCapture->getBufferSize());
    inframe = (uint8_t *)malloc(videoCapture->getBufferSize());
    Encoder_Param_t encoder_param{
        mpp_fmt,
        640,
        480,
        0,
        0,
        0,
        30,  //fps
        23};
        
    LOG(NOTICE,"RkEncoder create");
    rk_encoder = new RkEncoder(encoder_param);
    LOG(NOTICE,"RkEncoder init");

    rk_encoder->init();
    LOG(NOTICE,"RkEncoder init end");

    LOG(NOTICE,"initVideoEncoder out");

    return 0;
}

unsigned CamFrameSource::maxFrameSize() const
{
    return 1024*100;
}


void CamFrameSource::doGetNextFrame()
{
    LOG(NOTICE,"doGetNextFrame into");

    if (!encodedDataBuffer_.empty())
    {
        //LOG(NOTICE,"encodedDataBuffer_ is not empty");

        deliverFrame();
    }
    else
    {
        fFrameSize = 0;
        //afterGetting(this);
        return;
    }

    // //每隔50ms调用一次getNextFrame
    // int to_delay = 20*1000;
    // m_token = envir().taskScheduler().scheduleDelayedTask(to_delay,getNextFrame,this);
    
}

void CamFrameSource::getNextFrame(void *ptr)
{
    ((CamFrameSource*)ptr)->getNextH264Frame();
}


void CamFrameSource::getNextH264Frame()
{
    LOG(DEBUG,"getNextH264Frame into");
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int startCode = 0;
    int ret = videoCapture->isReadable(&tv);
    if (ret)
    {                                           
        int frameSize = videoCapture->get_frame((char*)inframe,videoCapture->getBufferSize());
        
        LOG(NOTICE,"frame = %x",inframe);
        //LOG(NOTICE,"frameSize = %d",frameSize);
        frameSize = rk_encoder->encode(inframe,frameSize,encodeData);
        //appendH264FrameToFile("output.h264", encodeData, frameSize);
        LOG(NOTICE,"frameSize = %d",frameSize);

        if (frameSize)
        {
            gettimeofday(&fPresentationTime,NULL); //设置pts，直接使用当前的系统时间
            if (rk_encoder->startCode3(encodeData))
            {
                startCode = 3;
                LOG(NOTICE,"startCode = %d",startCode);   
            }
            else
            {
                startCode = 4;
                LOG(NOTICE,"startCode = %d",startCode);  
            }
                
            //LOG(NOTICE,"memcpy(fTo,encodeData,frameSize);");
            memmove(fTo,encodeData,frameSize);
            fFrameSize = frameSize;
            
        }
        
       //free(inframe); 
    }
    //LOG(NOTICE,"afterGetting into");

    afterGetting(this);
}

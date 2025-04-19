/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-06 11:06:13
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-09 20:51:27
 * @FilePath: /src/liveRtsp/include/CamFrameSource.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include <liveMedia/FramedSource.hh>
#include <UsageEnvironment/UsageEnvironment.hh>
#include "VideoCapture.h"
#include "RkEncoder.h"
#include <mutex>
#include <queue>
#include <vector>
#include "CamRTSPServer.h"

class CamRTSPServer;

class CamFrameSource : public FramedSource
{

public:
    static CamFrameSource* createNew(UsageEnvironment &env,CamRTSPServer *cam);

    CamFrameSource(UsageEnvironment &env,CamRTSPServer *cam);
    ~CamFrameSource();
    void doGetNextFrame() override;
    unsigned maxFrameSize() const override;

private:
    /* data */
    EventTriggerId eventTriggerId_;
    static void getNextFrame(void *ptr);
    void getNextH264Frame();
    int initVideoEncoder();

    

    static void doDeliverFrame(void*);
    void deliverFrame();

    //回调函数，用于存储编码后的数据
    void onEncodedData(std::vector<uint8_t> &&data);
    std::queue<std::vector<uint8_t> > encodedDataBuffer_;//保存编码后的数据
    std::mutex encodedDataMutex;
    std::vector<uint8_t> encodedData_;

    VideoCapture *videoCapture;
    RkEncoder *rk_encoder;
    uint8_t *encodeData;
    TaskToken m_token;
    uint8_t *inframe;
 
};


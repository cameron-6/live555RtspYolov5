/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-06 11:11:15
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-09 21:53:43
 * @FilePath: /src/liveRtsp/src/H264DemandSubsession.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "H264DemandSubsession.h"
#include <liveMedia/H264VideoRTPSink.hh>
#include "CamFrameSource.h"
#include <string.h>
#include <stdlib.h>
#include "log.h"

H264DemandSubsession::H264DemandSubsession(UsageEnvironment &env,FramedSource *source,CamRTSPServer* cam)
    :OnDemandServerMediaSubsession(env,true)
{
    m_pFramesrc = source;
    m_cam = cam;
    m_pSDPLine = NULL;
    m_pDummyRTPSink = NULL;
    m_stateDone = 0;
}

H264DemandSubsession::~H264DemandSubsession()
{
    if (m_pSDPLine)
    {
        free(m_pSDPLine);
        m_pSDPLine = NULL;
    }
    
}

H264DemandSubsession* H264DemandSubsession::createNew(UsageEnvironment &env,FramedSource *source,CamRTSPServer* cam)
{
    return new H264DemandSubsession(env,source,cam);
}


//这是一个回调函数，在 m_pDummyRTPSink 播放完成后被调用，用来设置 m_stateDone 为完成标志 0xff。
void H264DemandSubsession::afterPlayingDummy(void* ptr)
{
    ((H264DemandSubsession*)ptr)->m_stateDone = 0xff;
}

void H264DemandSubsession::checkForAuxSDPLine(void* ptr)
{
    ((H264DemandSubsession*)ptr)->chkForAuxSDPFromSink();
}

void H264DemandSubsession::chkForAuxSDPFromSink()
{
    //如果 m_pDummyRTPSink 的 auxSDPLine 已经生成，则将 m_stateDone 设置为 0xff，表示操作完成。否则，继续调度任务检查。
    if (m_pDummyRTPSink->auxSDPLine())
    {
        m_stateDone = 0xff;
    }else{
        int delay = 50 * 1000;
        nextTask() = envir().taskScheduler().scheduleDelayedTask(delay,checkForAuxSDPLine,this);
    }
}


//生成并返回媒体流的 SDP 描述信息
char const* H264DemandSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
{
    LOG(NOTICE,"getAuxSDPLine into");
    if (m_pSDPLine)
    {
        LOG(NOTICE,"return m_pSDPLine");

        return m_pSDPLine;
    }

    m_pDummyRTPSink->startPlaying(*inputSource,0,0);
    checkForAuxSDPLine(this);
    m_stateDone = 0;
    //该部分启动一个事件循环，直到 m_stateDone 被设置为 0xff。这个事件循环会调用 checkForAuxSDPLine，并等待直到 m_pDummyRTPSink 完成生成 SDP 线。
    envir().taskScheduler().doEventLoop(&m_stateDone);
    m_pSDPLine = strdup(m_pDummyRTPSink->auxSDPLine());
    m_pDummyRTPSink->stopPlaying();
    
    return m_pSDPLine;
}


//该方法用于创建一个新的媒体流源（FramedSource）
FramedSource* H264DemandSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    LOG(NOTICE,"createNewStreamSource into");

    estBitrate = 50000;
    return H264VideoStreamFramer::createNew(envir(),(FramedSource *)(new CamFrameSource(envir(),m_cam)));
}

RTPSink* H264DemandSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
    return H264VideoRTPSink::createNew(envir(),rtpGroupsock,rtpPayloadTypeIfDynamic);
}

char const* H264DemandSubsession::sdpLines(int addressFamily)
{
    return fSDPLines = (char *) "m=video 0 RTP/AVP 96\r\n"
                                "c=IN IP4 0.0.0.0\r\n"
                                "b=AS:96\r\n"
                                "a=rtpmap:96 H264/90000\r\n"
                                "a=fmtp:96 packetization-mode=1;profile-level-id=000000;sprop-parameter-sets=H264\r\n"
                                "a=control:track1\r\n";
}
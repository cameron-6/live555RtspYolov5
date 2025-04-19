/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-06 11:06:58
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-09 15:55:29
 * @FilePath: /src/liveRtsp/include/H264DemandSubsession.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once


#include <liveMedia/OnDemandServerMediaSubsession.hh>
#include <liveMedia/H264VideoStreamFramer.hh>
#include <liveMedia/H264VideoRTPSink.hh>
#include <liveMedia/liveMedia.hh>

#include <BasicUsageEnvironment/BasicUsageEnvironment.hh>
#include <UsageEnvironment/UsageEnvironment.hh>
#include <H264VideoStreamDiscreteFramer.hh>
#include <StreamReplicator.hh>
#include <groupsock/Groupsock.hh>
#include "CamRTSPServer.h"

class CamRTSPServer;

class H264DemandSubsession : public OnDemandServerMediaSubsession
{

public:
    H264DemandSubsession(UsageEnvironment &env,FramedSource *source,CamRTSPServer* cam);
    ~H264DemandSubsession();

    static H264DemandSubsession * createNew(UsageEnvironment &env,FramedSource *source,CamRTSPServer* cam);

protected:
    //生成并返回媒体流的 SDP 描述信息
    char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) override;

    //该方法用于创建一个新的媒体流源（FramedSource）
    FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate) override;

    //该方法用于创建一个新的 RTP Sink（RTPSink）。RTP Sink 是一个处理和发送 RTP 数据包的对象
    RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource) override;

    //生成和返回一个 SDP（Session Description Protocol）描述信息，用于客户端了解会话的基本信息。
    char const* sdpLines(int addressFamily) override;

private:
    //这是一个回调函数，在 m_pDummyRTPSink 播放完成后被调用，用来设置 m_stateDone 为完成标志 0xff。
    static void afterPlayingDummy(void* ptr);
    //定时检查 m_pDummyRTPSink 是否已生成了 auxSDPLine，如果生成了就完成操作。如果没有生成，等待 50 毫秒后再次检查。
    static void checkForAuxSDPLine(void* ptr);
    void chkForAuxSDPFromSink();
private:
    //static void 
    FramedSource *  m_pFramesrc;
    CamRTSPServer*  m_cam;
    char *          m_pSDPLine;
    RTPSink*        m_pDummyRTPSink;
    char            m_stateDone;
};


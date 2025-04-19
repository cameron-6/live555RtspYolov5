/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-10 15:22:40
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-11 13:51:41
 * @FilePath: /v4l2_mplane/src/CamUnicastServerMediaSubsession.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "CamUnicastServerMediaSubsession.h"
#include "log.h"

CamUbicastServerMediaSubsession *
CamUbicastServerMediaSubsession::createNew(UsageEnvironment &env, 
                StreamReplicator *replicator, 
                size_t estBitrate, size_t udpDatagramSize) 
{
    return new CamUbicastServerMediaSubsession(env, replicator, estBitrate, udpDatagramSize);
}

CamUbicastServerMediaSubsession::CamUbicastServerMediaSubsession(UsageEnvironment &env,
                                                                 StreamReplicator *replicator,
                                                                 size_t estBitrate,
                                                                 size_t udpDatagramSize) :
    OnDemandServerMediaSubsession(env, False),
    replicator_(replicator),
    estBitrate_(estBitrate),
    udpDatagramSize_(udpDatagramSize)
{
    LOG(NOTICE, "Unicast media subsession with UDP datagram size of %d\n" \
               "\tand estimated bitrate of %d (kbps) is created", udpDatagramSize, estBitrate);

}

FramedSource *
CamUbicastServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate)
{
    estBitrate = static_cast<unsigned int> (this->estBitrate_);
    LOG(NOTICE,"createNewStreamSource into");

    //StreamReplicator 会将原始数据源的数据复制到多个副本，从而实现多客户端访问的效率提升。
    //replicator_已经和自己创建的framedSource关联，所以可以复制自己设置的源
    //每次有新的客户端连接时都会为对应的客户端创建一个独立的source副本
    auto source = replicator_->createStreamReplica(); 

    //H264or5VideoStreamDiscreteFramer
    // only discrete frames are being sent (w/o start code bytes)
    return H264VideoStreamDiscreteFramer::createNew(envir(), source);
}

RTPSink *
CamUbicastServerMediaSubsession::createNewRTPSink(Groupsock *rtpGroupsock,//用于传输 RTP 数据包的 Groupsock 对象
                          unsigned char rtpPayloadTypeIfDynamic,         //RTP 动态负载类型
                          FramedSource *inputSource                      //音视频帧输入源
                          )
{
    LOG(NOTICE,"createNewRTPSink into");
    auto sink = H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    //设置 RTP 数据包的最大尺寸。第一个参数是组装 RTP 数据包时，期望每个包包含的数据负载大小，第二个参数是RTP包的最大值
    sink->setPacketSizes(static_cast<unsigned int> (udpDatagramSize_), static_cast<unsigned int>(udpDatagramSize_));
    return sink;
}

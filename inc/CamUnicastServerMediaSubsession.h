/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-10 15:22:20
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-10 15:23:14
 * @FilePath: /v4l2_mplane/inc/CamUnicastServerMediaSubsession.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef CAM_UNICAST_SERVER_MEDIA_SUBSESSION_H
#define CAM_UNICAST_SERVER_MEDIA_SUBSESSION_H

#include <liveMedia/OnDemandServerMediaSubsession.hh>
#include <liveMedia/StreamReplicator.hh>
#include <liveMedia/H264VideoRTPSink.hh>
#include <liveMedia/H264VideoStreamDiscreteFramer.hh>

class CamUbicastServerMediaSubsession : public OnDemandServerMediaSubsession 
{
public:
    static CamUbicastServerMediaSubsession *
    createNew(UsageEnvironment &env, StreamReplicator *replicator, size_t estBitrate, size_t udpDatagramSize);


protected:
    StreamReplicator *replicator_;

    size_t estBitrate_;

    size_t udpDatagramSize_;

    CamUbicastServerMediaSubsession(UsageEnvironment &env, 
                                    StreamReplicator *replicator,
                                    size_t estBitrate,
                                    size_t udpDatagramSize);

    FramedSource *createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate) override;

    RTPSink *createNewRTPSink(Groupsock *rtpGroupsock, 
                              unsigned char rtpPayloadTypeIfDynamic, 
                              FramedSource *inputSource) override;
};

#endif
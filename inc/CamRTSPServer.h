/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-10 15:21:37
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-10 19:47:27
 * @FilePath: /v4l2_mplane/inc/CamRTSPServer1.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef CAM_RTSP_SERVER_H
#define CAM_RTSP_SERVER_H

#include <UsageEnvironment.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <liveMedia.hh>
#include <string>
#include <iostream>
#include "CamFramedSource.h"
#include "CamUnicastServerMediaSubsession.h"
#include "Yolov5.h"

class CamRTSPServer 
{
public:

    CamRTSPServer();
    ~CamRTSPServer();

    void stopServer();

    //void addTranscoder(std::shared_ptr<TransCoder> transcoder);
    void addYolov5Instance(std::shared_ptr<Yolov5> yolov5);
    void run();

private:
    char watcher_;
    TaskScheduler *scheduler_;
    UsageEnvironment *env_;
    RTSPServer *server_;
    size_t biteRate_;

    //用于估算合适的比特率
    size_t estimateBitrate(unsigned width, unsigned height, unsigned fps, std::string codec = "H264");
    /**
     * @brief Video sources.
     */
    //std::vector<std::shared_ptr<TransCoder>> transcoders_;
    std::vector<std::shared_ptr<Yolov5>> yolov5s_;
    /**
     * @brief Framed sources.
     */
    std::vector<FramedSource *> allocatedVideoSources;

    /**
     * @brief Announce new create media session.
     * 
     * @param sms - create server media session.
     * @param deviceName - the name of video source device.
     */
    void announceStream(ServerMediaSession *sms, const std::string &deviceName);

    /**
     * @brief Add new server media session using transcoder as a source to the server
     * 
     * @param transoder - video source.
     * @param streamName - the name of stream (part of the URL), e.g. rtsp://<ip>/camera/1.
     * @param streamDesc - description of stream.
     */
    void addMediaSession(std::shared_ptr<Yolov5> yolov5, const std::string &streamName, const std::string &streamDesc);

};

#endif
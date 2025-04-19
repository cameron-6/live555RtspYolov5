/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-08 21:37:43
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-09 19:18:36
 * @FilePath: /v4l2_mplane/inc/CamRTSPServer.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef CAM_RTSP_SERVER_H
#define CAM_RTSP_SERVER_H

#include <UsageEnvironment.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <liveMedia.hh>
#include <string>
#include "CamFrameSource.h"

#include "RkEncoder.h"
#include "VideoCapture.h"
#include <functional>
#include <vector>
#include "yolo_thread.h"

class CamRTSPServer 
{
public:

    CamRTSPServer(V4L2DeviceParameters param,std::string model_file);
    ~CamRTSPServer();

    void stopServer();

    void run();

    void setOnEncoderDataCallback(std::function<void(std::vector<uint8_t>&&data)> callback)
    {
        onEncoderDataCallback_ = callback;
    }

private:
    bool initCameraAndEncoder(V4L2DeviceParameters param);
    static void doCapture(void*);
    void Capture();

    static void doPublish(void*);
    void publish();
private:
    bool isRunning_ = false;
    static int g_frame_start_id;
    static int g_frame_end_id;
    char watcher_;
    TaskScheduler *scheduler_;
    UsageEnvironment *env_;
    RTSPServer *server_;
    
    YoloThread *yoloThread_;
    
    VideoCapture *videoCapture_ = NULL;
    RkEncoder *rkEncoder_ = NULL;
    std::function<void(std::vector<uint8_t> &&data)> onEncoderDataCallback_;
    uint8_t* encodeData;
    uint8_t* inframe;
    std::mutex mtx_;

    /**
     * @brief Framed sources.
     */

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

};

#endif
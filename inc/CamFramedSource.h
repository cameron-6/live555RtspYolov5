/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-10 15:32:16
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-11 14:10:29
 * @FilePath: /v4l2_mplane/inc/CamFramedSource.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef CAM_FRAME_SOURCE_H
#define CAM_FRAME_SOURCE_H

#include <FramedSource.hh>
#include <UsageEnvironment.hh>
#include <mutex>
#include "Yolov5.h"
#include <queue>

class CamFramedSource : public FramedSource {
public:
    static CamFramedSource *createNew(UsageEnvironment &env, Yolov5 &yolov5);

protected:
    CamFramedSource(UsageEnvironment &env,  Yolov5 &yolov5);
    ~CamFramedSource();

    void doGetNextFrame() override;
    void doStopGettingFrames() override;

private:
    static bool begin;
    Yolov5 &yolov5_;
    EventTriggerId eventTriggerId;
    std::mutex encodedDataMutex;

    size_t max_nalu_size_bytes;
    std::vector<uint8_t> encodedData;
    std::queue<std::vector<uint8_t> > encodedDataBuffer;

    void onEncodedData(std::vector<uint8_t> &&data);

    static void deliverFrame0(void *);
    void deliverData();
};

#endif
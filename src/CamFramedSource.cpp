#include "CamFramedSource.h"
#include "log.h"

CamFramedSource *CamFramedSource::createNew(UsageEnvironment &env, Yolov5 &yolov5) 
{
    return new CamFramedSource(env, yolov5);
}

CamFramedSource::CamFramedSource(UsageEnvironment &env,  Yolov5 &yolov5) :
    FramedSource(env),
    yolov5_(yolov5),
    eventTriggerId(0),
    max_nalu_size_bytes(0)
{
    // create trigger invoking method which will deliver frame
    eventTriggerId = envir().taskScheduler().createEventTrigger(CamFramedSource::deliverFrame0);
    //encodedDataBuffer.reserve(5);  // reserve enough space for handling incoming encoded data

    // set transcoder's callback indicating new encoded data availabity ，设置回调函数
    //transcoder_.setOnEncoderDataCallback(std::bind(&CamFramedSource::onEncodedData, this, std::placeholders::_1));
    yolov5_.setOnEncoderDataCallback(std::bind(&CamFramedSource::onEncodedData,this,std::placeholders::_1));

    LOG(NOTICE, "Starting to capture and encoder video from video ");
    
    // yolov5.start();
    // yolov5.detach();
    //yolov5.join();
}

CamFramedSource::~CamFramedSource()
{
    yolov5_.stop();

    envir().taskScheduler().deleteEventTrigger(eventTriggerId);
    while (!encodedDataBuffer.empty())
    {
        encodedDataBuffer.pop();
    }
    
    // encodedDataBuffer.clear();
    // encodedDataBuffer.shrink_to_fit();
}

// 回调函数，called when new encoded data is available
void CamFramedSource::onEncodedData(std::vector<uint8_t> &&data)
{
    if (!isCurrentlyAwaitingData()) {
        return;
    }

    encodedDataMutex.lock();
    encodedDataBuffer.emplace(std::move(data));
    encodedDataMutex.unlock();
    // trigger event to deliver frame
    envir().taskScheduler().triggerEvent(eventTriggerId, this);
}

void CamFramedSource::deliverFrame0(void *clientData) {
    ((CamFramedSource*) clientData)->deliverData();
}

void CamFramedSource::deliverData() {
    if (!isCurrentlyAwaitingData()) {
        return;
    }

    if(encodedDataBuffer.empty())
        return;

    encodedDataMutex.lock();
    encodedData = std::move(encodedDataBuffer.front());
    encodedDataBuffer.pop();
    encodedDataMutex.unlock();

    if (encodedData.size() > max_nalu_size_bytes) {
        max_nalu_size_bytes = encodedData.size();
    }

    if (encodedData.size() > fMaxSize) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = static_cast<unsigned int>(encodedData.size() - fMaxSize);
        LOG(WARN, "Exceeded max size, truncated: %d, size: %d", fNumTruncatedBytes, encodedData.size());
    } else {
        fFrameSize = static_cast<unsigned int> (encodedData.size());
    }

    // can be changed to the actual frame's captured time
    gettimeofday(&fPresentationTime, nullptr);
    
    // DO NOT CHANGE ADDRESS, ONLY COPY (see Live555 docs)
    memmove(fTo, encodedData.data(), fFrameSize);

    // should be invoked after successfully getting data
    FramedSource::afterGetting(this);
}

bool CamFramedSource::begin = true;

void CamFramedSource::doGetNextFrame()
{
    if(begin)
    {
        yolov5_.start();
        yolov5_.detach();
        begin = false;
    }

    if (!encodedDataBuffer.empty()) {
        deliverData();
    } else {
        fFrameSize = 0;
        return;
    }
}

void CamFramedSource::doStopGettingFrames()
{
    LOG(NOTICE, "Stop getting frames from the camera");
    FramedSource::doStopGettingFrames();
}

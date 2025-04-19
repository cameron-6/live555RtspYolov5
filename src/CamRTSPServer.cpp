#include "CamRTSPServer.h"
#include "log.h"
CamRTSPServer::CamRTSPServer() :
    watcher_(0),
    scheduler_(nullptr),
    env_(nullptr)
{
    // INIReader configs("./configs/config.ini");
    // if (configs.ParseError() < 0) {
    //     LOG(ERROR, "read server config failed.");
    //     return;
    // } else {
    //     config.rtspPort = configs.GetInteger("server", "rtsp_port", 8554);
    //     config.streamName = configs.Get("server", "stream_name", "unicast");
	// config.maxBufSize = configs.GetInteger("server", "max_buf_size", 2000000);
	// config.maxPacketSize = configs.GetInteger("server", "max_packet_size", 1500);
	// config.isHttpEnable = configs.GetBoolean("server", "http_ebable", false);
    //     config.httpPort = configs.GetInteger("server", "http_port", 8000);
    //     config.bitrate = configs.GetInteger("server", "bitrate", 2000);
    // }
    biteRate_ = estimateBitrate(640,480,30);

    OutPacketBuffer::maxSize = 200000;
    LOG(NOTICE, "setting OutPacketbuffer max size to %d (bytes)", OutPacketBuffer::maxSize);

    scheduler_ = BasicTaskScheduler::createNew();
    env_ = BasicUsageEnvironment::createNew(*scheduler_);
}

CamRTSPServer::~CamRTSPServer()
{
    Medium::close(server_);

    for (auto &src : allocatedVideoSources) {
        if(src) 
            Medium::close(src);
    }

    env_->reclaim();
    if (scheduler_)
        delete scheduler_;
    
    yolov5s_.clear();
    allocatedVideoSources.clear();
    watcher_ = 0;

    LOG(NOTICE, "RTSP server has been destructed!");
}

void CamRTSPServer::stopServer()
{
    LOG(NOTICE, "Stop server is involed!");
    watcher_ = 's';
}

void CamRTSPServer::run()
{
    // create server listening on the specified RTSP port
    server_ = RTSPServer::createNew(*env_, 8554);
    if (!server_) {
        LOG(ERROR, "Failed to create RTSP server: %s", env_->getResultMsg());
        exit(1);
    }

    LOG(NOTICE, "Server has been created on port 8554");

    LOG(INFO, "Creating media session for each transcoder");
    char const *streamName = "live";
    for (auto &yolov5 : yolov5s_) {
        addMediaSession(yolov5, streamName, "stream description");
    }

    env_->taskScheduler().doEventLoop(&watcher_); // do not return;
}

void CamRTSPServer::announceStream(ServerMediaSession *sms, const std::string &deviceName) 
{
    auto url = server_->rtspURL(sms);
    LOG(NOTICE, "Play the stream of the %s, url: %s", deviceName.c_str(), url);   
    delete[] url;
}

size_t CamRTSPServer::estimateBitrate(unsigned width, unsigned height, unsigned fps, std::string codec)
{
    // 基本比特率估算 (单位 kbps)
    double baseRate = (width * height * fps) / 1000.0;

    if (codec == "H264") {
        baseRate *= 0.07; // H.264 系数
    } else if (codec == "H265") {
        baseRate *= 0.05; // H.265 系数
    } else if (codec == "VP9" || codec == "AV1") {
        baseRate *= 0.04; // VP9/AV1 系数
    }
    // 防止过低或过高
    size_t bitrate = static_cast<size_t>(baseRate);
    return bitrate; // 限制在 500 - 20000 kbps
}

void CamRTSPServer::addMediaSession(std::shared_ptr<Yolov5> yolov5, const std::string &streamName, 
                                    const std::string &streamDesc)
{
    LOG(NOTICE, "Adding media session for camera: /dev/video0");

    // create framed source for camera
    auto framedSource = CamFramedSource::createNew(*env_, *yolov5);

    // store it in order to cleanup after
    allocatedVideoSources.push_back(framedSource);

    // store it in order to replicator for the framed source **
    auto replicator = StreamReplicator::createNew(*env_, framedSource, False);

    // create media session with the specified topic and descripton
    auto sms = ServerMediaSession::createNew(*env_, streamName.c_str(), "stream information",
                                            streamDesc.c_str(), False, "a=fmtp:96\n");

    // add unicast subsession 
    //参数2：传输的视频为640x480 bitrate设置为500-2000之间即可，参数3：RTP数据包的大小，推荐设置为1400
    LOG(NOTICE,"biteRate_ = %d",biteRate_);
    sms->addSubsession(CamUbicastServerMediaSubsession::createNew(*env_, replicator, 1440, 1400));

    server_->addServerMediaSession(sms);

    announceStream(sms, "/dev/video0");
}

void CamRTSPServer::addYolov5Instance(std::shared_ptr<Yolov5> yolov5)
{
    yolov5s_.emplace_back(yolov5);
}

// void CamRTSPServer::addTranscoder(std::shared_ptr<TransCoder> transcoder)
// {
//     transcoders_.emplace_back(transcoder);
// }

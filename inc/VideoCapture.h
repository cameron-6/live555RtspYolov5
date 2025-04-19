#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <errno.h>
#include <string>


struct V4L2DeviceParameters
{
    std::string devName;
	uint16_t width;
	uint16_t height;
	int fps;
	int buffer_cnt;
	//int m_openFlags;
};

struct plane_start {
	void * start;
};

struct buffer {
	struct plane_start* plane_start;
	struct v4l2_plane* planes_buffer;
};

class VideoCapture
{

public:
    VideoCapture(V4L2DeviceParameters param);
    ~VideoCapture();

    bool setFmt();

    bool require_buf();

    bool start();

    bool stop();
    bool isReadable(timeval *tv);

    size_t get_frame(char *buffer,size_t bufferSize);

    size_t inline getBufferSize()
    {
        return bufferSize_;
    }
private:
    int fd_ = -1;
    std::string dev_name_;
    struct timeval tv;
	int ret = -1, i, j, r;
	int num_planes_;
	struct v4l2_capability cap_;
	struct v4l2_format fmt_;
	struct v4l2_requestbuffers req_;
	struct v4l2_buffer buf_;
	struct v4l2_plane* planes_buffer_;
	struct plane_start* plane_start_;
	struct buffer *buffers_;
	enum v4l2_buf_type type_;
    struct v4l2_plane *tmp_plane;
    int width_;
    int height_;
    int fps_;
    int buffer_cnt_;
    int bufferSize_;
};


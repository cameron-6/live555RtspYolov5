/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-08 19:24:14
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-10 22:06:44
 * @FilePath: /v4l2_mplane/src/VideoCapture.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "VideoCapture.h"
#include <sys/select.h>

VideoCapture::VideoCapture(V4L2DeviceParameters param)
:   dev_name_(param.devName),
    width_(param.width),
    height_(param.height),
    fps_(param.fps),
    buffer_cnt_(param.buffer_cnt)
{
    fd_ = open(dev_name_.c_str(), O_RDWR);
    if (fd_< 0)
    {
        printf("open device: %s fail\n", dev_name_.c_str());
        return;
    }
    if (ioctl(fd_, VIDIOC_QUERYCAP, &cap_) < 0) {
		printf("Get video capability error!\n");
		//goto err1;
	}

	if (!(cap_.device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
		printf("Video device not support capture!\n");
		//goto err1;
        close(fd_);
	}

	printf("Support capture!\n");

    auto ret = setFmt();
    if (!ret)
    {
        printf("Set format fail\n");
        return ;
    }

    ret = require_buf();
    if (!ret)
    {
        printf("require_buf fail\n");
        return ;
    }
    
    
}

VideoCapture::~VideoCapture()
{
    if (planes_buffer_)
    {
        free(planes_buffer_);
        planes_buffer_ = NULL;
    }
    if (plane_start_)
    {
        free(plane_start_);
        plane_start_ = NULL;
    }
    if (buffers_)
    {
        free(buffers_);
        buffers_ = NULL;
    }
    
    
}


bool VideoCapture::setFmt()
{
    memset(&fmt_, 0, sizeof(struct v4l2_format));
	fmt_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt_.fmt.pix_mp.width = 640;
	fmt_.fmt.pix_mp.height = 480;
	fmt_.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_UYVY;
	fmt_.fmt.pix_mp.field = V4L2_FIELD_ANY;


	if (ioctl(fd_, VIDIOC_S_FMT, &fmt_) < 0) {
		printf("Set format fail\n");
		//goto err1;
        return false;
	}

    printf("Set format Success!\n");
	printf("width = %d\n", fmt_.fmt.pix_mp.width);
	printf("height = %d\n", fmt_.fmt.pix_mp.height);
	printf("nmplane = %d\n", fmt_.fmt.pix_mp.num_planes);
	printf("bufferSize = %d\n",fmt_.fmt.pix_mp.plane_fmt[0].sizeimage);
    bufferSize_ = fmt_.fmt.pix_mp.plane_fmt[0].sizeimage;
    return true;
}

bool VideoCapture::require_buf()
{
    req_.count = buffer_cnt_;
	req_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	req_.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd_, VIDIOC_REQBUFS, &req_) < 0) {
		printf("Reqbufs fail\n");
		//goto err1;
        return false;
	}

	printf("buffer number: %d\n", req_.count);

	num_planes_ = fmt_.fmt.pix_mp.num_planes;

	buffers_ = (struct buffer*)malloc(req_.count * sizeof(*buffers_));

	for(i = 0; i < req_.count; i++) {
		memset(&buf_, 0, sizeof(buf_));
		planes_buffer_ = (v4l2_plane*)calloc(num_planes_, sizeof(*planes_buffer_));
		plane_start_ = (struct plane_start*)calloc(num_planes_, sizeof(*plane_start_));
		memset(planes_buffer_, 0, sizeof(*planes_buffer_));
		buf_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf_.memory = V4L2_MEMORY_MMAP;
		buf_.m.planes = planes_buffer_;
		buf_.length = num_planes_;
		buf_.index = i;
		if (-1 == ioctl (fd_, VIDIOC_QUERYBUF, &buf_)) {
			printf("Querybuf fail\n");
			req_.count = i;
			//goto err2;
            return false;
		}

		(buffers_ + i)->planes_buffer = planes_buffer_;
		(buffers_ + i)->plane_start = plane_start_;
		for(j = 0; j < num_planes_; j++) {
			printf("plane[%d]: length = %d\n", j, (planes_buffer_ + j)->length);
			printf("plane[%d]: offset = %d\n", j, (planes_buffer_ + j)->m.mem_offset);
			(plane_start_ + j)->start = mmap (NULL /* start anywhere */,
									(planes_buffer_ + j)->length,
									PROT_READ | PROT_WRITE /* required */,
									MAP_SHARED /* recommended */,
									fd_,
									(planes_buffer_ + j)->m.mem_offset);
			if (MAP_FAILED == (plane_start_ +j)->start) {
				printf ("mmap failed\n");
				req_.count = i;
				for (i = 0; i < req_.count; i++) {
                    for (j = 0; j < num_planes_; j++) {
                        if (MAP_FAILED != ((buffers_ + i)->plane_start + j)->start) {
                            if (-1 == munmap(((buffers_ + i)->plane_start + j)->start, ((buffers_ + i)->planes_buffer + j)->length))
                                printf ("munmap error\n");
                        }
                    }
        	    }  
                return false; 
			}
		}
	}

    for (i = 0; i < req_.count; ++i) {
		memset(&buf_, 0, sizeof(buf_));

		buf_.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf_.memory      = V4L2_MEMORY_MMAP;
		buf_.length   	= num_planes_;
		buf_.index       = i;
		buf_.m.planes 	= (buffers_ + i)->planes_buffer;

		if (ioctl (fd_, VIDIOC_QBUF, &buf_) < 0)
			printf ("VIDIOC_QBUF failed\n");
	}

    return true;
}


bool VideoCapture::start()
{
    auto type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	if (ioctl(fd_, VIDIOC_STREAMON, &type) < 0)
    {
        printf ("VIDIOC_STREAMON failed\n");
        return false;
    }
		

    
	tmp_plane = (struct v4l2_plane *)calloc(num_planes_, sizeof(*tmp_plane));
    return true;
}

bool VideoCapture::stop()
{
    auto type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
 	if (ioctl(fd_, VIDIOC_STREAMOFF, &type) < 0)
    {
        printf("VIDIOC_STREAMOFF fail\n");
        return false;
    }
 		

    free(tmp_plane);
    tmp_plane = NULL;
    return true;
}

bool VideoCapture::isReadable(timeval *tv)
{
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(fd_, &fdset);
	return (select(fd_+1, &fdset, NULL, NULL, tv) == 1);
}


size_t VideoCapture::get_frame(char *buffer,size_t bufferSize)
{

    memset(&buf_, 0, sizeof(buf_));
    buf_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf_.memory = V4L2_MEMORY_MMAP;
    buf_.m.planes = tmp_plane;
    buf_.length = num_planes_;
    if (ioctl (fd_, VIDIOC_DQBUF, &buf_) < 0)
        printf("dqbuf fail\n");
    
    size_t size = 0;
    for (j = 0; j < num_planes_; j++) {       // printf("plane[%d] start = %p, bytesused = %d\n", j, ((buffers_ + buf_.index)->plane_start + j)->start, buf_.m.planes[j].bytesused);
        //fwrite(((buffers + buf.index)->plane_start + j)->start, (tmp_plane + j)->bytesused, 1, file_fd);
       // auto frameSize = rk_encoder->encode(((buffers + buf.index)->plane_start + j)->start,(tmp_plane + j)->bytesused,encodeData);
        memmove(buffer,((buffers_ + buf_.index)->plane_start + j)->start,(tmp_plane + j)->bytesused);
        size += (tmp_plane + j)->bytesused;
    }  

    if (ioctl (fd_, VIDIOC_QBUF, &buf_) < 0)
        printf("failture VIDIOC_QBUF\n");

    return size;
}
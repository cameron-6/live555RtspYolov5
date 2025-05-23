
#include "log.h"
#include "CamRTSPServer.h"
#include "VideoCapture.h"
#include "Yolov5.h"

int LogLevel;
static void announceStream(RTSPServer *rtspServer,ServerMediaSession *sms, const std::string &deviceName)
{
    auto url = rtspServer->rtspURL(sms);
    LOG(NOTICE, "Play the stream of the %s, url: %s", deviceName.c_str(), url);   
    delete[] url;
}

int main(int argc,char*argv[])
{
	initLogger(NOTICE);

    std::string model_file = argv[1];

    V4L2DeviceParameters param;
	param.buffer_cnt = 3;
	param.devName = "/dev/video0";
	param.width = 640;
	param.height = 480;
	param.fps = 30;


	try {
		CamRTSPServer server;
        server.addYolov5Instance(std::make_shared<Yolov5>(param,model_file));
		server.run();
	} catch (const std::invalid_argument &err) {
		LOG(ERROR, err.what());
	}
    // TaskScheduler *scheduler = BasicTaskScheduler::createNew();

    // UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);
    // UserAuthenticationDatabase *authorDB = NULL;

    // RTSPServer *rtspServer = RTSPServer::createNew(*env,8554,authorDB);
    // if (rtspServer == NULL)
    // {
    //     *env << "Failed to create RTSP server: "<<env->getResultMsg()<<"\n";
    //     exit(1);
    // }

    // char const *descriptionString = "Session streamed by \"H264OnDemandRTSPServer\"";
    // char const *streamName = "live";
    // ServerMediaSession *sms = ServerMediaSession::createNew(*env,streamName,streamName,descriptionString);

    // CamFrameSource *videoSource = 0;
    // sms->addSubsession(H264DemandSubsession::createNew(*env,(FramedSource*)videoSource));
    // rtspServer->addServerMediaSession(sms);
    
    // announceStream(rtspServer,sms,"/dev/video0");

    // env->taskScheduler().doEventLoop();

	
	return 0;

}

// int main(int argc, char **argv)
// {
// 	initLogger(NOTICE);
// 	int fd;
// 	fd_set fds;
// 	FILE *file_fd;
// 	struct timeval tv;
// 	int ret = -1, i, j, r;
// 	int num_planes;
// 	struct v4l2_capability cap;
// 	struct v4l2_format fmt;
// 	struct v4l2_requestbuffers req;
// 	struct v4l2_buffer buf;
// 	struct v4l2_plane* planes_buffer;
// 	struct plane_start* plane_start;
// 	struct buffer *buffers;
// 	enum v4l2_buf_type type;

// 	MppFrameFormat mpp_fmt;
//     mpp_fmt = MPP_FMT_YUV420SP;
// 	Encoder_Param_t encoder_param{
//         mpp_fmt,
//         640,
//         480,
//         0,
//         0,
//         0,
//         30,  //fps
//         23};
// 	RkEncoder *rk_encoder = new RkEncoder(encoder_param);
// 	rk_encoder->init();

// 	if (argc != 4) {
// 		printf("Usage: v4l2_test <device> <frame_num> <save_file>\n");
// 		printf("example: v4l2_test /dev/video0 10 test.yuv\n");
// 		return ret;
// 	}

// 	fd = open(argv[1], O_RDWR);

// 	if (fd < 0) {
// 		printf("open device: %s fail\n", argv[1]);
// 		//goto err;
// 	}

// 	file_fd = fopen(argv[3], "wb+");
// 	if (!file_fd) {
// 		printf("open save_file: %s fail\n", argv[3]);
// 		//goto err1;
// 	}

// 	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
// 		printf("Get video capability error!\n");
// 		//goto err1;
// 	}

// 	if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
// 		printf("Video device not support capture!\n");
// 		//goto err1;
// 	}

// 	printf("Support capture!\n");

// 	memset(&fmt, 0, sizeof(struct v4l2_format));
// 	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
// 	fmt.fmt.pix_mp.width = 640;
// 	fmt.fmt.pix_mp.height = 480;
// 	fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
// 	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;

// 	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
// 		printf("Set format fail\n");
// 		//goto err1;
// 	}

// 	printf("width = %d\n", fmt.fmt.pix_mp.width);
// 	printf("height = %d\n", fmt.fmt.pix_mp.height);
// 	printf("nmplane = %d\n", fmt.fmt.pix_mp.num_planes);
// 	printf("bufferSize = %d\n",fmt.fmt.pix_mp.plane_fmt[0].sizeimage);

// 	//memset(&fmt, 0, sizeof(struct v4l2_format));
// 	//fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
// 	//if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
// 	//	printf("Set format fail\n");
// 	//	goto err;
// 	//}

// 	//printf("nmplane = %d\n", fmt.fmt.pix_mp.num_planes);

// 	req.count = 5;
// 	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
// 	req.memory = V4L2_MEMORY_MMAP;
// 	if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
// 		printf("Reqbufs fail\n");
// 		//goto err1;
// 	}

// 	printf("buffer number: %d\n", req.count);

// 	num_planes = fmt.fmt.pix_mp.num_planes;

// 	buffers = (struct buffer*)malloc(req.count * sizeof(*buffers));

// 	for(i = 0; i < req.count; i++) {
// 		memset(&buf, 0, sizeof(buf));
// 		planes_buffer = (v4l2_plane*)calloc(num_planes, sizeof(*planes_buffer));
// 		plane_start = (struct plane_start*)calloc(num_planes, sizeof(*plane_start));
// 		memset(planes_buffer, 0, sizeof(*planes_buffer));
// 		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
// 		buf.memory = V4L2_MEMORY_MMAP;
// 		buf.m.planes = planes_buffer;
// 		buf.length = num_planes;
// 		buf.index = i;
// 		if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf)) {
// 			printf("Querybuf fail\n");
// 			req.count = i;
// 			//goto err2;
// 		}

// 		(buffers + i)->planes_buffer = planes_buffer;
// 		(buffers + i)->plane_start = plane_start;
// 		for(j = 0; j < num_planes; j++) {
// 			printf("plane[%d]: length = %d\n", j, (planes_buffer + j)->length);
// 			printf("plane[%d]: offset = %d\n", j, (planes_buffer + j)->m.mem_offset);
// 			(plane_start + j)->start = mmap (NULL /* start anywhere */,
// 									(planes_buffer + j)->length,
// 									PROT_READ | PROT_WRITE /* required */,
// 									MAP_SHARED /* recommended */,
// 									fd,
// 									(planes_buffer + j)->m.mem_offset);
// 			if (MAP_FAILED == (plane_start +j)->start) {
// 				printf ("mmap failed\n");
// 				req.count = i;
// 				//goto unmmap;
// 			}
// 		}
// 	}

// 	for (i = 0; i < req.count; ++i) {
// 		memset(&buf, 0, sizeof(buf));

// 		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
// 		buf.memory      = V4L2_MEMORY_MMAP;
// 		buf.length   	= num_planes;
// 		buf.index       = i;
// 		buf.m.planes 	= (buffers + i)->planes_buffer;

// 		if (ioctl (fd, VIDIOC_QBUF, &buf) < 0)
// 			printf ("VIDIOC_QBUF failed\n");
// 	}

// 	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

// 	if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
// 		printf ("VIDIOC_STREAMON failed\n");

// 	int num = 0;
// 	struct v4l2_plane *tmp_plane;
// 	tmp_plane = (struct v4l2_plane *)calloc(num_planes, sizeof(*tmp_plane));

// 	while (1)
// 	{
// 		FD_ZERO (&fds);
// 		FD_SET(fd, &fds);
// 		tv.tv_sec = 5;
// 		tv.tv_usec = 0;

// 		r = select (fd + 1, &fds, NULL, NULL, &tv);

// 		if (-1 == r)
// 		{
// 			if (EINTR == errno)
// 				continue;
// 			printf ("select err\n");
// 		}
// 		if (0 == r)
// 		{
// 			fprintf (stderr, "select timeout\n");
// 			exit (EXIT_FAILURE);
// 		}

// 		memset(&buf, 0, sizeof(buf));
// 		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
// 		buf.memory = V4L2_MEMORY_MMAP;
// 		buf.m.planes = tmp_plane;
// 		buf.length = num_planes;
// 		if (ioctl (fd, VIDIOC_DQBUF, &buf) < 0)
// 			printf("dqbuf fail\n");
		
// 		for (j = 0; j < num_planes; j++) {
// 			uint8_t* encodeData = (uint8_t *)malloc((tmp_plane + j)->bytesused);
// 			printf("plane[%d] start = %p, bytesused = %d\n", j, ((buffers + buf.index)->plane_start + j)->start, buf.m.planes[j].bytesused);
// 			//fwrite(((buffers + buf.index)->plane_start + j)->start, (tmp_plane + j)->bytesused, 1, file_fd);
// 			auto frameSize = rk_encoder->encode(((buffers + buf.index)->plane_start + j)->start,(tmp_plane + j)->bytesused,encodeData);
// 			appendH264FrameToFile("output.h264", encodeData, frameSize);
// 		}

// 		num++;
// 		if(num >= atoi(argv[2]))
// 			break;

// 		if (ioctl (fd, VIDIOC_QBUF, &buf) < 0)
// 			printf("failture VIDIOC_QBUF\n");
// 	}

// 	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
// 	if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
// 		printf("VIDIOC_STREAMOFF fail\n");

// 	free(tmp_plane);

// 	ret = 0;

// // unmmap:
// // err2:
// // 	for (i = 0; i < req.count; i++) {
// // 		for (j = 0; j < num_planes; j++) {
// // 			if (MAP_FAILED != ((buffers + i)->plane_start + j)->start) {
// // 				if (-1 == munmap(((buffers + i)->plane_start + j)->start, ((buffers + i)->planes_buffer + j)->length))
// // 					printf ("munmap error\n");
// // 			}
// // 		}
// // 	}

// // 	for (i = 0; i < req.count; i++) {
// // 		free((buffers + i)->planes_buffer);
// // 		free((buffers + i)->plane_start);
// // 	}

// // 	free(buffers);

// // 	fclose(file_fd);
// // err1:
// // 	close(fd);
// // err:
// 	return ret;
// }


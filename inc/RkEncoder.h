/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-08 13:47:30
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-08 14:02:37
 * @FilePath: /v4l2_mplane/inc/RkEncoder.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef RK_ENCODER_H
#define RK_ENCODER_H

#include <bits/stdint-uintn.h>
#include <rockchip/rk_mpi.h>
#include <rockchip/rk_mpi_cmd.h>
#include <rockchip/rk_type.h>
#include <rockchip/mpp_buffer.h>
#include <rockchip/mpp_frame.h>

typedef struct
{
	MppFrameFormat fmt;
	int width;
	int height;
	int hor_stride;
	int ver_stride;
	int frame_size;
	int fps;
	int fix_qp;
} Encoder_Param_t;

class RkEncoder
{
public:
	RkEncoder(Encoder_Param_t param);
	~RkEncoder();

	int init();
	int encode(void *inbuf, int insize, uint8_t *outbuf);

	int startCode3(uint8_t *buf);
	int startCode4(uint8_t *buf);

private:
	Encoder_Param_t m_param;
	MppCtx m_contex;
	MppApi *m_mpi;
	MppPacket m_packet = nullptr;
	MppFrame m_frame = nullptr;
	MppBuffer m_buffer = nullptr;
};

#endif

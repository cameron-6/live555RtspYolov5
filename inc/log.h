/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-12-08 13:51:11
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-12-08 13:51:45
 * @FilePath: /libv4l2cc/v4l2_mplane/inc/log.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef LOG_H
#define LOG_H

#define LOG(level, fmt, ...) \
	log (level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define COL(x)  "\033[;" #x "m"
#define RED     COL(31)
#define GREEN   COL(32)
#define YELLOW  COL(33)
#define BLUE    COL(34)
#define MAGENTA COL(35)
#define CYAN    COL(36)
#define WHITE   COL(0)

typedef enum {
	EMERG = 0,
	FATAL,
	ALERT,
	CRIT,
	ERROR,
	WARN,
	NOTICE,
	INFO,
	DEBUG,
	NOTSET
} PriorityLevel;

extern int LogLevel;

void log (PriorityLevel level, const char* file, const int line, const char *fmt, ...);
void initLogger(PriorityLevel level);

#endif

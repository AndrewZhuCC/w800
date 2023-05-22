#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "uni_include.h"
#include <string.h>
#include "audio.h"

#define USER_DEBUG						1
#if USER_DEBUG
#define USER_PRINT uni_printf
#else
#define USER_PRINT(fmt, ...)
#endif
#define USER_FW_VER						"1.0.2"

//下面定义任务优先级和任务栈
#define TASK_PRIOR_MAIN					40
#define TASK_STACK_MAIN					1024
#define QUEUE_SIZE_MAIN					8

// 下面定义消息	
#define MSG_KEY_DOWN					0x01
#define MSG_KEY_UP						0x02
#define MSG_RECORD_START                0x03
#define MSG_RECORD_END                  0x04


// 下面定义堆分配
#define USER_MALLOC						tls_mem_alloc
#define USER_FREE						tls_mem_free

//#define USER_PA_PIN                     UNI_IO_PB_07
#define USER_PA_PIN                     UNI_IO_PB_03
#endif

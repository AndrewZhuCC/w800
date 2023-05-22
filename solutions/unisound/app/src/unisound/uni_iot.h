/**************************************************************************
 * Copyright (C) 2017-2017  Unisound
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : uni_iot.h
 * Author      : yuanshifeng@unisound.com
 * Date        : 2021.06.10
 *
 **************************************************************************/
#ifndef HAL_INC_UNI_IOT_H_
#define HAL_INC_UNI_IOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#include <aos/aos.h>

#include "uni_types.h"
#include "uni_log.h"

#define UNI_CROW_VERSION                "v0.0.1-$(BUILD_ID)-$(DATE)"

typedef enum {
  UNI_NO_ERR = 0,
  UNI_INVALID_STATE_ERR,
  UNI_OPEN_ERR,
  UNI_CLOSE_ERR,
  UNI_START_ERR,
  UNI_STOP_ERR,
  UNI_DEST_ERR,
  UNI_SUSP_ERR,
  UNI_THREAD_ERR,
  UNI_INVALID_HANDLE_ERR,
  UNI_INVALID_FILE_ERR,
  UNI_INVALID_SIZE_ERR,
  UNI_INVALID_FORMAT_ERR,
  UNI_INVALID_PARAM_ERR,
  UNI_NO_CMD_ERR,
  UNI_NO_MEM_ERR,
  UNI_NO_SUPPORT_ERR,
  UNI_MODULE_INIT_ERR,
  UNI_MODULE_UNINIT_ERR,
  UNI_MODULE_INIT_ALREADY_ERR,
  UNI_MODULE_BUSY_ERR,
  UNI_MODULE_NONE_ERR,
  UNI_INVALID_USERNAME_ERR,
  UNI_INVALID_PASSWORD_ERR,
  UNI_NO_PERMISION_ERR,
  UNI_GET_STATUS_ERR,
  UNI_SET_STATUS_ERR,
  UNI_ERR_NUM_END
} uni_err_t;

#define MEMBER_TO_STRUCT(member_ptr, \
                         type, member)    (type *)((char *)(member_ptr) - \
                                          (unsigned long)&((type *)0)->member)
#define UNI_ISSPACE(x)                    (((x) == 0x20) || ((x) > 0x8 && (x) < 0xe))
#define uni_assert                        assert
#define uni_max(a, b)                     ((a) < (b) ? (b) : (a))
#define uni_min(a, b)                     ((b) < (a) ? (b) : (a))
#define uni_abs(a, b)                     ((a) > (b) ? ((a)-(b)) : ((b)-(a)))
#define uni_printf                        printf
#define uni_sprintf                       sprintf
#define uni_snprintf                      snprintf


#define uni_malloc                        aos_malloc
#define uni_free                          aos_free
#define uni_realloc                       aos_realloc

#define uni_atoi                          atoi
#define uni_atol                          atol
#define uni_strtol                        strtol
#define uni_strtoll                       strtoll
#define uni_strtof                        strtof

#define uni_memset                        memset
#define uni_memcpy                        memcpy
#define uni_memmove                       memmove
#define uni_strcat                        strcat
#define uni_strncat                       strncat
#define uni_strstr                        strstr
#define uni_strrchr                       strrchr
#define uni_strchr                        strchr
#define uni_strlen                        strlen
#define uni_strcpy                        strcpy
#define uni_strncpy                       strncpy
#define uni_strncmp                       strncmp
#define uni_strcmp                        strcmp
#define uni_strcasecmp                    strcasecmp

#define uni_isspace                       isspace

#define uni_usleep(us)                    aos_msleep((us)/1001 + 1)
#define uni_msleep(ms)                    aos_msleep(ms)
#define uni_sleep(s)                      aos_msleep((s) * 1000)

#define SemaphoreHandle_t                 aos_sem_t
#define uni_sem_t                         SemaphoreHandle_t
#define uni_sem_init(sem_ptr, cnt)        aos_sem_new(sem_ptr, cnt)
#define uni_sem_post(sem_ptr)             aos_sem_signal(&sem_ptr)
#define uni_sem_wait(sem_ptr)             aos_sem_wait(&sem_ptr, AOS_WAIT_FOREVER)
#define uni_sem_wait_ms(sem_ptr, ms)      aos_sem_wait(&sem_ptr, ms)
#define uni_sem_destroy(sem_ptr)          aos_sem_free(&sem_ptr)

/* THREAD STACK SIZE */
#define STACK_SIZE                        256               // 1KB (freeRTOS unit is word - 4 bytes)
#define STACK_SMALL_SIZE                  (STACK_SIZE * 1)  // 1KB
#define STACK_NORMAL_SIZE                 (STACK_SIZE * 2)  // 2kB
#define STACK_MIDDLE_SIZE                 (STACK_SIZE * 3)  // 3kB
#define STACK_LARGE_SIZE                  (STACK_SIZE * 4)  // 4kB

/* TASK PRIORITY */
#define OS_PRIORITY_IDLE                  AOS_MAX_APP_PRI            //idle (lowest)
#define OS_PRIORITY_LOW                   (AOS_MAX_APP_PRI - 1)      //low
#define OS_PRIORITY_NORMAL                (AOS_MAX_APP_PRI - 10)      //normal
#define OS_PRIORITY_HIGH                  (AOS_MAX_APP_PRI - 20)      //high
#define OS_PRIORITY_REALTIME              (1)  //realtime (highest)

#define OS_PRIORITY_KWS        33
#define OS_PRIORITY_AUDIO      30

typedef struct  {
    uni_u32 stack_size;
    uni_u32 priority;
    char task_name[16];
}thread_param;

//#define TaskHandle_t                      aos_task_t
//typedef void (*start_routine)(void *);
//#define uni_pthread_t                     TaskHandle_t
//#define uni_pthread_create(thread, para, task_func, arg) aos_task_new_ext(thread, )
//#define uni_pthread_detach(th)            
//#define uni_pthread_destroy(th)           vTaskDelete(th)

#define uni_mutex_t                       aos_mutex_t
#define uni_pthread_mutex_init(mtx_ptr) \
                                          aos_mutex_new(mtx_ptr)
#define uni_pthread_mutex_destroy(mtx_ptr) \
                                          aos_mutex_free(mtx_ptr)
#define uni_pthread_mutex_lock(mtx_ptr)    \
                                          aos_mutex_lock(mtx_ptr, AOS_WAIT_FOREVER)
#define uni_pthread_mutex_unlock(mtx_ptr)  \
                                          aos_mutex_unlock(mtx_ptr)

static inline long long uni_get_clock_time_ms(void) {
  return aos_now_ms();
}

static inline long long uni_get_clock_time_sec(void) {
  return aos_now_ms() / 1000;
}

#ifdef __cplusplus
}
#endif
#endif

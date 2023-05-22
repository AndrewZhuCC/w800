/*
 * Copyright 2019 Unisound AI Technology Co., Ltd.
 * Author: Hao Peng
 * All Rights Reserved.
 */

#ifndef OSAL_OSAL_STRING_H_
#define OSAL_OSAL_STRING_H_

#include "osal/osal-types.h"

#if defined(_WIN32) || defined(_WIN64)
#define _WINDOWS
#endif

#ifndef _WINDOWS
#define OSAL_EXPORT __attribute__((visibility("default")))
#else
#ifdef DLL_EXPORT
#define OSAL_EXPORT __declspec(dllexport)
#else
#define OSAL_EXPORT __declspec(dllimport)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @Description: �?memset
 * @Input params: mem：内存首地址
 *                value：赋�? *                len：长�? * @Output params:
 * @Return:
 */
OSAL_EXPORT void* OsalMemset(void* mem, int32_t value, size_t len);

/*
 * @Description: �?memcpy
 * @Input params: dest: 目的内存首地址
 *                src: 源内存首地址
 *                n: 拷贝字节�? * @Output params: �? * @Return: 指向 dest 的指�? */
OSAL_EXPORT void* OsalMemcpy(void* dest, const void* src, size_t n);

/*
 * @Description: �?memmove
 * @Input params: dest: 目的内存首地址
 *                src: 源内存首地址
 *                n: 移动字节�? * @Output params: �? * @Return: 指向 dest 的指�? */
OSAL_EXPORT void* OsalMemmove(void* dest, const void* src, size_t n);

/*
 * @Description: �?strcpy
 * @Input params: dest: 目标内存首地址
 *                src: 源内存首地址
 * @Output params: �? * @Return: 指向 dest 的指�? */
OSAL_EXPORT char* OsalStrcpy(char* dest, const char* src);

/*
 * @Description: �?strncpy
 * @Input params: dest: 目标内存首地址
 *                src: 源内存首地址
 *                n: 字节�? * @Output params: �? * @Return: 指向 dest 的指�? */
OSAL_EXPORT char* OsalStrncpy(char* dest, const char* src, size_t n);

/*
 * @Description: �?strstr
 * @Input params: haystack: 被检索字符串
 *                needle: 被匹配的字符�? * @Output params: �? * @Return: 成功：子串首地址
 *          失败：NULL
 */
OSAL_EXPORT char* OsalStrstr(const char* haystack, const char* needle);

/*
 * @Description: �?strchr
 * @Input params: str: 被检索字符串
 *                c：被匹配字符
 * @Output params: �? * @Return: 成功：匹配到 c 的位置的指针
 *          失败：NULL
 */
OSAL_EXPORT char* OsalStrchr(const char* str, int32_t c);

/*
 * @Description: �?strrchr
 * @Input params: str: 被检索字符串
 *                c：被匹配字符
 * @Output params: �? * @Return: 成功：匹配到 c 的位置的指针
 *          失败：NULL
 */
OSAL_EXPORT char* OsalStrrchr(const char* str, int32_t c);

/*
 * @Description: �?strlen
 * @Input params: str: 字符�? * @Output params: �? * @Return: 字符串长�? */
OSAL_EXPORT size_t OsalStrlen(const char* str);

/*
 * @Description: �?strcmp
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT int32_t OsalStrcmp(const char* s1, const char* s2);

/*
 * @Description: �?strncmp
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT int32_t OsalStrncmp(const char* s1, const char* s2, size_t n);

/*
 * @Description: �?memcmp
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT int32_t OsalMemcmp(const void* dest, const void* src, size_t n);

/*
 * @Description: �?strcat
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT char* OsalStrcat(char* dest, const char* src);

/*
 * @Description: �?strncat
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT char* OsalStrncat(char* dest, const char* src, size_t n);

/*
 * @Description: �?strtok
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT char* OsalStrtok(char* str, const char* delim);

/*
 * @Description: �?strtok_r
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT char* OsalStrtok_r(char* str, const char* delim, char** saveptr);

#ifdef __cplusplus
}
#endif

#endif  // OSAL_OSAL_STRING_H_

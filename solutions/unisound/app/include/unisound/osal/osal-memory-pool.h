/*
 * Copyright 2019 Unisound AI Technology Co., Ltd.
 * Author: Hao Peng
 * All Rights Reserved.
 */

#ifndef OSAL_OSAL_MEMORY_POOL_H_
#define OSAL_OSAL_MEMORY_POOL_H_

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

typedef void* OsalMemoryPoolHandle;

/*
 * @Description: 创建内存池
 * @Input params: size: 内存池大小，含内存池管理节点开销
 * @Output params:
 * @Return: 内存池 handle
 */
OSAL_EXPORT OsalMemoryPoolHandle OsalMemoryPoolCreate(size_t size);

/*
 * @Description: 分配内存
 * @Input params: handle: 内存池 handle
 *                size: 大小，超出内存池可用内存大小时会分配新内存
 * @Output params:
 * @Return: 内存地址
 */
OSAL_EXPORT void* OsalMemoryPoolAlloc(OsalMemoryPoolHandle handle, size_t size);

/*
 * @Description: 释放内存
 * @Input params: handle: 内存池 handle
 *                ptr: 内存地址
 * @Output params:
 * @Return:
 */
OSAL_EXPORT void OsalMemoryPoolDealloc(OsalMemoryPoolHandle handle, void* ptr);

/*
 * @Description: 内存池销毁
 * @Input params: handle: 内存池 handle
 * @Output params:
 * @Return:
 */
OSAL_EXPORT void OsalMemoryPoolDestroy(OsalMemoryPoolHandle handle);

/*
 * @Description: 打印内存池使用情况
 * @Input params: handle: 内存池 handle
 * @Output params:
 * @Return:
 */
OSAL_EXPORT void OsalMemoryPoolStatus(OsalMemoryPoolHandle handle);

/*
 * @Description: 获取内存池已经使用大小
 * @Input params: handle: 内存池 handle
 * @Output params:
 * @Return: used size
 */
OSAL_EXPORT size_t OsalMemoryPoolUsedSize(OsalMemoryPoolHandle handle);

#ifdef __cplusplus
}
#endif

#endif  // OSAL_OSAL_MEMORY_POOL_H_

/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <errno.h>
#include <string.h>
#include "yoc/partition.h"
#include "yoc/partition_flash.h"
#include "mtb_log.h"

#if defined(CONFIG_KERNEL_NONE)
#warning "Please implement flash operation interface."
void *part_flash_open(int id)
{
    (void)id;
    return NULL;
}

int part_flash_close(void *handle)
{
    (void)handle;
    return 0;
}

int part_flash_info_get(void *handle, partition_flash_info_t *info)
{
    return 0;
}

int part_flash_read(void *handle, uint32_t addr, void *data, size_t data_len)
{
    (void)handle;
    memcpy(data, (void *)addr, data_len);

    return 0;
}

int part_flash_write(void *handle, uint32_t addr, void *data, size_t data_len)
{
    (void)handle;

    return 0;
}

int part_flash_erase(void *handle, uint32_t addr, size_t len)
{
    (void)handle;

    return 0;
}

#else

#include <devices/flash.h>

void *part_flash_open(int id)
{
    void *flash_dev;

    flash_dev = (void *)flash_open_id("eflash", id);

    return flash_dev;
}

int part_flash_close(void *handle)
{
    if (handle) {
        return flash_close(handle);
    }
    return -EINVAL;
}

int part_flash_info_get(void *handle, partition_flash_info_t *info)
{
    flash_dev_info_t flash_info;

    if (handle && info) {
        if (flash_get_info((aos_dev_t *)handle, &flash_info) != 0) {
            return -1;
        }
        info->start_addr = flash_info.start_addr;
        info->sector_size = flash_info.block_size;
        info->sector_count = flash_info.block_count;
        MTB_LOGD("info->start_addr:0x%x", info->start_addr);
        MTB_LOGD("info->sector_size:0x%x", info->sector_size);
        MTB_LOGD("info->sector_count:0x%x", info->sector_count);
        return 0;
    }
    return -EINVAL;
}

int part_flash_read(void *handle, uint32_t addr, void *data, size_t data_len)
{
    if (handle && data && data_len > 0) {
        flash_dev_info_t flash_info;

        if (flash_get_info((aos_dev_t *)handle, &flash_info) != 0) {
            return -1;
        }
        if (addr < flash_info.start_addr) {
            return -EINVAL;
        }
        return flash_read(handle, addr - flash_info.start_addr, data, data_len);
    }
    return -EINVAL;
}

int part_flash_write(void *handle, uint32_t addr, void *data, size_t data_len)
{
    if (handle && data && data_len > 0) {
        flash_dev_info_t flash_info;

        if (flash_get_info((aos_dev_t *)handle, &flash_info) != 0) {
            return -1;
        }
        if (addr < flash_info.start_addr) {
            return -EINVAL;
        }
        return flash_program(handle, addr - flash_info.start_addr, data, data_len);
    }
    return -EINVAL;
}

int part_flash_erase(void *handle, uint32_t addr, size_t len)
{
    if (handle && len > 0) {
        flash_dev_info_t flash_info;

        if (flash_get_info((aos_dev_t *)handle, &flash_info) != 0) {
            return -1;
        }
        if (addr < flash_info.start_addr) {
            return -EINVAL;
        }
        return flash_erase(handle, addr - flash_info.start_addr, (len + flash_info.block_size - 1) / flash_info.block_size);
    }
    return -EINVAL;
}

#endif /* CONFIG_KERNEL_NONE */

static const partition_flash_ops_t default_flash_ops = {
    .hdl_mgr.index = 0,
    .open     = part_flash_open,
    .close    = part_flash_close,
    .info_get = part_flash_info_get,
    .read     = part_flash_read,
    .write    = part_flash_write,
    .erase    = part_flash_erase
};
static int g_init_default = 0;
static partition_flash_ops_t g_flash_ops[CONFIG_FLASH_NUM];

static void _get_handle_id(hdl_mgr_t *hdl_mgr, void **out_handle, uint32_t *id)
{
    *out_handle = NULL;
    *id = 0;
    if (hdl_mgr) {
        *id = hdl_mgr->index;
        *out_handle = hdl_mgr->handle;
    }
}

void partition_flash_register(partition_flash_ops_t *ops)
{
    // init default
    if (g_init_default != 1) {
        memcpy(&g_flash_ops[0], &default_flash_ops, sizeof(partition_flash_ops_t));
        g_init_default = 1;
    }

    // register new
    if (ops != NULL) {
        if (ops->hdl_mgr.index < CONFIG_FLASH_NUM)
            memcpy(&g_flash_ops[ops->hdl_mgr.index], ops, sizeof(partition_flash_ops_t));
    }
}

void partition_flash_register_default(void)
{
    // init default
    if (g_init_default != 1) {
        memcpy(&g_flash_ops[0], &default_flash_ops, sizeof(partition_flash_ops_t));
        g_init_default = 1;
    }
}

void *partition_flash_open(int id)
{
    if (id > CONFIG_FLASH_NUM - 1) {
        return NULL;
    }
    if (g_flash_ops[id].open) {
        void *handle = g_flash_ops[id].open(id);
        g_flash_ops[id].hdl_mgr.index = id;
        g_flash_ops[id].hdl_mgr.handle = handle;
        return &g_flash_ops[id].hdl_mgr;
    }
    return NULL;
}

int partition_flash_close(void *handle)
{
    void *hd;
    uint32_t id;

    _get_handle_id((hdl_mgr_t *)handle, &hd, &id);
    if (g_flash_ops[id].close) {
        return g_flash_ops[id].close(hd);
    }
    return -1;
}

int partition_flash_info_get(void *handle, partition_flash_info_t *info)
{
    void *hd;
    uint32_t id;

    _get_handle_id((hdl_mgr_t *)handle, &hd, &id);
    if (g_flash_ops[id].info_get) {
        return g_flash_ops[id].info_get(hd, info);
    }
    return -1;
}

int partition_flash_read(void *handle, uint32_t addr, void *data, size_t data_len)
{
    int ret;
    void *hd;
    uint32_t id;

    _get_handle_id((hdl_mgr_t *)handle, &hd, &id);
    ret = -1;
    if (g_flash_ops[id].read) {
        ret = g_flash_ops[id].read(hd, addr, data, data_len);
    }
    return ret;
}

int partition_flash_write(void *handle, uint32_t addr, void *data, size_t data_len)
{
    void *hd;
    uint32_t id;

    _get_handle_id((hdl_mgr_t *)handle, &hd, &id);
    if (g_flash_ops[id].write)
        return g_flash_ops[id].write(hd, addr, data, data_len);
    return -1;
}

int partition_flash_erase(void *handle, uint32_t addr, size_t len)
{
    void *hd;
    uint32_t id;

    _get_handle_id((hdl_mgr_t *)handle, &hd, &id);
    if (g_flash_ops[id].erase)
        return g_flash_ops[id].erase(hd, addr, len);
    return -1;
}
/*
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <aos/debug.h>
#include <ulog/ulog.h>
#include <yoc/partition_flash.h>
#include <yoc/partition.h>
#include <devices/flash.h>

#define TAG "extfls"

static void *extflash_open(int id)
{
    void *flash_dev;

    flash_dev = (void *)flash_open_id("spiflash", id);
    // LOGD(TAG, "extflash_open ret %p", flash_dev);

    return flash_dev;
}

static int extflash_close(void *handle)
{
    if (handle) {
        return flash_close(handle);
    }
    return -EINVAL;
}

static int extflash_info_get(void *handle, partition_flash_info_t *info)
{
    flash_dev_info_t flash_info;

    if (handle && info) {
        if (flash_get_info((aos_dev_t *)handle, &flash_info) != 0) {
            return -1;
        }
        info->start_addr = flash_info.start_addr;
        info->sector_size = flash_info.block_size;
        info->sector_count = flash_info.block_count;
        // LOGD(TAG, "info->start_addr:0x%x", info->start_addr);
        // LOGD(TAG, "info->sector_size:0x%x", info->sector_size);
        // LOGD(TAG, "info->sector_count:0x%x", info->sector_count);
        return 0;   
    }
    return -EINVAL;
}

static int extflash_read(void *handle, uint32_t addr, void *flash_test_data, size_t data_len)
{
    // LOGD(TAG, "read addr:0x%x, len %d", addr, data_len);

    if (handle && flash_test_data && data_len > 0) {
        flash_dev_info_t flash_info;

        if (flash_get_info((aos_dev_t *)handle, &flash_info) != 0) {
            return -1;
        }
        if (addr < flash_info.start_addr) {
            return -EINVAL;
        }
        return flash_read(handle, addr - flash_info.start_addr, flash_test_data, data_len);
    }
    return -EINVAL;
}

static int extflash_write(void *handle, uint32_t addr, void *flash_test_data, size_t data_len)
{
    // LOGD(TAG, "write addr:0x%x, len %d", addr, data_len);

    if (handle && flash_test_data && data_len > 0) {
        flash_dev_info_t flash_info;

        if (flash_get_info((aos_dev_t *)handle, &flash_info) != 0) {
            return -1;
        }
        if (addr < flash_info.start_addr) {
            return -EINVAL;
        }
        return flash_program(handle, addr - flash_info.start_addr, flash_test_data, data_len);
    }
    return -EINVAL;
}

static int extflash_erase(void *handle, uint32_t addr, size_t len)
{
    // LOGD(TAG, "erase addr:0x%x, len %d", addr, len);

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

static const partition_flash_ops_t extern_flash_ops = {
    .hdl_mgr.index = 1,
    .open     = extflash_open,
    .close    = extflash_close,
    .info_get = extflash_info_get,
    .read     = extflash_read,
    .write    = extflash_write,
    .erase    = extflash_erase
};

static int extflash_hdl;

int app_extflash_register(void)
{
    partition_flash_register((partition_flash_ops_t *)&extern_flash_ops);

    return 0;
}

int app_extflash_init(void)
{
    extflash_hdl = partition_open("user");
    aos_check_return_einval(extflash_hdl >= 0);    
}

int app_extflash_read(uint32_t addr, void *data, uint32_t size)
{
    LOGD(TAG, "extflash read %u %p %u", addr, data, size);
    return partition_read(extflash_hdl, addr, data, size);
}

int app_extflash_write(uint32_t addr, void *data, uint32_t size)
{
    LOGD(TAG, "extflash write %u %p %u", addr, data, size);
    return partition_write(extflash_hdl, addr, data, size);
}

void app_extflash_deinit(void)
{
    partition_close(extflash_hdl);
}

#define FLASH_RW_TEST_BLOCK_SIZE  256
uint8_t flash_test_data[FLASH_RW_TEST_BLOCK_SIZE];
uint8_t flash_test_rdata[FLASH_RW_TEST_BLOCK_SIZE];
int app_extflash_test()
{
    LOGD(TAG, "to open user");

    for (int i = 0; i < FLASH_RW_TEST_BLOCK_SIZE; i++) {
        flash_test_data[i] = i;
    }

    partition_t hdl = partition_open("user");
    aos_check_return_einval(hdl);

    memset(flash_test_rdata, 0, FLASH_RW_TEST_BLOCK_SIZE);
    partition_read(hdl, 0, flash_test_rdata, FLASH_RW_TEST_BLOCK_SIZE);
    printf("extflash read:\n");
    for(int i = 0; i < FLASH_RW_TEST_BLOCK_SIZE; i++) {
        printf("%02x ", flash_test_rdata[i]);
    }
    printf("\n");

    partition_erase(hdl, 0, 1);
    partition_write(hdl, 0, flash_test_data, FLASH_RW_TEST_BLOCK_SIZE);
    partition_read(hdl, 0, flash_test_rdata, FLASH_RW_TEST_BLOCK_SIZE);

    if (memcmp(flash_test_data, flash_test_rdata, FLASH_RW_TEST_BLOCK_SIZE) == 0) {
        LOGD(TAG, "flash read write test success");
    } else {
        LOGD(TAG, "flash read write test error");
        while(1);
    }

    partition_close(hdl);
    return 0;
}

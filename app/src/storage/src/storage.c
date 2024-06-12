#include "storage.h"

#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include <string.h>

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/"DISK_DRIVE_NAME":"

LOG_MODULE_REGISTER(storage, CONFIG_APP_LOG_LEVEL);

static const char *disk_mount_pt = DISK_MOUNT_PT;

static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

int storage_init()
{
    int ret;

    ret = disk_access_init(DISK_DRIVE_NAME);
    if (ret != 0)
    {
        LOG_ERR("Disk access init error %d", ret);
        return ret;
    }

    mp.mnt_point = disk_mount_pt;

    ret = fs_mount(&mp);

    if (ret != FR_OK)
    {
        LOG_ERR("FS mount error %d", ret);
        return ret;
    }
    else
    {
        LOG_DBG("FS mounted");
    }

    return 0;
}

int storage_write_line(const char *p_line, const char *p_filename)
{
    int ret;
    int idx = strlen(disk_mount_pt);
    char p_file_path[STORAGE_MAX_FILENAME_LEN];
    struct fs_file_t file_desc;

    if (strlen(p_line) > STORAGE_MAX_LINE_LEN)
    {
        LOG_ERR("Line is too long");
        return -1;
    }

    fs_file_t_init(&file_desc);

    strncpy(p_file_path, disk_mount_pt, STORAGE_MAX_FILENAME_LEN);
    p_file_path[idx++] = '/';
    strncat(&p_file_path[idx], p_filename, STORAGE_MAX_FILENAME_LEN - strlen(disk_mount_pt) - 2);

    ret = fs_open(&file_desc, p_file_path, FS_O_WRITE | FS_O_CREATE | FS_O_APPEND);
    if (ret < 0)
    {
		LOG_ERR("File open err %d", ret);
        return ret;
	}

    ret = fs_write(&file_desc, p_line, strlen(p_line));
    if (ret < 0)
	{
		LOG_ERR("Write err %d", ret);
	}
    else
    {
        LOG_DBG("Write successfull (%d bytes)", ret);
    }

    fs_close(&file_desc);

    return 0;
}

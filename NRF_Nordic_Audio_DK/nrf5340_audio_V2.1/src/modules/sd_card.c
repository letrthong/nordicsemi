/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sd_card.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(sd_card, CONFIG_LOG_SD_CARD_LEVEL);
LOG_MODULE_REGISTER(sd_card, CONFIG_LOG_MAIN_LEVEL);

#define SD_ROOT_PATH "/SD:/"
#define PATH_MAX_LEN 260 /* Maximum length for path support by Windows file system*/

static const char *sd_root_path = "/SD:";
static FATFS fat_fs;
static bool sd_init_success;

static struct fs_mount_t mnt_pt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

int sd_card_list_files(char *path)
{
	int ret;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;

	if (!sd_init_success) {
		return -ENODEV;
	}

	LOG_DBG("sd_card_list_files\n");
	fs_dir_t_init(&dirp);

	if (path == NULL) {
		ret = fs_opendir(&dirp, sd_root_path);
		if (ret) {
			LOG_ERR("Open SD card root dir failed");
			return ret;
		}
	} else {
		if (strlen(path) > CONFIG_FS_FATFS_MAX_LFN) {
			LOG_ERR("Path is too long");
			return -FR_INVALID_NAME;
		}

		strcat(abs_path_name, path);

		ret = fs_opendir(&dirp, abs_path_name);
		if (ret) {
			LOG_ERR("Open assigned path failed");
			return ret;
		}
	}
	while (true) {
		ret = fs_readdir(&dirp, &entry);
		if (ret) {
			return ret;
		}

		if (entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("[DIR ] %s", entry.name);
		} else {
			LOG_INF("[FILE] %s", entry.name);
		}
	}

	ret = fs_closedir(&dirp);
	if (ret) {
		LOG_ERR("Close SD card root dir failed");
		return ret;
	}

	return 0;
}

int sd_card_write(char const *const filename, char const *const data, size_t *size)
{
	struct fs_file_t f_entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;
	int ret;

	if (!sd_init_success) {
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		return -FR_INVALID_NAME;
	}

	strcat(abs_path_name, filename);
	fs_file_t_init(&f_entry);

	ret = fs_open(&f_entry, abs_path_name, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
	if (ret) {
		LOG_ERR("Create file failed");
		return ret;
	}

	/* If the file exists, moves the file position pointer to the end of the file */
	ret = fs_seek(&f_entry, 0, FS_SEEK_END);
	if (ret) {
		LOG_ERR("Seek file pointer failed");
		return ret;
	}

	ret = fs_write(&f_entry, data, *size);
	if (ret < 0) {
		LOG_ERR("Write file failed");
		return ret;
	}

	*size = ret;

	ret = fs_close(&f_entry);
	if (ret) {
		LOG_ERR("Close file failed");
		return ret;
	}

	return 0;
}

int sd_card_read(char const *const filename, char *const data, size_t *size)
{
	struct fs_file_t f_entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;
	int ret;

	if (!sd_init_success) {
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		return -FR_INVALID_NAME;
	}

	strcat(abs_path_name, filename);
	fs_file_t_init(&f_entry);

	ret = fs_open(&f_entry, abs_path_name, FS_O_READ);
	if (ret) {
		LOG_ERR("Open file failed");
		return ret;
	}

	ret = fs_read(&f_entry, data, *size);
	if (ret < 0) {
		LOG_ERR("Read file failed");
		return ret;
	}

	*size = ret;
	if (*size == 0) {
		LOG_WRN("File is empty");
	}

	ret = fs_close(&f_entry);
	if (ret) {
		LOG_ERR("Close file failed");
		return ret;
	}

	return 0;
}

int sd_card_init(void)
{
	int ret;
	static const char *sd_dev = "SD";
	uint64_t sd_card_size_bytes;
	uint32_t sector_count;
	size_t sector_size;

	LOG_DBG("sd_card_init\n");
	ret = disk_access_init(sd_dev);
	if (ret) {
		LOG_INF("SD card init failed, please check if SD card inserted");
		return -ENODEV;
	}

	ret = disk_access_ioctl(sd_dev, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	if (ret) {
		LOG_INF("SD card Unable to get sector count");
		return ret;
	}
	LOG_INF("SD card Sector count: %d", sector_count);

	ret = disk_access_ioctl(sd_dev, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
	if (ret) {
		LOG_INF("SD card Unable to get sector size");
		return ret;
	}
	LOG_INF("SD card Sector size: %d bytes", sector_size);

	sd_card_size_bytes = (uint64_t)sector_count * sector_size;
	LOG_INF("SD card volume size: %d MB", (uint32_t)(sd_card_size_bytes >> 20));

	mnt_pt.mnt_point = sd_root_path;
	ret = fs_mount(&mnt_pt);
	if (ret) {
		LOG_INF("SD card Mnt. disk failed, could be format issue. should be FAT/exFAT");
		return ret;
	}

	sd_init_success = true;

	return 0;
}


int  count_total_files(char *path ){
	int ret = 0;
	int count = 0;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;

	if (!sd_init_success) {
		return -ENODEV;
	}

	LOG_DBG("sd_card_list_files\n");
	fs_dir_t_init(&dirp);

	if (path == NULL) {
		ret = fs_opendir(&dirp, sd_root_path);
		if (ret) {
			LOG_ERR("Open SD card root dir failed");
			return count;
		}
	} else {
		if (strlen(path) > CONFIG_FS_FATFS_MAX_LFN) {
			LOG_ERR("Path is too long");
			//return -FR_INVALID_NAME;
			return count;
		}

		strcat(abs_path_name, path);

		ret = fs_opendir(&dirp, abs_path_name);
		if (ret) {
			LOG_ERR("Open assigned path failed");
			return count;
		}
	}
	while (true) {
		ret = fs_readdir(&dirp, &entry);
		if (ret) {
			return count;
		}

		if (entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("[DIR ] %s", entry.name);
		} else {
			LOG_INF("[FILE] %s", entry.name);
			count =  count + 1;
		}
	}

	ret = fs_closedir(&dirp);
	if (ret) {
		LOG_ERR("Close SD card root dir failed");
		return count;
	}

	return 0;
}

int delete_file(char const *const filename){
	int ret =  sd_card_write(filename, '', 0);
	if(ret == 0){
		 
		if (!sd_init_success) {
			return -ENODEV;
		}

		ret =  fs_rename(filename, '.delete');
	}

	return ret;
}

 
int  get_block_of_file(char const *const filename, size_t* size_of_block){
	struct fs_file_t f_entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;
	int ret;

	if (!sd_init_success) {
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		return -FR_INVALID_NAME;
	}

	strcat(abs_path_name, filename);
	fs_file_t_init(&f_entry);

	ret = fs_open(&f_entry, abs_path_name, FS_O_READ);
	if (ret) {
		LOG_ERR("Open file failed");
		return ret;
	}
     
	int total_bytes = 0;
	int size = 512;
	char buffer[size] ={};
	while(true){
		ret = fs_read(&f_entry, buffer,  size);
		
		LOG_DBG("get_block_of_files size=%d\n", ret);
		if (ret < 0) {
			LOG_ERR("Read file failed");
			break;
		}

		if (ret == 0 ) {
			LOG_ERR("End file");
			break;
		}

		total_bytes =  = total_bytes + ret;

	}
	
	int blocks = (total_bytes/size);
	LOG_DBG("get_block_of_files block=%d\n", blocks);
	ret = fs_close(&f_entry);
	if (ret) {
		LOG_ERR("Close file failed");
		return ret;
	}

	return blocks;
}


int read_block_of_file(char const *const filename, char *const data, size_t *size, size_t size_of_block, int block){
	int ret = 0;
	return ret;
}

int write_block_of_file(char const *const filename, char *const data, size_t *size){
	int ret = 0;
	return ret;
}

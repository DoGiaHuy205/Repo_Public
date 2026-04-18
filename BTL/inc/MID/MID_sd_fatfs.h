#ifndef MID_SD_FATFS_H
#define MID_SD_FATFS_H

#include "stm32f10x.h"
#include "ff.h"

// Ma trang thai tra ve
#define SD_WAV_OK           0U
#define SD_WAV_ERR_SD       1U
#define SD_WAV_ERR_MOUNT    2U
#define SD_WAV_ERR_OPEN     3U
#define SD_WAV_ERR_WRITE    4U
#define SD_WAV_ERR_SEEK     5U

// Khoi tao SD card, mount FAT, tao file WAV moi
uint8_t SD_WAV_Init(const char *filename);
// Tra ve SD_WAV_OK hoac ma loi

// Ghi mot block du lieu PCM 16-bit vao file
uint8_t SD_WAV_WriteBlock(const int16_t *pcm_data, uint32_t count);
// Tra ve SD_WAV_OK hoac SD_WAV_ERR_WRITE

// Cap nhat WAV header voi kich thuoc chinh xac, dong file, unmount
uint8_t SD_WAV_Finalize(void);
// Tra ve SD_WAV_OK hoac ma loi

// Lay tong so byte PCM da ghi (khong ke header)
uint32_t SD_WAV_GetDataBytes(void);

#endif

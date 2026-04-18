#ifndef FF_H
#define FF_H

#include "ffconf.h"
#include <stdint.h>

typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;
typedef uint64_t  QWORD;
typedef uint32_t  LBA_t;
typedef uint32_t  FSIZE_t;
typedef unsigned  UINT;

#define FA_READ             0x01U
#define FA_WRITE            0x02U
#define FA_OPEN_EXISTING    0x00U
#define FA_CREATE_NEW       0x04U
#define FA_CREATE_ALWAYS    0x08U
#define FA_OPEN_ALWAYS      0x10U
#define FA_OPEN_APPEND      0x30U

#define FS_FAT12    1
#define FS_FAT16    2
#define FS_FAT32    3

#define AM_RDO  0x01U
#define AM_HID  0x02U
#define AM_SYS  0x04U
#define AM_DIR  0x10U
#define AM_ARC  0x20U

typedef enum {
    FR_OK = 0,
    FR_DISK_ERR,
    FR_INT_ERR,
    FR_NOT_READY,
    FR_NO_FILE,
    FR_NO_PATH,
    FR_INVALID_NAME,
    FR_DENIED,
    FR_EXIST,
    FR_INVALID_OBJECT,
    FR_WRITE_PROTECTED,
    FR_INVALID_DRIVE,
    FR_NOT_ENABLED,
    FR_NO_FILESYSTEM,
    FR_MKFS_ABORTED,
    FR_TIMEOUT,
    FR_LOCKED,
    FR_NOT_ENOUGH_CORE,
    FR_TOO_MANY_OPEN_FILES,
    FR_INVALID_PARAMETER,
} FRESULT;

typedef struct {
    BYTE    fs_type;
    BYTE    pdrv;
    BYTE    n_fats;
    BYTE    wflag;
    WORD    ssize;
    WORD    csize;
    DWORD   n_rootdir;
    DWORD   n_fatent;
    DWORD   fatbase;
    DWORD   dirbase;
    DWORD   database;
    DWORD   last_clst;
    DWORD   free_clst;
    DWORD   winsect;
    BYTE    win[512];
} FATFS;

typedef struct {
    FATFS   *fs;
    BYTE    flag;
    BYTE    err;
    FSIZE_t fptr;
    FSIZE_t obj_size;
    DWORD   sclust;
    DWORD   clust;
    DWORD   sect;
    DWORD   dir_sect;
    BYTE    *dir_ptr;
    DWORD   buf_sect;
    BYTE    buf[512];
} FIL;

FRESULT f_mount(FATFS *fs, const char *path, BYTE opt);
FRESULT f_open(FIL *fp, const char *path, BYTE mode);
FRESULT f_close(FIL *fp);
FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw);
FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br);
FRESULT f_lseek(FIL *fp, FSIZE_t ofs);
FRESULT f_sync(FIL *fp);

#endif

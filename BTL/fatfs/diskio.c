#include "diskio.h"
#include "IP_sd_spi.h"

static DSTATUS drive_status = STA_NOINIT;

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return drive_status;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;

    if (SD_SPI_Init() == SD_OK) {
        drive_status = 0;
    } else {
        drive_status = STA_NOINIT | STA_NODISK;
    }
    return drive_status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    UINT i;
    if (pdrv != 0 || (drive_status & STA_NOINIT)) return RES_NOTRDY;
    if (count == 0) return RES_PARERR;

    for (i = 0; i < count; i++) {
        if (SD_ReadBlock((uint32_t)(sector + i), buff) != SD_OK) {
            return RES_ERROR;
        }
        buff += 512;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    UINT i;
    if (pdrv != 0 || (drive_status & STA_NOINIT)) return RES_NOTRDY;
    if (count == 0) return RES_PARERR;

    for (i = 0; i < count; i++) {
        if (SD_WriteBlock((uint32_t)(sector + i), buff) != SD_OK) {
            return RES_ERROR;
        }
        buff += 512;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0 || (drive_status & STA_NOINIT)) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void)
{
    return ((DWORD)(2024 - 1980) << 25)
         | ((DWORD)1             << 21)
         | ((DWORD)1             << 16)
         | ((DWORD)0             << 11)
         | ((DWORD)0             << 5)
         | ((DWORD)0             >> 1);
}

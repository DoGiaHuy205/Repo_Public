#include "ff.h"
#include "diskio.h"
#include <string.h>

#define BPB_BytsPerSec      11
#define BPB_SecPerClus      13
#define BPB_RsvdSecCnt      14
#define BPB_NumFATs         16
#define BPB_RootEntCnt      17
#define BPB_TotSec16        19
#define BPB_FATSz16         22
#define BPB_TotSec32        32
#define BPB_FATSz32         36
#define BPB_ExtFlags        40
#define BPB_RootClus        44
#define BPB_FSInfo          48
#define BS_55AA             510

#define DIR_Name            0
#define DIR_Attr            11
#define DIR_CrtTimeTenth    13
#define DIR_CrtTime         14
#define DIR_CrtDate         16
#define DIR_LstAccDate      18
#define DIR_FstClusHI       20
#define DIR_WrtTime         22
#define DIR_WrtDate         24
#define DIR_FstClusLO       26
#define DIR_FileSize        28

#define FAT32_EOC           0x0FFFFFFFUL
#define FAT32_BAD           0x0FFFFFF7UL
#define FAT32_FREE          0x00000000UL

#define FA__WRITTEN         0x20U
#define FA__DIRTY           0x40U

#define ld_word(ptr)        ((WORD)(*(BYTE*)(ptr)) | ((WORD)*((BYTE*)(ptr)+1) << 8))
#define ld_dword(ptr)       ((DWORD)(*(BYTE*)(ptr)) | ((DWORD)*((BYTE*)(ptr)+1)<<8) \
                             | ((DWORD)*((BYTE*)(ptr)+2)<<16) | ((DWORD)*((BYTE*)(ptr)+3)<<24))
#define st_word(ptr, val)   { *(BYTE*)(ptr)=(BYTE)(val); *((BYTE*)(ptr)+1)=(BYTE)((val)>>8); }
#define st_dword(ptr, val)  { *(BYTE*)(ptr)=(BYTE)(val); *((BYTE*)(ptr)+1)=(BYTE)((val)>>8); \
                              *((BYTE*)(ptr)+2)=(BYTE)((val)>>16); *((BYTE*)(ptr)+3)=(BYTE)((val)>>24); }

static FATFS *g_fs = NULL;

static DRESULT disk_rd(FATFS *fs, BYTE *buf, DWORD sect, UINT cnt)
{
    return disk_read(fs->pdrv, buf, (LBA_t)sect, cnt);
}

static DRESULT disk_wr(FATFS *fs, const BYTE *buf, DWORD sect, UINT cnt)
{
    return disk_write(fs->pdrv, buf, (LBA_t)sect, cnt);
}

static FRESULT move_window(FATFS *fs, DWORD sect)
{
    if (sect == fs->winsect) return FR_OK;

    if (fs->wflag) {
        if (disk_wr(fs, fs->win, fs->winsect, 1) != RES_OK) return FR_DISK_ERR;
        fs->wflag = 0;
    }

    if (disk_rd(fs, fs->win, sect, 1) != RES_OK) return FR_DISK_ERR;
    fs->winsect = sect;
    return FR_OK;
}

static FRESULT sync_window(FATFS *fs)
{
    if (fs->wflag) {
        if (disk_wr(fs, fs->win, fs->winsect, 1) != RES_OK) return FR_DISK_ERR;
        fs->wflag = 0;
    }
    return FR_OK;
}

static DWORD get_fat(FATFS *fs, DWORD clst)
{
    DWORD val;
    DWORD sect;

    if (clst < 2 || clst >= fs->n_fatent) return 1;

    sect = fs->fatbase + clst / 128U;
    if (move_window(fs, sect) != FR_OK) return 1;

    val = ld_dword(fs->win + (clst % 128U) * 4U) & 0x0FFFFFFFUL;
    return val;
}

static FRESULT put_fat(FATFS *fs, DWORD clst, DWORD val)
{
    DWORD sect;
    BYTE  *p;

    if (clst < 2 || clst >= fs->n_fatent) return FR_INT_ERR;

    sect = fs->fatbase + clst / 128U;
    if (move_window(fs, sect) != FR_OK) return FR_DISK_ERR;

    p = fs->win + (clst % 128U) * 4U;
    val = (ld_dword(p) & 0xF0000000UL) | (val & 0x0FFFFFFFUL);
    st_dword(p, val);
    fs->wflag = 1;
    return FR_OK;
}

static DWORD clst2sect(FATFS *fs, DWORD clst)
{
    clst -= 2;
    if (clst >= (fs->n_fatent - 2)) return 0;
    return fs->database + (DWORD)fs->csize * clst;
}

static DWORD create_chain(FATFS *fs, DWORD clst)
{
    DWORD cs;
    DWORD ncl;

    ncl = fs->last_clst;
    if (ncl < 2 || ncl >= fs->n_fatent) ncl = 2;

    cs = ncl;
    do {
        cs++;
        if (cs >= fs->n_fatent) cs = 2;
        if (cs == ncl) return 0xFFFFFFFFUL;
    } while (get_fat(fs, cs) != FAT32_FREE);

    if (put_fat(fs, cs, FAT32_EOC) != FR_OK) return 0xFFFFFFFFUL;

    if (clst != 0) {
        if (put_fat(fs, clst, cs) != FR_OK) return 0xFFFFFFFFUL;
    }

    fs->last_clst = cs;
    if (fs->free_clst > 0) fs->free_clst--;

    return cs;
}

static FRESULT make_sfn(const char *path, BYTE *sfn)
{
    UINT i, j;
    char c;

    memset(sfn, 0x20, 11);

    i = 0;
    j = 0;
    while (path[i] && path[i] != '.') {
        c = path[i++];
        if (c >= 'a' && c <= 'z') c -= 0x20;
        if (j < 8) sfn[j++] = (BYTE)c;
    }

    if (path[i] == '.') {
        i++;
        j = 8;
        while (path[i] && j < 11) {
            c = path[i++];
            if (c >= 'a' && c <= 'z') c -= 0x20;
            sfn[j++] = (BYTE)c;
        }
    }

    if (sfn[0] == 0x20) return FR_INVALID_NAME;
    return FR_OK;
}

static FRESULT locate_or_create_direntry(FIL *fp, const BYTE *sfn)
{
    FATFS  *fs = fp->fs;
    DWORD   sect;
    DWORD   clst;
    UINT    i;
    BYTE   *de;
    DWORD   free_sect = 0;
    UINT    free_off  = 0;
    BYTE    found_free = 0;

    clst = fs->dirbase;
    while (1) {
        sect = clst2sect(fs, clst);
        if (sect == 0) return FR_INT_ERR;

        for (i = 0; i < fs->csize; i++) {
            if (move_window(fs, sect + i) != FR_OK) return FR_DISK_ERR;

            for (UINT e = 0; e < 16U; e++) {
                de = fs->win + e * 32U;

                if (de[DIR_Name] == 0xE5U || de[DIR_Name] == 0x00U) {
                    if (!found_free) {
                        free_sect  = sect + i;
                        free_off   = e * 32U;
                        found_free = 1;
                    }
                    if (de[DIR_Name] == 0x00U) goto done_scan;
                } else if (!(de[DIR_Attr] & AM_DIR)) {
                    if (memcmp(de + DIR_Name, sfn, 11) == 0) {
                        fp->dir_sect = sect + i;
                        fp->dir_ptr  = de;
                        return FR_OK;
                    }
                }
            }
        }

        clst = get_fat(fs, clst);
        if (clst >= FAT32_BAD) break;
    }

done_scan:
    if (!found_free) return FR_DENIED;

    if (move_window(fs, free_sect) != FR_OK) return FR_DISK_ERR;
    de = fs->win + free_off;
    memset(de, 0, 32);
    memcpy(de + DIR_Name, sfn, 11);
    de[DIR_Attr]    = AM_ARC;
    st_word(de + DIR_CrtTime, 0);
    st_word(de + DIR_CrtDate, 0x5341);
    st_word(de + DIR_WrtTime, 0);
    st_word(de + DIR_WrtDate, 0x5341);
    st_word(de + DIR_LstAccDate, 0x5341);
    fs->wflag = 1;

    fp->dir_sect = free_sect;
    fp->dir_ptr  = de;
    return FR_OK;
}

static FRESULT sync_fs(FIL *fp)
{
    FATFS  *fs = fp->fs;
    FRESULT res;

    if (fp->flag & FA__DIRTY) {
        if (disk_wr(fs, fp->buf, fp->buf_sect, 1) != RES_OK) return FR_DISK_ERR;
        fp->flag &= (BYTE)~FA__DIRTY;
    }

    res = sync_window(fs);
    if (res != FR_OK) return res;

    res = move_window(fs, fp->dir_sect);
    if (res != FR_OK) return res;

    st_dword(fp->dir_ptr + DIR_FileSize,  (DWORD)fp->obj_size);
    st_word (fp->dir_ptr + DIR_FstClusHI, (WORD)(fp->sclust >> 16));
    st_word (fp->dir_ptr + DIR_FstClusLO, (WORD)(fp->sclust));
    st_word (fp->dir_ptr + DIR_WrtTime,   0);
    st_word (fp->dir_ptr + DIR_WrtDate,   0x5341);
    fs->wflag = 1;

    return sync_window(fs);
}

FRESULT f_mount(FATFS *fs, const char *path, BYTE opt)
{
    BYTE   boot[512];
    DWORD  bsect;
    DWORD  fasize;
    DWORD  tsect;
    DWORD  sysect;
    DWORD  nclst;

    (void)path;

    if (!fs) {
        g_fs = NULL;
        return FR_OK;
    }

    memset(fs, 0, sizeof(FATFS));
    fs->pdrv = 0;

    if (disk_initialize(fs->pdrv) & STA_NOINIT) return FR_NOT_READY;

    if (disk_rd(fs, boot, 0, 1) != RES_OK) return FR_DISK_ERR;

    if (ld_word(boot + BS_55AA) != 0xAA55U) return FR_NO_FILESYSTEM;

    if (boot[446 + 4] == 0) {
        bsect = 0;
    } else {
        bsect = ld_dword(boot + 446 + 8);
        if (disk_rd(fs, boot, bsect, 1) != RES_OK) return FR_DISK_ERR;
        if (ld_word(boot + BS_55AA) != 0xAA55U) return FR_NO_FILESYSTEM;
    }

    if (ld_word(boot + BPB_RootEntCnt) != 0) return FR_NO_FILESYSTEM;
    if (ld_word(boot + BPB_FATSz16)    != 0) return FR_NO_FILESYSTEM;

    fs->ssize  = 512;
    fs->csize  = boot[BPB_SecPerClus];
    fs->n_fats = boot[BPB_NumFATs];

    fasize = ld_dword(boot + BPB_FATSz32);
    tsect  = ld_dword(boot + BPB_TotSec32);
    sysect = bsect + ld_word(boot + BPB_RsvdSecCnt);

    fs->fatbase  = sysect;
    fs->dirbase  = ld_dword(boot + BPB_RootClus);
    fs->database = sysect + fs->n_fats * fasize;

    nclst = (tsect - (fs->database - bsect)) / fs->csize;
    fs->n_fatent = nclst + 2;

    if (nclst < 65525U) return FR_NO_FILESYSTEM;

    fs->fs_type     = FS_FAT32;
    fs->last_clst   = fs->dirbase + 1;
    fs->free_clst   = 0xFFFFFFFFUL;
    fs->winsect     = 0xFFFFFFFFUL;
    fs->wflag       = 0;

    g_fs = fs;
    return FR_OK;
}

FRESULT f_open(FIL *fp, const char *path, BYTE mode)
{
    FRESULT  res;
    FATFS   *fs = g_fs;
    BYTE     sfn[11];
    DWORD    clst;

    if (!fs) return FR_NOT_ENABLED;

    memset(fp, 0, sizeof(FIL));
    fp->fs       = fs;
    fp->buf_sect = 0xFFFFFFFFUL;

    res = make_sfn(path, sfn);
    if (res != FR_OK) return res;

    if (mode & FA_READ) {
        DWORD  sect;
        DWORD  dir_clst;
        UINT   i;
        BYTE  *de;
        uint8_t found = 0;

        dir_clst = fs->dirbase;
        while (!found) {
            sect = clst2sect(fs, dir_clst);
            if (sect == 0) return FR_NO_FILE;

            for (i = 0; i < fs->csize && !found; i++) {
                if (move_window(fs, sect + i) != FR_OK) return FR_DISK_ERR;
                for (UINT e = 0; e < 16U && !found; e++) {
                    de = fs->win + e * 32U;
                    if (de[DIR_Name] == 0x00U) return FR_NO_FILE;
                    if (de[DIR_Name] == 0xE5U) continue;
                    if (de[DIR_Attr] & AM_DIR) continue;
                    if (memcmp(de + DIR_Name, sfn, 11) == 0) {
                        fp->dir_sect = sect + i;
                        fp->dir_ptr  = de;
                        found = 1;
                    }
                }
            }
            if (!found) {
                dir_clst = get_fat(fs, dir_clst);
                if (dir_clst >= FAT32_BAD) return FR_NO_FILE;
            }
        }

        clst = ((DWORD)ld_word(fp->dir_ptr + DIR_FstClusHI) << 16)
              | ld_word(fp->dir_ptr + DIR_FstClusLO);
        fp->sclust   = clst;
        fp->obj_size = ld_dword(fp->dir_ptr + DIR_FileSize);
        fp->clust    = clst;
        fp->sect     = clst2sect(fs, clst);
        fp->fptr     = 0;
        fp->flag     = FA_READ;
        return FR_OK;
    }

    if (!(mode & FA_WRITE)) return FR_INVALID_PARAMETER;

    res = locate_or_create_direntry(fp, sfn);
    if (res != FR_OK) return res;

    if (mode & FA_CREATE_ALWAYS) {
        clst = ((DWORD)ld_word(fp->dir_ptr + DIR_FstClusHI) << 16)
              | ld_word(fp->dir_ptr + DIR_FstClusLO);
        if (clst >= 2) {
            DWORD c = clst;
            DWORD nc;
            while (c < FAT32_BAD && c >= 2) {
                nc = get_fat(fs, c);
                put_fat(fs, c, FAT32_FREE);
                c = nc;
            }
        }

        st_dword(fp->dir_ptr + DIR_FileSize,  0);
        st_word (fp->dir_ptr + DIR_FstClusHI, 0);
        st_word (fp->dir_ptr + DIR_FstClusLO, 0);
        fs->wflag    = 1;
        fp->sclust   = 0;
    } else {
        clst = ((DWORD)ld_word(fp->dir_ptr + DIR_FstClusHI) << 16)
              | ld_word(fp->dir_ptr + DIR_FstClusLO);
        fp->sclust   = clst;
        fp->obj_size = ld_dword(fp->dir_ptr + DIR_FileSize);
    }

    sync_window(fs);

    fp->clust = fp->sclust;
    fp->sect  = 0;
    fp->fptr  = 0;
    fp->flag  = mode;
    return FR_OK;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
    FRESULT      res;
    FATFS       *fs;
    DWORD        csect;
    const BYTE  *wbuf = (const BYTE *)buff;
    UINT         wcnt;

    *bw = 0;
    if (!fp || !fp->fs) return FR_INVALID_OBJECT;
    if (!(fp->flag & FA_WRITE)) return FR_DENIED;
    fs = fp->fs;

    while (btw > 0) {
        if (fp->fptr == 0 || (fp->fptr % (fs->csize * 512U)) == 0) {
            DWORD ncl = create_chain(fs, fp->clust);
            if (ncl == 0 || ncl == 0xFFFFFFFFUL) return FR_DISK_ERR;

            if (fp->sclust == 0) {
                fp->sclust = ncl;
                st_word(fp->dir_ptr + DIR_FstClusHI, (WORD)(ncl >> 16));
                st_word(fp->dir_ptr + DIR_FstClusLO, (WORD)(ncl));
                fs->wflag = 1;
            }
            fp->clust = ncl;
            fp->sect  = clst2sect(fs, ncl);
            if (fp->sect == 0) return FR_INT_ERR;
        } else if ((fp->fptr % 512U) == 0) {
            if (fp->flag & FA__DIRTY) {
                if (disk_wr(fs, fp->buf, fp->buf_sect, 1) != RES_OK) return FR_DISK_ERR;
                fp->flag &= (BYTE)~FA__DIRTY;
            }
            fp->sect++;
        }

        csect = (UINT)(fp->fptr % 512U);
        wcnt  = 512U - csect;
        if (wcnt > btw) wcnt = btw;

        if (fp->buf_sect != fp->sect) {
            if (fp->flag & FA__DIRTY) {
                if (disk_wr(fs, fp->buf, fp->buf_sect, 1) != RES_OK) return FR_DISK_ERR;
                fp->flag &= (BYTE)~FA__DIRTY;
            }
            if (csect != 0 || wcnt < 512U) {
                if (disk_rd(fs, fp->buf, fp->sect, 1) != RES_OK) {
                    memset(fp->buf, 0, 512);
                }
            }
            fp->buf_sect = fp->sect;
        }

        memcpy(fp->buf + csect, wbuf, wcnt);
        fp->flag |= FA__DIRTY;

        fp->fptr  += wcnt;
        wbuf      += wcnt;
        btw       -= wcnt;
        *bw       += wcnt;

        if (fp->fptr > fp->obj_size) {
            fp->obj_size = fp->fptr;
            fp->flag    |= FA__WRITTEN;
        }
    }

    if ((fp->fptr % 512U) == 0 && (fp->flag & FA__DIRTY)) {
        if (disk_wr(fs, fp->buf, fp->buf_sect, 1) != RES_OK) return FR_DISK_ERR;
        fp->flag &= (BYTE)~FA__DIRTY;
    }

    (void)res;
    return FR_OK;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
    FATFS *fs;
    DWORD  clst;

    if (!fp || !fp->fs) return FR_INVALID_OBJECT;
    fs = fp->fs;

    if (fp->flag & FA__DIRTY) {
        if (disk_wr(fs, fp->buf, fp->buf_sect, 1) != RES_OK) return FR_DISK_ERR;
        fp->flag &= (BYTE)~FA__DIRTY;
    }

    if (ofs == 0) {
        fp->fptr  = 0;
        fp->clust = fp->sclust;
        fp->sect  = clst2sect(fs, fp->sclust);
        fp->buf_sect = 0xFFFFFFFFUL;
        return FR_OK;
    }

    fp->fptr  = 0;
    fp->clust = fp->sclust;
    clst      = fp->sclust;

    while (fp->fptr + (fs->csize * 512U) <= ofs && clst < FAT32_BAD) {
        fp->fptr += fs->csize * 512U;
        clst = get_fat(fs, clst);
        if (clst < 2 || clst >= FAT32_BAD) return FR_INT_ERR;
        fp->clust = clst;
    }

    fp->sect     = clst2sect(fs, fp->clust) + (DWORD)((ofs - fp->fptr) / 512U);
    fp->fptr     = ofs;
    fp->buf_sect = 0xFFFFFFFFUL;
    return FR_OK;
}

FRESULT f_sync(FIL *fp)
{
    if (!fp || !fp->fs) return FR_INVALID_OBJECT;
    return sync_fs(fp);
}

FRESULT f_close(FIL *fp)
{
    FRESULT res;
    if (!fp || !fp->fs) return FR_INVALID_OBJECT;
    res = sync_fs(fp);
    fp->fs = NULL;
    return res;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
    FATFS *fs;
    UINT   rcnt;
    BYTE  *rbuf = (BYTE *)buff;

    *br = 0;
    if (!fp || !fp->fs) return FR_INVALID_OBJECT;
    fs = fp->fs;

    while (btr > 0) {
        UINT csect = (UINT)(fp->fptr % 512U);
        rcnt = 512U - csect;
        if (rcnt > btr) rcnt = btr;

        if (fp->buf_sect != fp->sect) {
            if (disk_rd(fs, fp->buf, fp->sect, 1) != RES_OK) return FR_DISK_ERR;
            fp->buf_sect = fp->sect;
        }

        memcpy(rbuf, fp->buf + csect, rcnt);
        fp->fptr += rcnt;
        rbuf     += rcnt;
        btr      -= rcnt;
        *br      += rcnt;

        if ((fp->fptr % 512U) == 0) {
            fp->sect++;
        }
    }
    return FR_OK;
}

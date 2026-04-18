#include "MID_sd_fatfs.h"
#include "IP_sd_spi.h"
#include "MID_oled_ssd1306.h"
#include <string.h>

// Bien noi bo
static FATFS  g_fatfs;
static FIL    g_file;
static uint32_t g_data_bytes;  // Tong so byte PCM da ghi
static uint8_t  g_is_open;     // Co hieu file dang mo

// Cau truc WAV header (ghi lan dau, cap nhat lan cuoi)
#pragma pack(1)
typedef struct {
    char     riff_id[4];
    uint32_t riff_size;
    char     wave_id[4];
    char     fmt_id[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data_id[4];
    uint32_t data_size;
} WAV_Hdr;
#pragma pack()

// build_wav_header - Tao WAV header voi kich thuoc du lieu dat truoc
static void build_wav_header(WAV_Hdr *hdr, uint32_t data_bytes)
{
    hdr->riff_id[0]='R'; hdr->riff_id[1]='I'; hdr->riff_id[2]='F'; hdr->riff_id[3]='F';
    hdr->riff_size     = data_bytes + 36U;
    hdr->wave_id[0]='W'; hdr->wave_id[1]='A'; hdr->wave_id[2]='V'; hdr->wave_id[3]='E';
    hdr->fmt_id[0]='f';  hdr->fmt_id[1]='m';  hdr->fmt_id[2]='t';  hdr->fmt_id[3]=' ';
    hdr->fmt_size      = 16U;
    hdr->audio_format  = 1U;    // PCM
    hdr->num_channels  = 1U;    // Mono
    hdr->sample_rate   = 8000U; // 8kHz
    hdr->bits_per_sample = 16U;
    hdr->byte_rate     = 8000U * 1U * 2U; // 16000 bytes/s
    hdr->block_align   = 1U * 2U;         // 2 bytes/sample
    hdr->data_id[0]='d'; hdr->data_id[1]='a'; hdr->data_id[2]='t'; hdr->data_id[3]='a';
    hdr->data_size     = data_bytes;
}

// SD_WAV_Init - Khoi tao SD, mount FAT, tao file WAV, ghi header tam (size=0)
uint8_t SD_WAV_Init(const char *filename)
{
    FRESULT fr;
    UINT    bw;
    WAV_Hdr hdr;

    g_data_bytes = 0;
    g_is_open    = 0;

    // Mount FAT (disk_initialize() duoc goi ben trong f_mount)
    fr = f_mount(&g_fatfs, "0:", 1);
    if (fr != FR_OK) {
        OLED_Clear();
        OLED_DrawString(0, 0, "SD ERR:MOUNT");
        OLED_UpdateScreen();
        return SD_WAV_ERR_MOUNT;
    }

    // Mo file, ghi de neu da ton tai
    fr = f_open(&g_file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        OLED_Clear();
        OLED_DrawString(0, 0, "SD ERR:OPEN");
        OLED_UpdateScreen();
        return SD_WAV_ERR_OPEN;
    }

    // Ghi WAV header tam voi data_size = 0 (dat cho, cap nhat sau)
    build_wav_header(&hdr, 0x00000000UL);
    fr = f_write(&g_file, &hdr, sizeof(hdr), &bw);
    if (fr != FR_OK || bw != sizeof(hdr)) {
        f_close(&g_file);
        return SD_WAV_ERR_WRITE;
    }

    g_is_open = 1;
    return SD_WAV_OK;
}

// SD_WAV_WriteBlock - Ghi mot block PCM 16-bit vao file tren SD
uint8_t SD_WAV_WriteBlock(const int16_t *pcm_data, uint32_t count)
{
    FRESULT fr;
    UINT    bw;
    uint32_t byte_count = count * 2U;

    if (!g_is_open) return SD_WAV_ERR_WRITE;

    fr = f_write(&g_file, pcm_data, (UINT)byte_count, &bw);
    if (fr != FR_OK || bw != (UINT)byte_count) {
        return SD_WAV_ERR_WRITE;
    }

    g_data_bytes += bw;
    return SD_WAV_OK;
}

// SD_WAV_Finalize - Cap nhat WAV header voi kich thuoc chinh xac, dong file, unmount
uint8_t SD_WAV_Finalize(void)
{
    FRESULT fr;
    UINT    bw;
    WAV_Hdr hdr;

    if (!g_is_open) return SD_WAV_ERR_WRITE;

    f_sync(&g_file);

    fr = f_lseek(&g_file, 0);
    if (fr != FR_OK) {
        f_close(&g_file);
        g_is_open = 0;
        return SD_WAV_ERR_SEEK;
    }

    build_wav_header(&hdr, g_data_bytes);
    fr = f_write(&g_file, &hdr, sizeof(hdr), &bw);
    if (fr != FR_OK) {
        f_close(&g_file);
        g_is_open = 0;
        return SD_WAV_ERR_WRITE;
    }

    f_close(&g_file);
    g_is_open = 0;

    f_mount(NULL, "0:", 0);

    return SD_WAV_OK;
}

// SD_WAV_GetDataBytes - Lay tong so byte PCM da ghi
uint32_t SD_WAV_GetDataBytes(void)
{
    return g_data_bytes;
}

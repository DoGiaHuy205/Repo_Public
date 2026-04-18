// VER: FINAL (refactored IP/MID/APP)
// design by DO GIA HUY, PHAM XUAN HIEU, NGUYEN MINH HIEU

#include "IP_SYSCLK.h"
#include "IP_USART1.h"
#include "IP_GPIO_BTN.h"
#include "IP_audio_capture.h"

#include "MID_oled_ssd1306.h"
#include "MID_sd_fatfs.h"
#include "MID_audio_filters.h"

#include "APP_main.h"

#include "ff.h"
#include "misc.h"
#include <stddef.h>

static uint16_t  display_buf[DISPLAY_BUF_SIZE];
static int16_t   pcm_block[AUDIO_BLOCK_SIZE];
static uint8_t   uart_chunk[UART_CHUNK_SIZE];
static uint32_t  total_samples;

// Hien thi dem nguoc tren OLED trong khi cho browser tai WAV
// Muc dich: Tranh ESP32 bi ket buffer UART khi browser dang tai file
static void APP_WaitWithCountdown(uint32_t ms, const char *label)
{
    char buf[12];
    uint32_t s = ms / 1000U;
    while (s > 0U) {
        uint8_t d = (uint8_t)(s / 10U);
        uint8_t u = (uint8_t)(s % 10U);
        OLED_Clear();
        OLED_DrawString(0, 0,  label);
        OLED_DrawString(0, 14, "Web dang tai");
        OLED_DrawString(0, 24, "Cho browser...");
        buf[0] = '0' + d; buf[1] = '0' + u;
        buf[2] = 's'; buf[3] = '\0';
        OLED_DrawString(40, 40, buf);
        OLED_UpdateScreen();
        IP_Delay_ms(1000);
        s--;
    }
}

// Cap nhat buffer hien thi OLED tu du lieu ADC DMA
// Lay mau cach deu de vua 128 diem ngang
static void APP_UpdateDisplayBuffer(volatile uint16_t *src, uint16_t len)
{
    uint16_t i;
    uint16_t step = len / DISPLAY_BUF_SIZE;
    if (step == 0) step = 1;
    for (i = 0; i < DISPLAY_BUF_SIZE; i++) {
        display_buf[i] = (i * step < len) ? src[i * step] : 2048U;
    }
}

// Chuyen ADC 12-bit sang PCM 16-bit roi ghi vao the SD
// PCM = (adc - 2048) << 4  (x16, scale len 16-bit)
static void APP_ProcessAndWriteBlock(volatile uint16_t *src, uint16_t len)
{
    uint16_t i;
    int32_t  v;
    for (i = 0; i < len; i++) {
        v = ((int32_t)src[i] - 2048) << 4;
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        pcm_block[i] = (int16_t)v;
        total_samples++;
    }
    SD_WAV_WriteBlock(pcm_block, len);
}

// Gui frame header qua UART
// Format: [0xAA][0xBB][0xCC][type][sz3][sz2][sz1][sz0]
static void APP_UART_SendFrameHeader(uint8_t type, uint32_t file_size)
{
    uint8_t hdr[8];
    hdr[0] = PROTO_START_0;
    hdr[1] = PROTO_START_1;
    hdr[2] = PROTO_START_2;
    hdr[3] = type;
    hdr[4] = (uint8_t)(file_size >> 24);
    hdr[5] = (uint8_t)(file_size >> 16);
    hdr[6] = (uint8_t)(file_size >> 8);
    hdr[7] = (uint8_t)(file_size);
    IP_USART1_SendBuf(hdr, 8U);
}

// Doc file WAV tu SD va gui nguyen ban qua UART (TYPE 0x01)
// Khong ap dung bo loc - du lieu goc
static void APP_ESP32_SendWAV_Raw(void)
{
    FATFS   fs;
    FIL     fil;
    FRESULT fr;
    UINT    br;
    uint32_t wav_size;
    uint32_t sent;

    OLED_Clear();
    OLED_DrawString(0, 0, "STEP 3A/4");
    OLED_DrawString(0, 9, "Send RAW->ESP32");
    OLED_UpdateScreen();

    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) return;

    fr = f_open(&fil, WAV_FILENAME, FA_READ | FA_OPEN_EXISTING);
    if (fr != FR_OK) { f_mount(NULL, "0:", 0); return; }

    wav_size = (uint32_t)fil.obj_size;

    // Gui header frame UART
    APP_UART_SendFrameHeader(PROTO_TYPE_RAW, wav_size);

    // Gui du lieu WAV tung chunk 512 byte
    sent = 0;
    while (sent < wav_size) {
        br = 0;
        f_read(&fil, uart_chunk, UART_CHUNK_SIZE, &br);
        if (br == 0) break;
        IP_USART1_SendBuf(uart_chunk, br);
        sent += br;
    }

    IP_USART1_WaitTC();
    f_close(&fil);
    f_mount(NULL, "0:", 0);
}

// Doc SD, ap dung bo loc so, roi gui qua UART
// filter_type  PROTO_TYPE_FIR hoac PROTO_TYPE_IIR
static void APP_ESP32_StreamFiltered(uint8_t filter_type)
{
    FATFS      fs;
    FIL        fil;
    FRESULT    fr;
    UINT       br;
    FIR_State  fir;
    IIR_State  iir;
    int16_t   *wav_chunk_i16;
    uint32_t   data_size;
    uint32_t   wav_total;
    uint32_t   sent;
    uint16_t   i;
    uint8_t    wav_header[44];
    const char *label;

    label = (filter_type == PROTO_TYPE_FIR) ? "FIR 1kHz LP" : "IIR 1kHz LP";
    OLED_Clear();
    OLED_DrawString(0, 0, (filter_type == PROTO_TYPE_FIR) ?
                          "STEP 3B/4" : "STEP 3C/4");
    OLED_DrawString(0, 9,  label);
    OLED_DrawString(0, 18, "Send->ESP32...");
    OLED_UpdateScreen();

    // Khoi tao bo loc (xoa trang thai cu)
    FIR_Init(&fir);
    IIR_Init(&iir);

    // Mount SD va mo file
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) return;

    fr = f_open(&fil, WAV_FILENAME, FA_READ | FA_OPEN_EXISTING);
    if (fr != FR_OK) { f_mount(NULL, "0:", 0); return; }

    wav_total = (uint32_t)fil.obj_size;
    data_size = (wav_total > 44U) ? (wav_total - 44U) : 0U;

    // Doc WAV header 44 byte (giu nguyen format)
    f_read(&fil, wav_header, 44U, &br);

    // Gui frame header UART: type + tong size bang raw size
    APP_UART_SendFrameHeader(filter_type, wav_total);

    // Gui WAV header goc (44 byte) de ESP32 co the parse format
    IP_USART1_SendBuf(wav_header, 44U);

    // Doc PCM, ap dung bo loc, gui
    sent = 0;
    while (sent < data_size) {
        uint32_t to_read = UART_CHUNK_SIZE;
        if ((data_size - sent) < to_read) {
            to_read = data_size - sent;
        }

        br = 0;
        fr = f_read(&fil, uart_chunk, (UINT)to_read, &br);
        if (fr != FR_OK || br == 0) break;

        // Ap dung bo loc cho tung mau (2 byte = 1 sample int16)
        wav_chunk_i16 = (int16_t *)uart_chunk;
        if (filter_type == PROTO_TYPE_FIR) {
            for (i = 0; i < (br / 2U); i++) {
                wav_chunk_i16[i] = FIR_Process(&fir, wav_chunk_i16[i]);
            }
        } else {
            for (i = 0; i < (br / 2U); i++) {
                wav_chunk_i16[i] = IIR_Process(&iir, wav_chunk_i16[i]);
            }
        }

        // Gui chunk da loc qua UART
        IP_USART1_SendBuf(uart_chunk, br);
        sent += br;
    }

    IP_USART1_WaitTC();
    f_close(&fil);
    f_mount(NULL, "0:", 0);
}

// MAIN - CHU TRINH UNG DUNG CHINH
int main(void)
{
    // Khoi tao he thong
    IP_SystemClock_Config();                  /* [IP] 72MHz HSE + PLL */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    IP_USART1_Init(USART1_BAUDRATE);          /* [IP] UART 115200 bps */
    IP_BTN_Init();                            /* [IP] PA1 nut bam */
    OLED_Init();                              /* [MID] OLED SSD1306 */
    Audio_Init();                             /* [IP]  ADC+DMA+TIM3 */

    // Man hinh chao
    OLED_Clear();
    OLED_DrawString(0, 0,  "AUDIO+FILTER");
    OLED_DrawString(0, 9,  "FIR+IIR Demo");
    OLED_DrawString(0, 20, "STM32->ESP32");
    OLED_UpdateScreen();
    IP_Delay_ms(1500);

    // VONG LAP CHINH
    while (1)
    {
        // B0: CHO NUT BAM
        OLED_Clear();
        OLED_DrawString(0, 0,  "[ READY ]");
        OLED_DrawString(0, 12, "Nhan PA1 de");
        OLED_DrawString(0, 22, "bat dau thu am");
        OLED_DrawString(0, 34, "BTN -> PA1-GND");
        OLED_UpdateScreen();

        IP_BTN_WaitPressRelease();            // [IP] Cho nhan + tha

        // B1: KHOI TAO SD VA GHI AM
        total_samples = 0;

        OLED_Clear();
        OLED_DrawString(0, 0, "STEP 1/4");
        OLED_DrawString(0, 9, "Init SD...");
        OLED_UpdateScreen();

        if (SD_WAV_Init(WAV_FILENAME) != SD_WAV_OK) { // [MID] Mount + tao file
            OLED_Clear();
            OLED_DrawString(0, 0, "SD INIT FAIL!");
            OLED_UpdateScreen();
            IP_Delay_ms(3000);
            continue;
        }

        OLED_Clear();
        OLED_DrawString(0, 0, "STEP 2/4");
        OLED_DrawString(0, 9, "REC 8kHz 16bit");
        OLED_UpdateScreen();

        Audio_StartRecord();                  // [IP] Bat ADC+DMA+TIM3

        // Vong lap DMA double-buffer: xu ly tung nua buffer
        while (audio_state != AUDIO_STATE_DONE)
        {
            if (audio_half_cplt) {
                audio_half_cplt = 0;
                APP_UpdateDisplayBuffer(&audio_buffer[0], AUDIO_BLOCK_SIZE);
                APP_ProcessAndWriteBlock(&audio_buffer[0], AUDIO_BLOCK_SIZE);
                OLED_Clear();
                OLED_DrawString(0, 0, "REC->SD: OK");
                OLED_DrawWaveform(display_buf, DISPLAY_BUF_SIZE); // [MID]
            }
            if (audio_full_cplt) {
                audio_full_cplt = 0;
                APP_UpdateDisplayBuffer(&audio_buffer[AUDIO_BLOCK_SIZE], AUDIO_BLOCK_SIZE);
                APP_ProcessAndWriteBlock(&audio_buffer[AUDIO_BLOCK_SIZE], AUDIO_BLOCK_SIZE);
                OLED_Clear();
                OLED_DrawString(0, 0, "REC->SD: OK");
                OLED_DrawWaveform(display_buf, DISPLAY_BUF_SIZE); // [MID]
            }
        }

        // B2: FINALIZE FILE WAV TREN SD
        OLED_Clear();
        OLED_DrawString(0, 0, "STEP 2B/4");
        OLED_DrawString(0, 9, "Finalize WAV...");
        OLED_UpdateScreen();

        if (SD_WAV_Finalize() != SD_WAV_OK) { // [MID] Cap nhat header + dong file
            IP_Delay_ms(2000);
            continue;
        }

        // B3A: GUI RAW WAV (TYPE 0x01) QUA UART
        APP_ESP32_SendWAV_Raw();

        // Cho browser tai xong RAW truoc khi gui FIR
        // (Tranh ESP32 bi ket buffer khi dang serve /audio_raw)
        APP_WaitWithCountdown(WAIT_BTW_STREAMS_MS, "RAW sent! Wait");

        // B3B: FIR FILTERED (TYPE 0x02)
        APP_ESP32_StreamFiltered(PROTO_TYPE_FIR);

        APP_WaitWithCountdown(WAIT_BTW_STREAMS_MS, "FIR sent! Wait");

        // B3C: IIR FILTERED (TYPE 0x03)
        APP_ESP32_StreamFiltered(PROTO_TYPE_IIR);

        // B4: HOAN THANH TAT CA 3 STREAM
        OLED_Clear();
        OLED_DrawString(0, 0,  "ALL DONE!");
        OLED_DrawString(0, 10, "RAW+FIR+IIR");
        OLED_DrawString(0, 20, "Check web tabs");
        OLED_DrawString(0, 32, "Nhan BTN tiep");
        OLED_UpdateScreen();

        // Reset trang thai de chuan bi chu ky moi
        audio_state = AUDIO_STATE_IDLE;
    }
}

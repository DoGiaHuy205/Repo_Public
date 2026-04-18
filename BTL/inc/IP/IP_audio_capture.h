#ifndef IP_AUDIO_CAPTURE_H
#define IP_AUDIO_CAPTURE_H

#include "stm32f10x.h"

// GPIO va ADC
#define AUDIO_ADC               ADC1
#define AUDIO_ADC_CLK           RCC_APB2Periph_ADC1
#define AUDIO_GPIO_CLK          RCC_APB2Periph_GPIOA
#define AUDIO_GPIO_PORT         GPIOA
#define AUDIO_GPIO_PIN          GPIO_Pin_0  // PA0 = ADC1_IN0
#define AUDIO_ADC_CHANNEL       ADC_Channel_0

// DMA
#define AUDIO_DMA               DMA1
#define AUDIO_DMA_CHANNEL       DMA1_Channel1
#define AUDIO_DMA_CLK           RCC_AHBPeriph_DMA1
#define AUDIO_DMA_IRQn          DMA1_Channel1_IRQn
#define AUDIO_DMA_IRQHandler    DMA1_Channel1_IRQHandler

// Timer
#define AUDIO_TIM               TIM3
#define AUDIO_TIM_CLK           RCC_APB1Periph_TIM3
#define AUDIO_SAMPLE_RATE       8000U
#define AUDIO_TIM_PRESCALER     0U
#define AUDIO_TIM_PERIOD        (72000000UL / AUDIO_SAMPLE_RATE - 1U)

// Buffer
#define AUDIO_BLOCK_SIZE        512U
#define AUDIO_BUFFER_SIZE       (2U * AUDIO_BLOCK_SIZE)
#define MAX_RECORD_BLOCKS       (3U * AUDIO_SAMPLE_RATE / AUDIO_BLOCK_SIZE)

// Trang thai he thong thu am
typedef enum {
    AUDIO_STATE_IDLE = 0,    // Trang thai cho
    AUDIO_STATE_RECORDING,   // Dang thu am
    AUDIO_STATE_BLOCK_READY, // Mot nua buffer da day
    AUDIO_STATE_DONE         // Hoan thanh ghi am
} AudioState_t;

// WAV Header (44 byte, chuan RIFF PCM)
#pragma pack(1)
typedef struct {
    char     riff_id[4];
    uint32_t riff_size;
    char     wave_id[4];
    char     fmt_id[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;      // 8000 Hz
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;  // 16
    char     data_id[4];
    uint32_t data_size;        // So byte du lieu ghi am
} WAV_Header_t;
#pragma pack()

// Bien toan cuc (dinh nghia trong .c)
extern volatile uint16_t audio_buffer[AUDIO_BUFFER_SIZE]; // Bo dem DMA vong
extern volatile AudioState_t audio_state;                  // Trang thai MUA
extern volatile uint8_t  audio_half_cplt;                  // 1/2 buffer 1 full
extern volatile uint8_t  audio_full_cplt;                  // 1/2 buffer 2 full
extern volatile uint32_t audio_block_count;                // So block da thu

// Prototype ham API
void Audio_Init(void);
void Audio_StartRecord(void);
void Audio_StopRecord(void);
WAV_Header_t Audio_BuildWAVHeader(uint32_t num_samples);

#endif

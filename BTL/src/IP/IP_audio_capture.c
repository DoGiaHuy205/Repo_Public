#include "IP_audio_capture.h"
#include <string.h>

// Khai bao truoc cho ISR - tranh warning cua ARMClang V6
void DMA1_Channel1_IRQHandler(void);

// Bo dem DMA vong 1024 mau (uint16_t)
volatile uint16_t audio_buffer[AUDIO_BUFFER_SIZE];

// Trang thai he thong
volatile AudioState_t audio_state  = AUDIO_STATE_IDLE;

// Co bao hieu nua buffer san sang xu ly
volatile uint8_t  audio_half_cplt  = 0;  // Set trong DMA HT ISR
volatile uint8_t  audio_full_cplt  = 0;  // Set trong DMA TC ISR

// So block da ghi xong
volatile uint32_t audio_block_count = 0;

// Audio_GPIO_Init - Cau hinh PA0 la Analog Input (ADC1_IN0)

static void Audio_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio_init;

    // Bat clock GPIOA
    RCC_APB2PeriphClockCmd(AUDIO_GPIO_CLK, ENABLE);

    // PA0 -> ADC1_IN0: Analog Input (GPIO_Mode_AIN = 0x00)
    gpio_init.GPIO_Pin  = AUDIO_GPIO_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(AUDIO_GPIO_PORT, &gpio_init);
}

// Thanh ghi TIM3:
static void Audio_TIM_Init(void)
{
    TIM_TimeBaseInitTypeDef tim_init;

    // Bat clock TIM3 (APB1)
    RCC_APB1PeriphClockCmd(AUDIO_TIM_CLK, ENABLE);

    // Cau hinh timer:
    tim_init.TIM_Prescaler     = AUDIO_TIM_PRESCALER;
    tim_init.TIM_CounterMode   = TIM_CounterMode_Up;
    tim_init.TIM_Period        = (uint32_t)AUDIO_TIM_PERIOD;
    tim_init.TIM_ClockDivision = TIM_CKD_DIV1;
    tim_init.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(AUDIO_TIM, &tim_init);

    // Cau hinh MMS (Master Mode Selection) = Update (010)
    TIM_SelectOutputTrigger(AUDIO_TIM, TIM_TRGOSource_Update);
}

// Audio_ADC_Init - Cau hinh ADC1 triggered by TIM3 TRGO
static void Audio_ADC_Init(void)
{
    ADC_InitTypeDef adc_init;

    // Bat clock ADC1
    RCC_APB2PeriphClockCmd(AUDIO_ADC_CLK, ENABLE);

    // Prescaler ADC: PCLK2 = 72MHz / 6 = 12MHz < 14MHz (yeu cau toi da)
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    // Cau hinh ADC1: doc kenh 0 (PA0), trigger TIM3 TRGO, can trai phai
    adc_init.ADC_Mode               = ADC_Mode_Independent;
    adc_init.ADC_ScanConvMode       = DISABLE;
    adc_init.ADC_ContinuousConvMode = DISABLE;          // Doi trigger
    adc_init.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_T3_TRGO; // TIM3 TRGO
    adc_init.ADC_DataAlign          = ADC_DataAlign_Right;
    adc_init.ADC_NbrOfChannel       = 1;
    ADC_Init(AUDIO_ADC, &adc_init);

    // Cau hinh thoi gian lay mau kenh 0 (PA0):
    ADC_RegularChannelConfig(AUDIO_ADC, AUDIO_ADC_CHANNEL, 1, ADC_SampleTime_239Cycles5);

    // Bat DMA cho ADC1
    ADC_DMACmd(AUDIO_ADC, ENABLE);

    // Bat External Trigger
    ADC_ExternalTrigConvCmd(AUDIO_ADC, ENABLE);

    // Bat ADC1
    ADC_Cmd(AUDIO_ADC, ENABLE);

    // Calibration ADC (bat buoc sau reset):
    ADC_ResetCalibration(AUDIO_ADC);
    while (ADC_GetResetCalibrationStatus(AUDIO_ADC));
    ADC_StartCalibration(AUDIO_ADC);
    while (ADC_GetCalibrationStatus(AUDIO_ADC));
}

// Audio_DMA_Init - Cau hinh DMA1 Channel 1 chuyen ADC1->DR den audio_buffer
static void Audio_DMA_Init(void)
{
    DMA_InitTypeDef   dma_init;
    NVIC_InitTypeDef  nvic_init;

    // Bat clock DMA1 (AHB bus)
    RCC_AHBPeriphClockCmd(AUDIO_DMA_CLK, ENABLE);

    // Reset DMA channel 1
    DMA_DeInit(AUDIO_DMA_CHANNEL);

    // Cau hinh DMA1 Channel 1:
    dma_init.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    dma_init.DMA_MemoryBaseAddr     = (uint32_t)audio_buffer;
    dma_init.DMA_DIR                = DMA_DIR_PeripheralSRC;
    dma_init.DMA_BufferSize         = AUDIO_BUFFER_SIZE;
    dma_init.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_init.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma_init.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
    dma_init.DMA_Mode               = DMA_Mode_Circular;
    dma_init.DMA_Priority           = DMA_Priority_High;
    dma_init.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(AUDIO_DMA_CHANNEL, &dma_init);

    // Bat ngat Half Transfer (HT) va Transfer Complete (TC)
    DMA_ITConfig(AUDIO_DMA_CHANNEL, DMA_IT_HT | DMA_IT_TC, ENABLE);

    // Cau hinh NVIC cho DMA1_Channel1_IRQ
    nvic_init.NVIC_IRQChannel                   = AUDIO_DMA_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority  = 0;
    nvic_init.NVIC_IRQChannelSubPriority         = 0;
    nvic_init.NVIC_IRQChannelCmd                 = ENABLE;
    NVIC_Init(&nvic_init);
}

// Audio_Init - Khoi tao toan bo he thong thu am
void Audio_Init(void)
{
    audio_state       = AUDIO_STATE_IDLE;
    audio_half_cplt   = 0;
    audio_full_cplt   = 0;
    audio_block_count = 0;

    Audio_GPIO_Init();
    Audio_TIM_Init();
    Audio_ADC_Init();
    Audio_DMA_Init();
}

// Audio_StartRecord - Bat dau thu am thanh
void Audio_StartRecord(void)
{
    uint16_t i;

    // Dat lai trang thai va co
    audio_state       = AUDIO_STATE_RECORDING;
    audio_half_cplt   = 0;
    audio_full_cplt   = 0;
    audio_block_count = 0;

    // [FIX 1] Xoa co ngat DMA con dong tu lan truoc trong DMA1->ISR
    DMA_ClearFlag(DMA1_FLAG_GL1 | DMA1_FLAG_TC1 | DMA1_FLAG_HT1 | DMA1_FLAG_TE1);

    // [FIX 2] Xoa IRQ pending con ton trong NVIC
    NVIC_ClearPendingIRQ(AUDIO_DMA_IRQn);

    // [FIX 3] Reset CNDTR ve AUDIO_BUFFER_SIZE (= 1024)
    AUDIO_DMA_CHANNEL->CNDTR = AUDIO_BUFFER_SIZE;

    // [FIX 4] Xoa audio_buffer ve mid-rail ADC 12-bit (2048 = 0V analog)
    for (i = 0U; i < AUDIO_BUFFER_SIZE; i++) {
        audio_buffer[i] = 2048U;
    }

    // Bat DMA channel 1 (CCR.EN = 1)
    DMA_Cmd(AUDIO_DMA_CHANNEL, ENABLE);

    // Bat TIM3 (CR1.CEN = 1) -> TRGO -> ADC -> DMA lay mau moi
    TIM_Cmd(AUDIO_TIM, ENABLE);
}

// Audio_StopRecord - Dung thu am thanh
void Audio_StopRecord(void)
{
    // Tat TIM3 truoc (CR1.CEN = 0)
    TIM_Cmd(AUDIO_TIM, DISABLE);

    // Tat DMA channel 1 (CCR.EN = 0)
    DMA_Cmd(AUDIO_DMA_CHANNEL, DISABLE);

    audio_state = AUDIO_STATE_DONE;
}

// Audio_BuildWAVHeader - Tao header file WAV chuan (RIFF PCM 16-bit Mono 8kHz)
WAV_Header_t Audio_BuildWAVHeader(uint32_t num_samples)
{
    WAV_Header_t hdr;
    uint32_t data_bytes = num_samples * 2U; // 16-bit = 2 byte/mau

    // RIFF chunk
    hdr.riff_id[0] = 'R'; hdr.riff_id[1] = 'I';
    hdr.riff_id[2] = 'F'; hdr.riff_id[3] = 'F';
    hdr.riff_size  = data_bytes + 36U;

    hdr.wave_id[0] = 'W'; hdr.wave_id[1] = 'A';
    hdr.wave_id[2] = 'V'; hdr.wave_id[3] = 'E';

    // fmt sub-chunk
    hdr.fmt_id[0] = 'f'; hdr.fmt_id[1] = 'm';
    hdr.fmt_id[2] = 't'; hdr.fmt_id[3] = ' ';
    hdr.fmt_size       = 16U;
    hdr.audio_format   = 1U;   // PCM
    hdr.num_channels   = 1U;   // Mono
    hdr.sample_rate    = AUDIO_SAMPLE_RATE;  // 8000 Hz
    hdr.bits_per_sample = 16U;
    hdr.byte_rate      = hdr.sample_rate * hdr.num_channels * (hdr.bits_per_sample / 8U);
    hdr.block_align    = hdr.num_channels * (hdr.bits_per_sample / 8U);

    // data sub-chunk
    hdr.data_id[0] = 'd'; hdr.data_id[1] = 'a';
    hdr.data_id[2] = 't'; hdr.data_id[3] = 'a';
    hdr.data_size  = data_bytes;

    return hdr;
}

// TRINH PHUC VU NGAT DMA1 Channel 1
void AUDIO_DMA_IRQHandler(void)
{
    // Half Transfer (HT): Nua buffer 1 [0..511] day
    if (DMA_GetITStatus(DMA1_IT_HT1)) {
        DMA_ClearITPendingBit(DMA1_IT_HT1);
        audio_half_cplt = 1;
        if (audio_state == AUDIO_STATE_RECORDING) {
            audio_state = AUDIO_STATE_BLOCK_READY;
        }
        audio_block_count++;
        if (audio_block_count >= MAX_RECORD_BLOCKS) {
            Audio_StopRecord();
        }
    }

    // Transfer Complete (TC): Nua buffer 2 [512..1023] day
    if (DMA_GetITStatus(DMA1_IT_TC1)) {
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        audio_full_cplt = 1;
        if (audio_state == AUDIO_STATE_RECORDING) {
            audio_state = AUDIO_STATE_BLOCK_READY;
        }
        audio_block_count++;
        if (audio_block_count >= MAX_RECORD_BLOCKS) {
            Audio_StopRecord();
        }
    }
}

#ifndef MID_AUDIO_FILTERS_H
#define MID_AUDIO_FILTERS_H

#include "stm32f10x.h"

// Cau hinh bo loc
#define FIR_TAPS            31      // 31 tap = do tre 15 mau
#define FILTER_SAMPLE_RATE  8000U
#define FILTER_CUTOFF_HZ    1000U

// Loai bo loc dang dung
typedef enum {
    FILTER_TYPE_FIR  = 0,
    FILTER_TYPE_IIR  = 1,
    FILTER_TYPE_NONE = 2    // Khong ap dung bo loc
} FilterType_t;

// Trang thai bo loc FIR
typedef struct {
    int16_t  delay[FIR_TAPS];
    uint8_t  index;             // Vi tri ghi vao delay line
} FIR_State;

// Trang thai bo loc IIR (Biquad Section)
typedef struct {
    int16_t  x1;
    int16_t  x2;
    int16_t  y1;
    int16_t  y2;
} IIR_State;

// He so IIR (Q15 fixed-point)
#define IIR_B0       3199
#define IIR_B1       6398
#define IIR_B2       3199
#define IIR_A1_NEG   30900
#define IIR_A2_NEG  -10923

// He so FIR (Q15, 31 tap, Hamming LP 1kHz@8kHz)
extern const int16_t fir_coeffs_lp1k[FIR_TAPS];

// Khoi tao bo loc
void FIR_Init (FIR_State *s);
void IIR_Init (IIR_State *s);

// Xu ly 1 mau: input -> output
int16_t FIR_Process (FIR_State *s, int16_t input);
int16_t IIR_Process (IIR_State *s, int16_t input);

// Xu ly mot block mau
void FIR_ProcessBlock (FIR_State *s,
                       const int16_t *src_buf,
                       int16_t       *dst_buf,
                       uint16_t       len);
void IIR_ProcessBlock (IIR_State *s,
                       const int16_t *src_buf,
                       int16_t       *dst_buf,
                       uint16_t       len);

#endif

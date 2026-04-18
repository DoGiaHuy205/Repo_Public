#include "MID_audio_filters.h"

const int16_t fir_coeffs_lp1k[FIR_TAPS] = {
    -39,   // k=0
    -67,   // k=1
    -68,   // k=2
      0,   // k=3
    157,   // k=4
    323,   // k=5
    326,   // k=6
      0,   // k=7
   -615,   // k=8
  -1186,   // k=9
  -1136,   // k=10
      0,   // k=11
   2244,   // k=12
   5008,   // k=13
   7302,   // k=14
   8192,   // k=15 - trung tam: 0.25 * 32768 = 8192
   7302,   // k=16
   5008,   // k=17
   2244,   // k=18
      0,   // k=19
  -1136,   // k=20
  -1186,   // k=21
   -615,   // k=22
      0,   // k=23
    326,   // k=24
    323,   // k=25
    157,   // k=26
      0,   // k=27
    -68,   // k=28
    -67,   // k=29
    -39    // k=30
};

// FIR_Init - Xoa trang thai delay line
void FIR_Init(FIR_State *s)
{
    uint8_t i;
    for (i = 0; i < FIR_TAPS; i++) {
        s->delay[i] = 0;
    }
    s->index = 0;
}

// FIR_Process - Xu ly 1 mau vao, tra ra 1 mau da loc
int16_t FIR_Process(FIR_State *s, int16_t input)
{
    int32_t  acc = 0;
    int16_t  k;
    uint8_t  idx;

    // Ghi mau moi vao dau delay line (circular)
    s->delay[s->index] = input;
    s->index = (s->index + 1U < FIR_TAPS) ? (s->index + 1U) : 0U;

    // Tinh tich chap FIR: y = sum( h[k] * x[n-k] )
    idx = s->index;
    for (k = 0; k < FIR_TAPS; k++) {
        // Di nguoc trong circular buffer: lay x[n-k]
        if (idx == 0U) idx = FIR_TAPS;
        idx--;
        // Q15 multiply: he so Q15 * mau Q0 = Q15, tich la Q30 -> int32
        acc += (int32_t)fir_coeffs_lp1k[k] * (int32_t)s->delay[idx];
    }

    // Dich ve Q0, bao hoa neu can
    acc >>= 15;
    if (acc >  32767) acc =  32767;
    if (acc < -32768) acc = -32768;

    return (int16_t)acc;
}

// FIR_ProcessBlock - Xu ly mot block N mau
void FIR_ProcessBlock(FIR_State *s,
                      const int16_t *src_buf,
                      int16_t       *dst_buf,
                      uint16_t       len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        dst_buf[i] = FIR_Process(s, src_buf[i]);
    }
}

// IIR_Init - Dat lai trang thai biquad
void IIR_Init(IIR_State *s)
{
    s->x1 = 0;
    s->x2 = 0;
    s->y1 = 0;
    s->y2 = 0;
}

// IIR_Process - Butterworth LP bac 2, Direct Form I, Q15
int16_t IIR_Process(IIR_State *s, int16_t input)
{
    int32_t acc;
    int16_t output;

    // Tich luy Direct Form I (Q15 arithmetic)
    acc  = (int32_t)IIR_B0     * (int32_t)input;  
    acc += (int32_t)IIR_B1     * (int32_t)s->x1;  
    acc += (int32_t)IIR_B2     * (int32_t)s->x2;  
    acc += (int32_t)IIR_A1_NEG * (int32_t)s->y1;  
    acc += (int32_t)IIR_A2_NEG * (int32_t)s->y2;  

    // Chuyen Q15 -> Q0
    acc >>= 15;
    if (acc >  32767) acc =  32767;
    if (acc < -32768) acc = -32768;
    output = (int16_t)acc;

    // Cap nhat trang thai
    s->x2 = s->x1;
    s->x1 = input;
    s->y2 = s->y1;
    s->y1 = output;

    return output;
}

// IIR_ProcessBlock - Xu ly mot block N mau
void IIR_ProcessBlock(IIR_State *s,
                      const int16_t *src_buf,
                      int16_t       *dst_buf,
                      uint16_t       len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        dst_buf[i] = IIR_Process(s, src_buf[i]);
    }
}

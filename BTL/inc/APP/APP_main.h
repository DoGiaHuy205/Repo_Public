#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "stm32f10x.h"

#define WAV_FILENAME            "AUDIO.WAV"
#define UART_CHUNK_SIZE         512U
#define WAIT_BTW_STREAMS_MS     3000U

#define PROTO_START_0           0xAAU
#define PROTO_START_1           0xBBU
#define PROTO_START_2           0xCCU
#define PROTO_TYPE_RAW          0x01U   // Audio goc, chua loc
#define PROTO_TYPE_FIR          0x02U   // FIR 1kHz LP
#define PROTO_TYPE_IIR          0x03U   // IIR 1kHz LP

#define DISPLAY_BUF_SIZE        128U

#endif

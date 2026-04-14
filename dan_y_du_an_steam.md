# DÀN Ý CHI TIẾT DỰ ÁN STEAM — VI XỬ LÝ
## "Thu Âm – Lọc Số – Hiển Thị Web Thời Gian Thực"
### STM32F103C8T6 + MAX9814 + OLED + ESP32

---

## I. GIỚI THIỆU TỔNG QUAN

### 1.1 Mục tiêu STEAM
- **S**cience: Lý thuyết tín hiệu số, bộ lọc FIR/IIR, FFT, ADC
- **T**echnology: STM32F103C8T6, ESP32, SPI/I2C/UART, FatFS
- **E**ngineering: Hệ thống nhúng real-time, double-buffer DMA
- **A**rt: Giao diện web — waveform + spectrum + audio player
- **M**ath: Convolution, Z-transform, Bilinear transform, Q15 fixed-point

### 1.2 Kiến trúc tổng thể
```
[MAX9814 Mic] → PA0 (ADC1_IN0)
                        ↓
           [STM32F103C8T6]
             TIM3 TRGO → trigger ADC @ 8kHz
             DMA1 Ch1  → double-buffer → audio_buffer[]
             SPI1      → SD Card → AUDIO.WAV
             I2C1      → OLED SSD1306
             USART1 TX (PA9)
                        ↓ UART 115200 baud
                   [ESP32]
                    WiFi → HTTP
                        ↓
              [Web Browser]
               /audio_raw  → RAW waveform + FFT
               /audio_fir  → FIR filtered
               /audio_iir  → IIR filtered
               Slider Fc   → JS real-time filter
```

---

## II. PHẦN a — THU ÂM & HIỂN THỊ OLED

### 2.1 Module MAX9814
| Thông số | Giá trị |
|---|---|
| Điện áp | 2.7–5.5V |
| Output | Analog, DC-bias ~1.25V |
| Gain | 40/50/60 dB chọn qua chân GAIN |
| AGC | Có (Auto Gain Control) |
| Kết nối | OUT → PA0 (ADC1_IN0) |

### 2.2 ADC1 — Các thanh ghi quan trọng
| Thanh ghi | Địa chỉ | Vai trò |
|---|---|---|
| `ADC1->CR1` | 0x40012404 | Chế độ: SCAN=0, CONT=0 |
| `ADC1->CR2` | 0x40012408 | ADON, EXTTRIG, EXTSEL=TIM3_TRGO, DMA |
| `ADC1->SMPR2` | 0x40012410 | Thời gian lấy mẫu CH0 = 239.5 cycles |
| `ADC1->SQR3` | 0x40012434 | SQ1=0 (channel 0 = PA0) |
| `ADC1->DR` | 0x4001244C | Data Register 12-bit right-aligned |

**Cấu hình CR2 (bit-level):**
```
bit [1]  CONT    = 0  → Single conversion, đợi trigger
bit [8]  DMA     = 1  → Bật DMA request
bit [19] EXTTRIG = 1  → Trigger từ TIM3
bit [20:17] EXTSEL = 100b → TIM3_TRGO
```

**Tính thời gian lấy mẫu:**
```
ADC_CLK = PCLK2 / 6 = 72MHz / 6 = 12 MHz
T_conv  = (239.5 + 12.5) / 12MHz ≈ 21 µs
T_trig  = 1 / 8000 = 125 µs → 21 µs << 125 µs (OK)
```

### 2.3 TIM3 — Trigger ADC 8kHz

> **Tại sao TIM3 không phải TIM2?**
> STM32F103: `ADC_ExternalTrigConv_T3_TRGO` là kết nối **hardwired** giữa TIM3 Update Event → ADC1. TIM2 chỉ có CC2 trigger, **không có TRGO** cho ADC1.

| Thanh ghi | Giá trị | Ý nghĩa |
|---|---|---|
| `TIM3->PSC` | 0 | Prescaler=0, f_CK=72MHz |
| `TIM3->ARR` | 8999 | f = 72MHz/(8999+1) = **8000 Hz** |
| `TIM3->CR2[6:4]` | 010b | MMS=Update → TRGO phát khi reset |

**Công thức:**
```
f_TRGO = SYSCLK / (PSC+1) / (ARR+1)
       = 72,000,000 / 1 / 9000 = 8,000 Hz ✓
```

### 2.4 DMA1 Channel 1 — Double Buffer

**Tại sao dùng DMA?** CPU không phải polling từng ADC conversion → ghi âm liên tục không mất mẫu.

| Thanh ghi | Giá trị | Ý nghĩa |
|---|---|---|
| `CPAR` | &ADC1->DR | Peripheral addr (cố định) |
| `CMAR` | audio_buffer | Memory addr (tăng dần) |
| `CNDTR` | 1024 | Số lần transfer |
| `CCR[5]` CIRC | 1 | Circular mode |
| `CCR[7]` MINC | 1 | Memory increment |
| `CCR[3:2]` MSIZE | 01b | HalfWord (16-bit) |
| `CCR[2]` HTIE | 1 | Half Transfer Interrupt |
| `CCR[1]` TCIE | 1 | Transfer Complete Interrupt |

**Cơ chế Ping-Pong:**
```
audio_buffer[1024]:
  [0..511]    = Half 1 ← HT ISR: CPU xử lý khi DMA fill nửa 2
  [512..1023] = Half 2 ← TC ISR: CPU xử lý khi DMA fill nửa 1

ISR chỉ SET flag, main loop xử lý nặng:
  audio_half_cplt = 1  (HT)
  audio_full_cplt = 1  (TC)
  audio_block_count++
  if count >= MAX_RECORD_BLOCKS: Audio_StopRecord()
```

### 2.5 Chuyển ADC → PCM 16-bit
```
ADC: 0–4095 (12-bit unsigned), mid ≈ 2048 (MAX9814 DC bias = VCC/2)

PCM = (ADC_val - 2048) << 4
    → [-32768, +32767]  ← int16_t, chuẩn WAV
```

### 2.6 Cấu trúc WAV Header 44 byte
```
Offset  Size  Field            Giá trị
00      4     "RIFF"
04      4     file_size - 8
08      4     "WAVE"
12      4     "fmt "
16      4     16               (PCM)
20      2     1                (PCM = linear)
22      2     1                (Mono)
24      4     8000             (Sample Rate Hz)
28      4     16000            (ByteRate = SR×ch×bits/8)
32      2     2                (BlockAlign)
34      2     16               (BitsPerSample)
36      4     "data"
40      4     data_bytes       (= num_samples × 2)
44      ...   PCM int16_t[]
```

### 2.7 OLED SSD1306 — I2C
- Giao tiếp: I2C1, 400kHz, addr 0x3C
- Chân: PB6 (SCL), PB7 (SDA)
- Framebuffer: 128×64 bit = 1024 byte trên RAM OLED
- Waveform: 128 điểm downsampled từ audio_buffer, map biên độ → pixel Y

---

## III. PHẦN b — UART STM32 → ESP32 → WEB

### 3.1 USART1 — Cấu hình
| Thông số | Giá trị |
|---|---|
| TX | PA9 (AF_PP) |
| Baud | 115200 bps |
| Frame | 8N1 |
| `BRR` | 72MHz / 115200 = **625** |

### 3.2 Giao thức UART tự định nghĩa
```
[0xAA][0xBB][0xCC][TYPE][SZ3][SZ2][SZ1][SZ0][... WAV bytes ...]
│←── Magic sync ──→│ │    │←── Big-endian uint32 ──→│
                    TYPE:
                      0x01 = RAW  (audio gốc)
                      0x02 = FIR  (lọc FIR LP 1kHz)
                      0x03 = IIR  (lọc IIR LP 1kHz)
```

### 3.3 ESP32 — UART State Machine
```
RX_IDLE → (0xAA) → RX_S1 → (0xBB) → RX_S2 → (0xCC) → RX_TYPE
       → RX_SZ0 → RX_SZ1 → RX_SZ2 → RX_SZ3
       → RX_DATA: ghi vào raw_buf/fir_buf/iir_buf
       → Khi rxReceived >= rxExpected: set raw_ready=true, rxState=IDLE
```

### 3.4 Web Endpoints
| URL | Mô tả |
|---|---|
| `GET /` | HTML page (nhúng trong PROGMEM) |
| `GET /audio_raw` | Serve raw_buf (WAV) |
| `GET /audio_fir` | Serve fir_buf (WAV) |
| `GET /audio_iir` | Serve iir_buf (WAV) |
| `GET /check` | JSON `{"raw":v,"fir":v,"iir":v,"heap":n}` |

### 3.5 Polling & Version Detection
```
Browser poll /check mỗi 1500ms
Mỗi lần raw_ready=true: rv++ (version counter)
Browser: nếu d.raw !== rawVer → fetch /audio_raw → decode → render
```

---

## IV. PHẦN c — BỘ LỌC FIR & IIR TRÊN STM32

### 4.1 FIR — Finite Impulse Response

**Phương trình:**
```
y[n] = Σ(k=0..30) h[k] · x[n-k]     (31 tap)
```

**Thiết kế hệ số — Windowed Sinc + Hamming:**
```
fc = 1000 Hz, Fs = 8000 Hz
ωc = 2π·fc/Fs = π/4

h_ideal[n] = sin(ωc·(n-15)) / (π·(n-15))   n ≠ 15
h_ideal[15] = ωc/π = 0.25

w[n] = 0.54 - 0.46·cos(2π·n/30)   (Hamming)

h[n] = h_ideal[n] × w[n]
```

**Q15 Fixed-Point (không cần FPU):**
```
h_Q15[k] = round(h[k] × 32768)

Tính output:
  int32_t acc = 0;
  for k: acc += h_Q15[k] × x[n-k]   (int32 tránh tràn)
  y[n] = clip(acc >> 15, -32768, 32767)
```

**Đặc tính:**
- Pass-band 0–1kHz: suy giảm < 0.5 dB
- Stop-band > 1.5kHz: suy giảm > 40 dB
- Độ trễ nhóm: **hằng số 1.875 ms** (linear phase ✓)

### 4.2 IIR — Butterworth bậc 2

**Phương trình sai phân (Direct Form I):**
```
y[n] = b0·x[n] + b1·x[n-1] + b2·x[n-2]
             - a1·y[n-1] - a2·y[n-2]
```

**Thiết kế — Bilinear Transform:**
```
Analog Butterworth: H(s) = 1/(s² + √2·s + 1)

Pre-warp: K = tan(π·fc/Fs) = tan(π/8) = 0.41421
norm = K² + √2·K + 1 = 0.1716 + 0.5858 + 1 = 1.7574

b0 = b2 = K²/norm      = 0.09763
b1      = 2K²/norm     = 0.19526
a1      = 2(K²-1)/norm = -0.94281
a2      = (K²-√2K+1)/norm = 0.33333
```

**Q15:**
```
IIR_B0 = 3199   IIR_B1 = 6398   IIR_B2 = 3199
IIR_A1_NEG = 30900    (tương ứng -a1 = +0.94281)
IIR_A2_NEG = -10923   (tương ứng -a2 = -0.33333)
```

### 4.3 So sánh FIR vs IIR
| Tiêu chí | FIR 31-tap | IIR bậc 2 |
|---|---|---|
| Phép nhân/mẫu | 31 | 4 |
| Linear phase | ✓ | ✗ |
| Ổn định vô điều kiện | ✓ | Cần kiểm tra |
| Hiệu quả tính toán | Thấp | Cao |

---

## V. PHẦN d — WEB ĐIỀU CHỈNH FC THỜI GIAN THỰC

### 5.1 Hai lớp lọc song song
```
STM32 (cố định Fc=1kHz) → /audio_fir, /audio_iir
                                        ↕ so sánh
Browser JS (Fc tùy chỉnh) → jsFirPcm, jsIirPcm
        ↑ từ rawPcm (decode /audio_raw)
```

### 5.2 Fix thời lượng JS filter (Bug quan trọng)
```
VẤN ĐỀ: decodeAudioData() resample 8kHz → 44100Hz
  → rawPcm.length tăng 5.5×
  → pcmToWav(filtered, FS=8000) → thời lượng 17s sai!

FIX:
  actualSR = buf.sampleRate  // lấy từ AudioBuffer
  designFIR(fc, actualSR)    // Fc/Fs đúng
  pcmToWav(filtered, actualSR) // duration = RAW ✓
```

### 5.3 Hiển thị Waveform (Canvas)
```
px X = sample_index × (canvas.width / pcm.length)
px Y = H/2 - amplitude × H/2 × 0.9

Màu:
  RAW       #58a6ff (xanh dương, nét liền)
  FIR(JS)   #f0c040 (vàng, nét liền)
  IIR(JS)   #ff7eb3 (hồng, nét liền)
  FIR STM32 #3fb950 (xanh lá, nét đứt)
  IIR STM32 #bc8cff (tím, nét đứt)
```

### 5.4 Hiển thị FFT
```
DFT N=1024 điểm tính trong browser:
  mag[k] = sqrt(Re² + Im²) / N    k=0..511
  Trục X: 0 → 4000 Hz (Nyquist = Fs/2)
  Grid: 500Hz, 1kHz, 2kHz, 3kHz
  Fc marker: đường gạch tại fc hiện tại
```

### 5.5 Slider Fc
```
FIR: range 100–3500 Hz, step 50, default 1000 Hz
IIR: range 100–3500 Hz, step 50, default 1000 Hz
Limit: ≤ Nyquist (Fs/2 = 4000 Hz)
Debounce 400ms → auto applyFilters()
Nút "Reset 1kHz" → về mặc định khớp STM32
```

---

## VI. PHẦN e — USER MANUAL

### 6.1 Kết nối phần cứng
| Linh kiện | Chân | STM32 | Giao thức |
|---|---|---|---|
| MAX9814 OUT | → | PA0 | ADC (Analog) |
| SD CS | → | PA4 | SPI1 |
| SD SCK | → | PA5 | SPI1 |
| SD MISO | → | PA6 | SPI1 |
| SD MOSI | → | PA7 | SPI1 |
| OLED SCL | → | PB6 | I2C1 |
| OLED SDA | → | PB7 | I2C1 |
| ESP32 RX | ← | PA9 | USART1 |
| BTN | PA1→GND | | Pull-Up nội |

### 6.2 Hướng dẫn vận hành
```
1. Cấp nguồn → OLED: "AUDIO+FILTER / STM32->ESP32"
2. Mở browser: http://[ESP32_IP]
3. OLED: [ READY ] → Nhấn BTN (PA1)
4. Thu âm ~3s (OLED hiện waveform)
5. OLED: STEP 3A/4 "Send RAW->ESP32" → file 0x01
6. Chờ 3s (browser tải RAW xong)
7. OLED: STEP 3B/4 "FIR 1kHz LP" → file 0x02
8. Chờ 3s (browser tải FIR xong)
9. OLED: STEP 3C/4 "IIR 1kHz LP" → file 0x03
10. OLED: "ALL DONE! Check web tabs"
11. Web: Charts cập nhật, có thể phát audio
12. Kéo slider Fc → xem tín hiệu thay đổi realtime
13. Nhấn BTN lần nữa để ghi âm mới
```

### 6.3 Bảng thông số hiển thị trên Web
| Thông số | Đơn vị | Ý nghĩa |
|---|---|---|
| Thời lượng | s | Độ dài file WAV |
| Kích thước | KB | Dung lượng file |
| RMS | dBFS | Mức năng lượng tín hiệu |
| Heap | KB | RAM còn lại trên ESP32 |

---

## VII. PHÂN TÍCH KỸ THUẬT SÂU (CHO QUÁ TRÌNH BÁO CÁO)

### 7.1 Clock Tree
```
HSE 8MHz → PLL×9 → SYSCLK 72MHz
  ├─ AHB (HCLK) 72MHz   → DMA, Flash, Core
  ├─ APB1 36MHz          → TIM3 (×2=72MHz), I2C1, USART2
  └─ APB2 72MHz          → ADC1(÷6=12MHz), USART1, SPI1, GPIO
```

### 7.2 Bug Fixes quan trọng

#### Bug 1 — DMA Spurious IRQ (âm thanh cũ trong file mới)
```
Nguyên nhân:
  Audio_StopRecord() disable DMA → CNDTR giữ giá trị giữa chừng
  NVIC còn pending HT/TC flag → ISR fire ngay khi re-enable
  → Block đầu tiên = data cũ trong audio_buffer

Fix trong Audio_StartRecord():
  [1] DMA_ClearFlag(GL1|TC1|HT1|TE1)   // Xóa DMA status flags
  [2] NVIC_ClearPendingIRQ(DMA1_Ch1)   // Xóa NVIC pending
  [3] AUDIO_DMA_CHANNEL->CNDTR = 1024  // Reset bộ đếm về đầu
  [4] for(i) audio_buffer[i] = 2048    // Silence = 0V analog
```

#### Bug 2 — AudioContext Resample (duration 17s)
```
Nguyên nhân: decodeAudioData() → 44100Hz, nhưng dùng FS=8000
  → pcmToWav(filtered, 8000) → 44100 samples/8000Hz = 17s

Fix: actualSR = buf.sampleRate (44100)
  designFIR(fc, actualSR), pcmToWav(filtered, actualSR)
```

#### Bug 3 — UART Buffer Overflow
```
Nguyên nhân: serveWAV() gọi processUART() trong loop
  → WiFi load chậm → bytes mới đến → HW buffer 4096B tràn

Fix: WAIT_BTW_STREAMS_MS = 3000ms giữa các stream
  → 47KB / WiFi ~5MB/s = <10ms → 3s là đủ thừa
```

### 7.3 Luồng dữ liệu đầy đủ
```
MAX9814 → ADC1(PA0, 12-bit) → DMA1Ch1(circular,1024)
        → audio_buffer[] (double-buffer HT/TC)
        → PCM=(ADC-2048)<<4
        → SD_WAV_WriteBlock() → f_write() → AUDIO.WAV (FatFS)

AUDIO.WAV → ESP32_SendWAV_Raw()     → UART TYPE=0x01
          → ESP32_StreamFiltered()  → FIR_Process() → TYPE=0x02
                                    → IIR_Process() → TYPE=0x03

ESP32 UART RX → State Machine → raw_buf/fir_buf/iir_buf
     WiFi HTTP → Browser fetch /audio_raw|fir|iir
              → decodeAudioData() → Float32 PCM (@ actualSR)
              → Canvas: waveform + FFT
              → designFIR/IIR(fc, actualSR) → JS filter overlay
              → pcmToWav() → <audio> player
```

---

## VIII. PHÂN CÔNG NHÓM
| Thành viên | Trách nhiệm |
|---|---|
| **Huy** | STM32: ADC/DMA/TIM3, SD/WAV, UART protocol |
| **Minh Hiếu** | STM32: FIR/IIR (toán + Q15), OLED driver |
| **Xuân Hiếu** | ESP32: Web server, HTML/JS waveform+FFT+slider |
| **Cả nhóm** | Tích hợp, debug, báo cáo, manual |

---

## IX. TÀI LIỆU THAM KHẢO
1. **RM0008** — STM32F10x Reference Manual: Ch.11 DMA, Ch.12 ADC, Ch.14 TIMx, Ch.27 USART
2. **WAVE PCM Format** — http://soundfile.sapp.org/doc/WaveFormat/
3. **FIR Design** — Oppenheim & Schafer, *Discrete-Time Signal Processing*
4. **IIR Butterworth** — Bilinear Transform (Proakis & Manolakis)
5. **FatFS** — ChaN's FAT Filesystem: http://elm-chan.org/fsw/ff/
6. **ESP32 Arduino** — WebServer & HardwareSerial API docs

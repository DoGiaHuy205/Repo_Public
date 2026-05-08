# WatchDog UART — Tang Nano 9K

Hệ thống Watchdog Timer cấu hình qua UART, triển khai trên FPGA **Sipeed Tang Nano 9K** (GW1NR-LV9QN88PC6/I5).

---

## Yêu Cầu

- Phần cứng: Sipeed Tang Nano 9K

- Phần mềm nạp: [Gowin EDA](https://www.gowinsemi.com/en/support/download_eda/) (có Gowin Programmer) hoặc [openFPGALoader](https://github.com/trabucayre/openFPGALoader)

- Terminal UART: Hercules, RealTerm, hoặc Python `pyserial`

- Kết nối: Cáp USB-C vào cổng USB-C của Tang Nano 9K

---

## 1. Nạp Bitstream

### Bước 1 — Tổng hợp & Place-Route (nếu chưa có bitstream)

1. Mở Gowin EDA, mở project `WatchDog_UART_Tang9k.gprj`

2. Chạy Synthesize → Place & Route

3. File bitstream xuất ra tại: `impl/pnr/WatchDog_UART_Tang9k.fs`

### Bước 2 — Nạp bằng Gowin Programmer

1. Kết nối Tang Nano 9K qua cổng JTAG (cáp USB-C)

2. Mở Gowin Programmer (trong Gowin EDA: Tools → Programmer)

3. Chọn device: GW1NR-9C

4. Operation: SRAM Program (tạm thời) hoặc Flash Program (lưu vĩnh viễn)

5. Chọn file `.fs` → nhấn Program/Verify

### Bước 2 — Nạp bằng openFPGALoader (thay thế)

```bash
openFPGALoader -b tangnano9k impl/pnr/WatchDog_UART_Tang9k.fs
```

Nạp vĩnh viễn vào Flash:

```bash
openFPGALoader -b tangnano9k --write-flash impl/pnr/WatchDog_UART_Tang9k.fs
```

---

## 2. Kết Nối Phần Cứng

```
PC ──USB-C──► Tang Nano 9K
               (BL702 USB-Serial onboard)
               UART: 115200 baud, 8N1
```

| Tín hiệu | Chân | Mô tả |
|---|---|---|
| Clock | 52 | 27 MHz oscillator onboard |
| S1 (`wdi_btn_n`) | 3 | Nút WDI Kick (active-LOW) |
| S2 (`en_btn_n`) | 4 | Nút Enable (active-LOW) |
| LED WDO (`wdo`) | 10 | SÁNG = FAULT đang xảy ra |
| LED ENOUT (`enout`) | 11 | SÁNG = Watchdog đang giám sát |
| UART RX | 18 | Nhận lệnh từ PC |
| UART TX | 17 | Gửi phản hồi về PC |

> Lưu ý LED: Tang Nano 9K dùng LED Common-Anode — mức 0 = SÁNG, mức 1 = TẮT.

---

## 3. Chạy Chương Trình

### 3.1 Kích Hoạt Bằng Nút Bấm

- Nhấn giữ S2 → EN=1 → vào trạng thái ARMING (chờ 150 µs)

- Sau arm delay → vào ACTIVE → LED ENOUT sáng

- Nhấn S1 định kỳ trong vòng 1600 ms để tránh FAULT

- Nếu quên nhấn → FAULT → LED WDO sáng, WDO=0 kéo xuống

- Sau 200 ms → tự phục hồi về ACTIVE

- Thả S2 → tắt watchdog → về DISABLED

### 3.2 Kích Hoạt Qua UART (Software)

Mở terminal, cấu hình 115200 baud, 8N1, chế độ HEX.

Bật watchdog:

```
Gửi:   55 01 00 04 01 00 00 00 51
Nhận:  AA 01 00 00 AB
```
→ EN_SW=1 → ARMING → ACTIVE → LED ENOUT sáng

Gửi kick định kỳ (< 1600 ms/lần):

```
Gửi:   55 03 00 00 56
Nhận:  AA 03 00 00 A9
```

Đọc trạng thái:

```
Gửi:   55 02 10 00 47
Nhận:  AA 02 10 04 [D3 D2 D1 D0] CHK
```

Tắt watchdog:

```
Gửi:   55 01 00 04 00 00 00 00 50
Nhận:  AA 01 00 00 AB
```

---

## 4. Cấu Hình Tham Số Qua UART

Gửi lệnh cấu hình trước khi bật watchdog để có hiệu lực từ lần bật tiếp theo.

| Tham số | Mặc định | Lệnh ví dụ |
|---|---|---|
| tWD (timeout) | 1600 ms | `55 01 04 04 40 06 00 00 63` (1600 ms) |
| tRST (fault hold) | 200 ms | `55 01 08 04 C8 00 00 00 81` (200 ms) |
| arm_delay | 150 µs | `55 01 0C 04 96 00 00 00 9F` (150 µs) |

Ví dụ đặt tWD = 5000 ms:

```
Gửi:  55 01 04 04 88 13 00 00 1E
Nhận: AA 01 04 00 AF
```

Xóa FAULT ngay lập tức:

```
Gửi:  55 01 00 04 04 00 00 00 54
Nhận: AA 01 00 00 AB
```

---

## 5. Bản Đồ Thanh Ghi

| Địa chỉ | Tên | R/W | Mô tả |
|---|---|---|---|
| `0x00` | CTRL | R/W | [0]=EN_SW, [1]=WDI_SRC(soft kick), [2]=CLR_FAULT |
| `0x04` | TWD_MS | R/W | Watchdog timeout (ms) |
| `0x08` | TRST_MS | R/W | WDO hold time sau fault (ms) |
| `0x0C` | ARM_US | R/W | Arm delay sau khi EN (µs, 16-bit) |
| `0x10` | STATUS | R only | [0]=EN_EFF, [1]=FAULT, [2]=ENOUT, [3]=WDO, [4]=LAST_KICK_SRC |

---

## 6. Giao Thức Frame UART

```
Request  (PC → FPGA): 55 | CMD | ADDR | LEN | DATA[0..N-1] | CHK
Response (FPGA → PC): AA | CMD | ADDR | LEN | DATA[0..3]   | CHK
```

- SOP: `0x55` (request) / `0xAA` (response)

- CMD: `0x01`=Write, `0x02`=Read, `0x03`=Kick

- LEN: số byte data (Write=4, Read=0, Kick=0)

- DATA: little-endian, 4 bytes (chỉ có khi Write hoặc Read response)

- CHK: XOR của tất cả byte từ SOP đến hết DATA

Tính CHK bằng Python:

```python
def chk(frame): return __import__('functools').reduce(lambda a,b: a^b, frame)

# Ví dụ: Kick
print(hex(chk([0x55, 0x03, 0x00, 0x00])))  # → 0x56
```

---

## 7. Sơ Đồ Trạng Thái FSM

```
          EN rising edge
DISABLED ─────────────────► ARMING (arm_delay µs)
    ▲                            │ timer done
    │ EN=0               EN=0   ▼
    │◄───────────────── ACTIVE ──────► FAULT
    │                    ▲  │             │
    │                    │  │ kick        │ tRST ms / CLR_FAULT
    │                    └──┘             │
    └─────────────────────────────────────┘
              EN=0
```

---

## 8. Cấu Trúc Project

```
WatchDog_UART_Tang9k/
├── WatchDog_UART_Tang9k.gprj   # Gowin project file
└── src/
    ├── WatchDog_UART.v          # Top module
    ├── watchdog_core.v          # FSM watchdog chính
    ├── regfile.v                # Thanh ghi cấu hình
    ├── uart_frame_parser.v      # Bộ phân tích frame UART
    ├── uart_rx.v                # UART receiver
    ├── uart_tx.v                # UART transmitter
    ├── sync_debounce.v          # Lọc nhiễu nút bấm
    ├── WatchDog_UART_Tang9k.cst # Pin constraint
    └── WatchDog_UART.sdc        # Timing constraint
```

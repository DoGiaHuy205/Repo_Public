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

- Nhấn giữ S1 → EN=1 → vào trạng thái ARMING (chờ 150 µs)

- Sau arm delay → vào ACTIVE → LED ENOUT sáng

- Nhấn S2 định kỳ trong vòng 1600 ms để tránh FAULT

- Nếu quên nhấn → FAULT → LED WDO sáng, WDO=0 kéo xuống

- Sau 200 ms → tự phục hồi về ACTIVE

- Thả S1 → tắt watchdog → về DISABLED

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
Nhận:  AA 02 10 04 [D0 D1 D2 D3] CHK
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
| tWD (timeout) | 1600 ms | `55 01 04 04 40 06 00 00 12` (1600 ms) |
| tRST (fault hold) | 200 ms | `55 01 08 04 C8 00 00 00 90` (200 ms) |
| arm_delay | 150 µs | `55 01 0C 04 96 00 00 00 CA` (150 µs) |

Ví dụ đặt tWD = 5000 ms:

```
Gửi:  55 01 04 04 88 13 00 00 CF
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


---

## 9. Hướng dẫn sử dụng chi tiết Watchdog UART

### 9.1 Tổng quan kiến trúc:

```mermaid
    PC  ──UART──►  uart_rx  ──►  uart_frame_parser  ──►  regfile  ──►  watchdog_core
                                    │                    │
                             (phân tích frame)    (lưu thanh ghi)
                                    │
                             uart_tx ──►  PC
```

### 9.2 Cấu Trúc Frame UART

- Frame Gửi (PC → FPGA)

```
┌──────┬──────┬──────┬──────┬──────────────────────┬──────┐
│ SOP  │ CMD  │ ADDR │ LEN  │   DATA [LEN bytes]   │ CHK  │
│ 0x55 │ 1B   │ 1B   │ 1B   │   0 hoặc 4 bytes     │ 1B   │
└──────┴──────┴──────┴──────┴──────────────────────┴──────┘
```

- Frame Nhận (FPGA → PC)

```
┌──────┬──────┬──────┬──────┬──────────────────────┬──────┐
│ SOP  │ CMD  │ ADDR │ LEN  │   DATA [LEN bytes]   │ CHK  │
│ 0xAA │ 1B   │ 1B   │ 1B   │     4 bytes          │ 1B   │
└──────┴──────┴──────┴──────┴──────────────────────┴──────┘
```

- Giải thích trường:

| Trường | Loại | Mô tả |
| :---: | :---: | :---: |
| SOP | 1B | Start Of Packet. Gửi = 0x55, Nhận = 0xAA. Dùng để parser nhận biết đầu frame |
| CMD | 1B | Mã lệnh: 0x01=Write, 0x02=Read, 0x03=Kick, 0x04=Status |
| ADDR | 1B | Địa chỉ thanh ghi (0x00-0x14) |
| LEN | 1B | Số byte DATA đi kèm. Write = 4, Read request = 0, Kick = 0 |
| DATA | 0-4B | Dữ liệu, little-endian (byte thấp gửi trước) |
| CHK | 1B | XOR checksum của tất cả các byte trong frame (SOP đến hết DATA) |

### 9.3 Bảng Thanh Ghi (Register Map)

- Địa chỉ thanh ghi:

| Địa chỉ | Tên | R/W | Mô tả |
| :---: | :---: | :---: | :---: |
| `0x00` | CTRL | R/W | Điều khiển: bit 0=EN_SW (bật/tắt), bit 1=WDI_SRC (chọn kick software), bit 2=CLR_FAULT (xóa lỗi) |
| `0x04` | TWD_MS | R/W | Watchdog timeout (ms), 0-65535 ms |
| `0x08` | TRST_MS | R/W | Thời gian giữ WDO sau khi xảy ra lỗi (ms), 0-65535 ms |
| `0x0C` | ARM_US | R/W | Thời gian delay sau khi bật EN trước khi watchdog hoạt động (µs), 0-65535 µs |
| `0x10` | STATUS | Ronly | Trạng thái: bit 0=EN_EFF (EN đang có hiệu lực), bit 1=FAULT (đang lỗi), bit 2=ENOUT (watchdog đang giám sát), bit 3=WDO (mức tín hiệu WDO), bit 4=LAST_KICK_SRC (nguồn kick cuối cùng) |

- Thanh Ghi CTRL (0x00) — Chi Tiết Các Bit

| Bit | Tên | Mô tả |
| :---: | :---: | :---: |
| 2 | CLR_FAULT | Viết 1 để xóa FAULT ngay lập tức, tự động về 0 sau 1 chu kỳ xung nhịp |
| 1 | WDI_SRC | 0 = chỉ nhận kick từ nút bấm S1 (HW) |
|   |   | 1 = chỉ nhận kick từ lệnh UART (SW) |
| 0 | EN_SW | 0 = tắt watchdog bằng phần mềm |
|   |   | 1 = bật watchdog bằng phần mềm |

- Thanh Ghi TWD_MS (0x04) — Watchdog Timeout

| Bit | Mô tả |
| :---: | :---: |
| 15..0 | Giá trị timeout tính bằng ms, từ 0 đến 65535 (65.5 giây). Nếu watchdog không được kick trong khoảng thời gian này → xảy ra lỗi FAULT. |

- Thanh Ghi TRST_MS (0x08) — Fault Hold Time

| Bit | Mô tả |
| :---: | :---: |
| 15..0 | Thời gian giữ tín hiệu WDO ở mức logic 0 sau khi watchdog phát hiện FAULT. Giá trị từ 0 đến 65535 ms. |

- Thanh Ghi ARM_US (0x0C) — Arm Delay Time

| Bit | Mô tả |
| :---: | :---: |
| 15..0 | Thời gian delay tính bằng µs, từ 0 đến 65535 µs. Sau khi EN được bật, watchdog sẽ đợi khoảng thời gian này trước khi bắt đầu giám sát. Giúp tránh lỗi FAULT trong quá trình khởi động.

- Thanh Ghi STATUS (0x10) — Status Register

| Bit | Tên | Mô tả |
| :---: | :---: | :---: |
| 4 | LAST_KICK_SRC | Nguồn kick cuối cùng: 0=không có (timeout), 1=s1(HW), 2=uart(SW) |
| 3 | WDO | Mức tín hiệu WDO ra ngoài (0=idle/fault, 1=active) |
| 2 | ENOUT | Trạng thái watchdog: 1=đang giám sát, 0=tắt |
| 1 | FAULT | Trạng thái lỗi: 1=đang lỗi, 0=bình thường |
| 0 | EN_EFF | Trạng thái EN hiệu lực: 1=đang bật, 0=tắt |

### 9.4 Các Lệnh UART — Chi Tiết Từng Lệnh

#### 9.4.1. Lệnh 1: WRITE (CMD = 0x01)

- Chức năng: Ghi giá trị 4 byte vào một thanh ghi.

- Cấu trúc frame gửi:

```
55 | 01 | ADDR | 04 | D0 D1 D2 D3 | CHK
```

- Frame nhận về:

```
AA | 01 | ADDR | 00 | CHK
```

- Ví dụ 1: Bật Watchdog (EN_SW=1, WDI_SRC=SW)

```
Gửi:  55 01 00 04 01 00 00 00 51
Nhận: AA 01 00 00 AB
```

- Phân tích byte-by-byte frame gửi: 55 01 00 04 01 00 00 00 51

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0x55` | Bắt đầu frame gửi |
| CMD | `0x01` | Lệnh ghi (Write) |
| ADDR | `0x00` | Ghi vào thanh ghi CTRL (0x00) |
| LEN | `0x04` | Độ dài dữ liệu (4 byte) |
| D0 | `0x01` | Dữ liệu byte 0 = 1 (EN_SW=1) |
| D1 | `0x00` | Dữ liệu byte 1 = 0 (WDI_SRC=0) |
| D2 | `0x00` | Dữ liệu byte 2 = 0 |
| D3 | `0x00` | Dữ liệu byte 3 = 0 |
| CHK | `0x51` | Checksum |

- Phân tích byte-by-byte frame nhận: AA 01 00 00 AB

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0xAA` | Bắt đầu frame nhận |
| CMD | `0x01` | Echo lại CMD = Write |
| ADDR | `0x00` | Echo lại ADDR = 0x00 |
| LEN | `0x00` | Không có data (Write ACK) |
| CHK | `0xAB` | Checksum |

---

- Ví dụ 2: Bật Watchdog + Dùng SW Kick
```
Gửi:  55 01 00 04 03 00 00 00 53
Nhận: AA 01 00 00 AB
```

- Phân tích byte-by-byte frame gửi: 55 01 00 04 03 00 00 00 53

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0x55` | Bắt đầu frame gửi |
| CMD | `0x01` | Lệnh ghi (Write) |
| ADDR | `0x00` | Ghi vào thanh ghi CTRL (0x00) |
| LEN | `0x04` | Độ dài dữ liệu (4 byte) |
| D0 | `0x03` | Dữ liệu byte 0 = 3 (EN_SW=1, WDI_SRC=1) |
| D1 | `0x00` | Dữ liệu byte 1 = 0 |
| D2 | `0x00` | Dữ liệu byte 2 = 0 |
| D3 | `0x00` | Dữ liệu byte 3 = 0 |
| CHK | `0x53` | Checksum |

- Phân tích byte-by-byte frame nhận: AA 01 00 00 AB

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0xAA` | Bắt đầu frame nhận |
| CMD | `0x01` | Echo lại CMD = Write |
| ADDR | `0x00` | Echo lại ADDR = 0x00 |
| LEN | `0x00` | Không có data (Write ACK) |
| CHK | `0xAB` | Checksum |

---

- Ví Dụ 3: Xóa FAULT Ngay Lập Tức (CLR_FAULT)
```
Gửi:  55 01 00 04 04 00 00 00 54
Nhận: AA 01 00 00 AB
```

- Phân tích byte-by-byte frame gửi: 55 01 00 04 04 00 00 00 54

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0x55` | Bắt đầu frame gửi |
| CMD | `0x01` | Lệnh ghi (Write) |
| ADDR | `0x00` | Ghi vào thanh ghi CTRL (0x00) |
| LEN | `0x04` | Độ dài dữ liệu (4 byte) |
| D0 | `0x04` | Dữ liệu byte 0 = 4 (KICK_SW=1) |
| D1 | `0x00` | Dữ liệu byte 1 = 0 |
| D2 | `0x00` | Dữ liệu byte 2 = 0 |
| D3 | `0x00` | Dữ liệu byte 3 = 0 |
| CHK | `0x54` | Checksum |

- Phân tích byte-by-byte frame nhận: AA 01 00 00 AB

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0xAA` | Bắt đầu frame nhận |
| CMD | `0x01` | Echo lại CMD = Write |
| ADDR | `0x00` | Echo lại ADDR = 0x00 |
| LEN | `0x00` | Không có data (Write ACK) |
| CHK | `0xAB` | Checksum |

---

- Ví dụ 4: Cài Timeout Watchdog = 5000 ms
```
Gửi:  55 01 04 04 88 13 00 00 CF
Nhận: AA 01 04 00 AF
```

- Phân tích byte-by-byte frame gửi: 55 01 04 04 88 13 00 00 CF

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0x55` | Bắt đầu frame gửi |
| CMD | `0x01` | Lệnh ghi (Write) |
| ADDR | `0x04` | Ghi vào thanh ghi TWD_MS (0x04) |
| LEN | `0x04` | Độ dài dữ liệu (4 byte) |
| D0 | `0x88` | Dữ liệu byte 0 = 136 |
| D1 | `0x13` | Dữ liệu byte 1 = 19 |
| D2 | `0x00` | Dữ liệu byte 2 = 0 |
| D3 | `0x00` | Dữ liệu byte 3 = 0 |
| CHK | `0xCF` | Checksum |

- Phân tích byte-by-byte frame nhận: AA 01 04 00 AF

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0xAA` | Bắt đầu frame nhận |
| CMD | `0x01` | Echo lại CMD = Write |
| ADDR | `0x04` | Echo lại ADDR = 0x04 |
| LEN | `0x00` | Không có data (Write ACK) |
| CHK | `0xAF` | Checksum |

---

#### 9.4.2. Lệnh 2: READ (CMD = 0x02)

- Chức năng: Đọc 4 byte từ một thanh ghi.

- Cấu trúc frame gửi:

```
55 | 02 | ADDR | 00 | CHK
```

- Frame nhận về:

```
AA | 02 | ADDR | 04 | D0 D1 D2 D3 | CHK
```

- Ví dụ 1: Đọc Trạng Thái (STATUS register)
```
Gửi:  55 02 10 00 47
Nhận: AA 02 10 04 [D0 D1 D2 D3] CHK
```

- Phân tích byte-by-byte frame gửi: 55 02 10 00 47

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0x55` | Bắt đầu frame gửi |
| CMD | `0x02` | Lệnh đọc (Read) |
| ADDR | `0x10` | Đọc thanh ghi STATUS (0x10) |
| LEN | `0x00` | Không có data (Read request) |
| CHK | `0x47` | Checksum |

- Phân tích byte-by-byte frame nhận: AA 02 10 04 [D0 D1 D2 D3] CHK

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0xAA` | Bắt đầu frame nhận |
| CMD | `0x02` | Echo lại CMD = Read |
| ADDR | `0x10` | Echo lại ADDR = 0x10 |
| LEN | `0x04` | Độ dài dữ liệu (4 byte) |
| D0 | `0x00` | Dữ liệu byte 0 = 0 |
| D1 | `0x00` | Dữ liệu byte 1 = 0 |
| D2 | `0x00` | Dữ liệu byte 2 = 0 |
| D3 | `0x01` | Dữ liệu byte 3 = 1 |
| CHK | `0x56` | Checksum |

---

- Ví Dụ 2: Đọc Timeout Watchdog Hiện Tại

```
Gửi:  55 02 04 00 53
Nhận: AA 02 04 04 40 06 00 00 EC
```

- Phân tích byte-by-byte frame gửi: 55 02 04 00 53

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0x55` | Bắt đầu frame gửi |
| CMD | `0x02` | Lệnh đọc (Read) |
| ADDR | `0x04` | Đọc thanh ghi TWD_MS (0x04) |
| LEN | `0x00` | Không có data (Read request) |
| CHK | `0x53` | Checksum |

- Phân tích byte-by-byte frame nhận: AA 02 04 04 40 06 00 00 EC

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0xAA` | Bắt đầu frame nhận |
| CMD | `0x02` | Echo lại CMD = Read |
| ADDR | `0x04` | Echo lại ADDR = 0x04 |
| LEN | `0x04` | Độ dài dữ liệu (4 byte) |
| D0 | `0x40` | Dữ liệu byte 0 = 64 |
| D1 | `0x06` | Dữ liệu byte 1 = 6 |
| D2 | `0x00` | Dữ liệu byte 2 = 0 |
| D3 | `0x00` | Dữ liệu byte 3 = 0 |
| CHK | `0xEC` | Checksum |

- DATA = 0x00000640 = 1600 ms (giá trị mặc định).

---

#### 9.4.3. Lệnh 3: KICK (CMD = 0x03)

- Chức năng: Kick watchdog bằng phần mềm (dùng khi WDI_SRC = 1).

- Cấu trúc frame gửi:

```
55 | 03 | ADDR | 00 | CHK
```

- Frame nhận về:

```
AA | 03 | ADDR | 04 | DATA | CHK
```

- Ví dụ 1: Kick Watchdog
```
Gửi:  55 03 00 00 56
Nhận: AA 03 00 00 A9
```

- Phân tích byte-by-byte frame gửi: 55 03 00 00 56

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0x55` | Bắt đầu frame gửi |
| CMD | `0x03` | Lệnh kick (Kick) |
| ADDR | `0x00` | Ghi vào thanh ghi CTRL (0x00) |
| LEN | `0x00` | Không có data (Kick ACK) |
| CHK | `0x56` | Checksum |

- Phân tích byte-by-byte frame nhận: AA 03 00 00 A9

| Vị trí | Giá trị | Ý nghĩa |
| :---: | :---: | :---: |
| SOP | `0xAA` | Bắt đầu frame nhận |
| CMD | `0x03` | Echo lại CMD = Kick |
| ADDR | `0x00` | Echo lại ADDR = 0x00 |
| LEN | `0x00` | Không có data (Kick ACK) |
| CHK | `A9` | Checksum |

---

#### 9.4.4. Lệnh 4: STATUS (CMD = 0x04) — Shortcut

- Chức năng: Đọc trạng thái watchdog (status). Tương đương với READ 0x10 nhưng ngắn gọn hơn. FPGA tự động đọc thanh ghi STATUS mà không cần chỉ định địa chỉ.

- Cấu trúc frame gửi:

```
55 | 04 | 00 | 00 | CHK
```

- Frame nhận về:

```
AA | 04 | 00 | 04 | D0 D1 D2 D3 | CHK
```

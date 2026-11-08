# Phân tích mạch nguyên lý TI PMP23391

**Bộ nguồn DC-DC full-bridge đồng bộ, cách ly, 300 W (chịu bức xạ 100 kRad)**

> Tài liệu phân tích dựa trên 2 trang sơ đồ nguyên lý của thiết kế tham chiếu TI PMP23391 (Rev D, 03/13/2025), dùng cho bài tập vẽ lại trong Altium (Bài 9 – PMP391).
> Những chỗ ghi **(suy luận)** là phần mình đoán từ cách đấu nối trên sơ đồ, chưa đối chiếu datasheet.

---

## Mục lục

1. [Tổng quan](#1-tổng-quan)
2. [Sơ đồ khối và luồng tín hiệu](#2-sơ-đồ-khối-và-luồng-tín-hiệu)
3. [Hai miền đất PGND và SGND](#3-hai-miền-đất-pgnd-và-sgnd)
4. [Trang 1: Tầng công suất](#4-trang-1-tầng-công-suất)
5. [Trang 1: Bộ điều khiển U101](#5-trang-1-bộ-điều-khiển-u101)
6. [Trang 1: Truyền xung điều khiển qua barrier cách ly](#6-trang-1-truyền-xung-điều-khiển-qua-barrier-cách-ly)
7. [Trang 1: Cảm biến dòng sơ cấp](#7-trang-1-cảm-biến-dòng-sơ-cấp)
8. [Trang 2: Các mạch driver gate](#8-trang-2-các-mạch-driver-gate)
9. [Nguồn phụ cách ly](#9-nguồn-phụ-cách-ly)
10. [Bảng tra nhãn tín hiệu liên kết giữa hai trang](#10-bảng-tra-nhãn-tín-hiệu-liên-kết-giữa-hai-trang)
11. [Lưu ý khi vẽ lại trong Altium](#11-lưu-ý-khi-vẽ-lại-trong-altium)
12. [Điểm cần kiểm tra lại](#12-điểm-cần-kiểm-tra-lại)

---

## 1. Tổng quan

| Thông số | Giá trị |
|---|---|
| Điện áp vào | 22 – 36 V (danh định 28 V) |
| Điện áp ra | 12 V |
| Dòng ra tối đa | 25 A |
| Công suất | khoảng 300 W |
| Topology | Cầu đầy đủ (full-bridge) cách ly, biến áp thứ cấp center-tap, chỉnh lưu đồng bộ |
| Linh kiện đóng cắt | GaN FET EPC7018GC (100 V) ở cả phía sơ cấp và thứ cấp |
| Bộ điều khiển | TPS7H5005-SEP (đặt ở phía thứ cấp) |
| Driver gate | 3 × TPS7H6005 (U201, U202, U203) |
| Ghi chú | Thiết kế chịu bức xạ 100 kRad. Có nhãn "CAUTION HOT SURFACE" |

**Ý tưởng thiết kế:** bộ điều khiển nằm ở phía thứ cấp (cùng phía điện áp ra) nên mạch hồi tiếp điện áp không cần opto hay tham chiếu cách ly. Việc cách ly được thực hiện bằng:
- biến áp công suất T103,
- hai biến áp xung L102/L103 (truyền lệnh đóng cắt từ thứ cấp sang sơ cấp),
- biến áp dòng T102 (đưa tín hiệu dòng sơ cấp về phía thứ cấp).

---

## 2. Sơ đồ khối và luồng tín hiệu

```
                        (SGND – thứ cấp)                         (PGND – sơ cấp)

 VOUT ──► R124/R125/R126 ──► VSENSE ─┐
                                      ▼
 CS_ILIM ◄── R103 ◄── D101–D104 ◄── T102 (biến áp dòng, trong đường dòng sơ cấp)
     │
     ▼
 ┌────────────┐  OUTA/OUTB  ┌───────────────┐   OUTA/OUTB   ┌───────────────┐
 │ U101       ├────────────►│ L102/L103     ├──────────────►│ U201 / U202   │
 │ TPS7H5005  │             │ (biến áp xung)│               │ TPS7H6005     │
 │            │ SRA/SRB     └───────────────┘               └───────┬───────┘
 │            ├────────────────────────────┐                        │ VG_HOx, VS_HOx, VG_LOx
 └────────────┘                            ▼                        ▼
                                    ┌───────────────┐        ┌───────────────┐
                                    │ U203          │        │ Q101–Q104     │
                                    │ TPS7H6005     │        │ cầu GaN       │
                                    └──────┬────────┘        └───────┬───────┘
                                           │ VG_SRA, VG_SRB          │ SW_A, SW_B
                                           ▼                         ▼
                                    ┌───────────────┐        L301 ──► T103 (3:2)
                                    │ Q105 / Q106   │◄────────────────┘
                                    │ SR đồng bộ    │
                                    └──────┬────────┘
                                           ▼
                                      L101 ──► C115–C125, C301–C304 ──► VOUT
```

**Luồng hoạt động tóm tắt**
1. U101 so sánh VSENSE với tham chiếu nội, qua mạng bù ở chân COMP để tạo độ rộng xung.
2. U101 xuất OUTA/OUTB, đi qua biến áp xung L102/L103 sang phía sơ cấp.
3. U201/U202 khuếch đại thành tín hiệu gate cho cầu GaN Q101–Q104.
4. Cầu GaN tạo xung vuông, đi qua L301 vào cuộn sơ cấp T103.
5. Thứ cấp T103 được chỉnh lưu đồng bộ bởi Q105/Q106, do U203 lái theo SRA/SRB từ U101.
6. L101 và dãy tụ lọc tạo điện áp ra 12 V.

---

## 3. Hai miền đất PGND và SGND

| Miền | Gồm những gì |
|---|---|
| **PGND** (sơ cấp) | Đầu vào J101, cầu GaN Q101–Q104, U201, U202, phía thứ cấp của L102/L103 và Q109/Q110 |
| **SGND** (thứ cấp) | U101, chỉnh lưu đồng bộ Q105/Q106, U203, đầu ra J105, mạch hồi tiếp |

Hai miền này **không được nối chung**. Chúng chỉ liên lạc qua T103, T102, L102, L103 và nguồn bias cách ly (+12Vp và +12Vs).

---

## 4. Trang 1: Tầng công suất

### 4.1. Đầu vào

- J101 là cổng vào VIN. TP101, TP102 là các điểm đo.
- Tụ lọc đầu vào: C102, C103 (22 µF/50 V) và C107, C108 (10 µF/50 V).
- R108 (0,3 Ω) nằm trên đường VIN và được vẽ mờ trên sơ đồ, có thể là linh kiện tuỳ chọn (DNP) **(suy luận)**.

### 4.2. Cầu đầy đủ phía sơ cấp

| Nhánh | FET cao | FET thấp | Nút giữa |
|---|---|---|---|
| A | Q101 | Q102 | SW_A |
| B | Q103 | Q104 | SW_B |

- Cả 4 FET là **EPC7018GC** (GaN, 100 V).
- Tín hiệu gate (VG_HOA, VS_HOA, VG_LOA, VG_HOB, VS_HOB, VG_LOB) do trang 2 cung cấp.
- **L301 (110 nH)** nằm nối tiếp với cuộn sơ cấp T103. Cùng với điện cảm rò của biến áp (Lleak = 250 nH), nó cung cấp năng lượng để chuyển mạch mềm (ZVS) cho cầu **(suy luận)**.

### 4.3. Biến áp công suất T103 (CP-D1549-BZ)

- Tỉ số PRI:SEC = 3:2.
- Lpri = 22 µH, Lleak = 250 nH.
- Thứ cấp có **trung tính giữa** (chân 9), hai đầu ngoài là chân 11 và chân 7.

### 4.4. Chỉnh lưu đồng bộ và bộ lọc ngõ ra

- **Q105, Q106** (EPC7018GC) là hai FET SR, nối mỗi đầu thứ cấp (chân 11 và chân 7) xuống SGND.
- **D106, D107** (MBRS1100T3G) mắc song song với Q105/Q106, đóng vai trò diode dẫn trong thời gian chết.
- **Snubber RC:** R117/R119 (33 Ω) với C117 (1200 pF); R118/R120 (33 Ω) với C118 (1200 pF).
- **D109, D110** (MBRS1100T3G), C122 (0,1 µF) và R121/R301 (10 kΩ) tạo mạch dò đỉnh hoặc kẹp điện áp trên hai nút thứ cấp **(suy luận)**.
- **L101 (1,3 µH, 7443556130)** nối từ trung tính giữa của T103 tới VOUT.
- **Tụ ngõ ra:** C115, C116 (1 µF), C119, C120 (10 µF), C121, C123, C124, C125 (100 µF/25 V), cùng C301–C304 (4,7 µF/25 V).
- Nhãn "**2.06 Arms ripple**" cho biết dòng ripple mà dãy tụ ra phải chịu.
- Cổng ra J105, các điểm đo TP103, TP104.

### 4.5. Ước lượng duty cycle

Giả sử tỉ số 3:2 là tỉ lệ cuộn sơ cấp so với **mỗi nửa** thứ cấp, ta có Vout ≈ Vin × (2/3) × D.

| Vin | Duty D hiệu dụng (ước lượng) |
|---|---|
| 22 V | ≈ 0,82 |
| 28 V | ≈ 0,64 |
| 36 V | ≈ 0,50 |

Dải duty này hợp lý với dải vào 22–36 V. Nếu tỉ số 3:2 được hiểu theo cách khác thì các giá trị trên sẽ thay đổi.

---

## 5. Trang 1: Bộ điều khiển U101

**Linh kiện:** TPS7H5005MPWSEP, đặt ở phía thứ cấp (SGND), cấp nguồn bởi +12Vs.

### 5.1. Nguồn và điều khiển

| Chân | Linh kiện | Ghi chú |
|---|---|---|
| VIN (9) | C101 1 µF | Cấp từ +12Vs |
| VLDO (17) | C104 2,2 µF/16 V | Tụ cho LDO nội |
| EN (8) | J103 | Chọn hoặc cấp tín hiệu enable |

### 5.2. Các chân lập trình

| Chân | Linh kiện | Chức năng |
|---|---|---|
| RT | R105 = 205 kΩ | Đặt tần số đóng cắt |
| SS | C106 = 0,1 µF | Soft-start |
| LEB | R104 = 49,9 kΩ | Thời gian blanking cho cảm biến dòng |
| HICC | C105 = 5,6 nF | Mạch liên quan giới hạn dòng/hiccup |
| PS | R101 = 147 kΩ | Lập trình bằng điện trở |
| SP | R102 = 75 kΩ | Lập trình bằng điện trở |
| SYNC | R106 = 1 MΩ | Đồng bộ xung nhịp |
| REFCAP | C109 = 0,47 µF, R109 = 0 Ω | Tụ cho điện áp tham chiếu |
| RSC | R110 = 34,8 kΩ | Lập trình bằng điện trở |

> Chức năng chính xác của PS, SP, RSC, HICC và công thức tính tần số từ R105 cần tra datasheet TPS7H5005-SEP. Tài liệu này không tính ra giá trị tần số.

### 5.3. Phản hồi điện áp (VSENSE)

Chuỗi chia từ VOUT: `VOUT → R124 (10 Ω) → R125 (115 kΩ) → R126 (6,19 kΩ) → SGND`.

- Điện áp tại VSENSE ≈ 12 V × 6,19 / (115 + 6,19 + 0,01) ≈ **0,61 V**, phù hợp với mức tham chiếu cỡ 0,6 V.
- **R124 (10 Ω) và J106 (3 chân)** là điểm bơm tín hiệu để đo đáp ứng vòng (đồ thị Bode) **(suy luận)**.

### 5.4. Mạng bù (COMP)

- R112 = 30,1 kΩ, C111 = 6,8 nF, C112 = 120 pF.
- Cực thấp: f = 1 / (2π × 30,1 kΩ × 6,8 nF) ≈ **777 Hz**, khớp nhãn *fp,comp = 777 Hz* trên sơ đồ.
- Cực tần cao theo nhãn trên sơ đồ: *fp,hf = 35,3 kHz*.

### 5.5. Lọc cảm biến dòng (CS_ILIM)

- R111 = 1,5 kΩ và C110 = 10 pF tạo cực tại 1 / (2π × 1,5 kΩ × 10 pF) ≈ **10,61 MHz**, khớp nhãn trên sơ đồ.

### 5.6. Đầu ra điều khiển

- **OUTA, OUTB** (chân 10, 11): đi sang khối truyền xung cách ly (mục 6).
- **SRA, SRB** (chân 15, 14): đi qua mạng R114/C114/D108 và R113/C113/D105 để tạo trễ và định hình xung, rồi sang U203 ở trang 2. Đây nhiều khả năng là chỗ chỉnh dead-time cho SR **(suy luận)**.

---

## 6. Trang 1: Truyền xung điều khiển qua barrier cách ly

Có hai kênh giống hệt nhau cho OUTA và OUTB.

**Kênh A:** `OUTA → R115 (0 Ω) → Q107 (BSS138) + D111 → L102 (LPD4012-223MRB, 22 µH) → Q109 (MMBT2907A) + R122 (10 kΩ) + D113 (1N4148W) → nhãn OUTA`

**Kênh B:** tương tự với R116, Q108, D112, L103, Q110, R123, D114, ra nhãn OUTB.

- **L102/L103** là biến áp xung, truyền lệnh từ phía thứ cấp sang phía sơ cấp.
- **Q107/Q108 (BSS138, N-FET)** cùng D111/D112 giúp xả hoặc reset cuộn lái **(suy luận)**.
- **Q109/Q110 (MMBT2907A, PNP)** giúp tắt nhanh phía sơ cấp. Chúng nằm trên PGND.
- Hai nhãn OUTA/OUTB ở phía PGND được đưa vào U201 và U202 ở trang 2.

---

## 7. Trang 1: Cảm biến dòng sơ cấp

- **T102 (63554-R01)** là biến áp dòng đặt trong đường dòng sơ cấp, đưa tín hiệu về phía thứ cấp mà vẫn giữ cách ly.
- Phía thứ cấp của T102 qua cầu chỉnh lưu **D101–D104 (BAT46WJ, Schottky)**, tạo dòng một chiều.
- Dòng này chạy qua **R103 = 1,69 Ω** để thành điện áp tỉ lệ với dòng sơ cấp.
- Điện áp đó đi qua lọc RC (R111/C110) vào chân **CS_ILIM** của U101.
- D101 và D102 có nối tới nhãn VOUT, nên mình đoán đó là kẹp để reset lõi biến áp dòng **(suy luận)**.

---

## 8. Trang 2: Các mạch driver gate

### 8.1. Tổng quan ba driver

| IC | Nguồn / đất | Tín hiệu vào | Tín hiệu ra | Điều khiển |
|---|---|---|---|---|
| U201 | +12Vp / PGND | OUTA | VG_HOA, VS_HOA, VG_LOA | Nhánh A (Q101, Q102) |
| U202 | +12Vp / PGND | OUTB | VG_HOB, VS_HOB, VG_LOB | Nhánh B (Q103, Q104) |
| U203 | +12Vs / SGND | SRA, SRB | VG_SRA, VG_SRB | Q105, Q106 (SR phía thứ cấp) |

Cả ba là **TPS7H6005**, driver nửa cầu cho GaN FET. Mỗi IC có hai kênh (HO và LO).

### 8.2. U201 và U202: driver nhánh cầu sơ cấp

**Chế độ làm việc:** ghi chú trên sơ đồ nói chế độ PWM với DLH và DHL đều nối RES về AGND. R203/R205 = 100 kΩ cùng C305 (hoặc C306) = 100 pF ở hai chân này đặt **dead-time** giữa FET cao và FET thấp.

**Tín hiệu vào:**
- OUTA (hoặc OUTB) qua R204 (hoặc R217) = 100 Ω vào chân PWM_LI.
- R202 (hoặc R216) = 0 Ω nối nó tới EN_HI, nên hai chân vào được nối chung.
- R206 (hoặc R219) = 10 kΩ nối ở chân PGOOD.

**Cấp nguồn và bypass:**
- +12Vp qua R201 (hoặc R214) = 0 Ω vào VIN. C201 (hoặc C206) = 1 µF.
- BP7L có C202 (hoặc C207) = 1 µF.
- BP5L có C205 (hoặc C210) = 2,2 µF/16 V.

**Bootstrap cho FET phía cao (Q101 hoặc Q103):**
- R207 (hoặc R220) = 2 Ω và diode **D201 (hoặc D203) SS1H20LW** nạp tụ bootstrap C203/C204 (hoặc C208/C209).
- **D202 (hoặc D204) là Zener 13 V (MMSZ5243B-7-F)** kẹp điện áp BOOT để bảo vệ.
- Chân PSW/ASW nối vào nút VS_HOA (hoặc VS_HOB), tức nút giữa của nhánh cầu (SW_A hoặc SW_B ở trang 1).

**Đầu ra gate:**
- HOH và HOL qua hai điện trở **2,49 Ω** (R208/R209 hoặc R221/R222) rồi nối chung vào VG_HOA (hoặc VG_HOB).
- LOH và LOL qua R210/R211 (hoặc R223/R224) = 2,49 Ω vào VG_LOA (hoặc VG_LOB).
- R212/R213 (hoặc R225/R226) = 10 kΩ kéo gate xuống để giữ GaN FET ở trạng thái tắt khi chưa có xung.
- Việc tách hai điện trở cho đường kéo lên và kéo xuống cho phép chỉnh tốc độ bật và tắt riêng. Hiện hai giá trị bằng nhau.

### 8.3. U203: driver chỉnh lưu đồng bộ

- **Chế độ độc lập, tắt interlock** (theo ghi chú trên sơ đồ): DLH = BP5L, DHL = RES về AGND (R230 = 49,9 kΩ). Hai kênh điều khiển độc lập, không có dead-time cứng giữa chúng.
- **Tín hiệu vào:** SRA vào EN_HI, SRB vào PWM_LI, lấy từ U101.
- **Nguồn:** +12Vs qua R227, SGND, có C211, C212 (1 µF) và C213, C214 (2,2 µF/16 V).
- **BOOT nối thẳng với VIN**, không dùng bootstrap. Cả hai FET SR đều có source nối SGND nên không cần nâng áp cho kênh cao.
- **Đầu ra:** qua R231/R232 và R233/R234 (2,49 Ω), kéo xuống bằng R235/R236 (10 kΩ). Đầu ra là VG_SRA và VG_SRB, đi tới gate Q105 và Q106 ở trang 1.

---

## 9. Nguồn phụ cách ly

Khung vàng "Connections for isolated bias supply" trên trang 1 cho biết:

| Cổng | Vai trò |
|---|---|
| J102 (61300611121) | Cấp +12Vp và nối PGND (phía sơ cấp) |
| J104 (61300211121) | Cấp +12Vs và nối SGND (phía thứ cấp) |

Mạch này **không tự tạo bias**. Nguồn bias cách ly (+12Vp và +12Vs) phải được cấp từ bên ngoài.

---

## 10. Bảng tra nhãn tín hiệu liên kết giữa hai trang

| Nhãn | Từ | Đến |
|---|---|---|
| OUTA / OUTB | Q109/Q110 (trang 1) | U201 / U202 (trang 2) |
| SRA / SRB | U101 (trang 1) | U203 (trang 2) |
| VG_HOA, VS_HOA | U201 (trang 2) | Q101 (trang 1) |
| VG_LOA | U201 (trang 2) | Q102 (trang 1) |
| VG_HOB, VS_HOB | U202 (trang 2) | Q103 (trang 1) |
| VG_LOB | U202 (trang 2) | Q104 (trang 1) |
| VG_SRA | U203 (trang 2) | Q105 (trang 1) |
| VG_SRB | U203 (trang 2) | Q106 (trang 1) |
| SW_A, SW_B | Nút giữa hai nhánh cầu | T103, và VS_HOA/VS_HOB ở trang 2 |

---

## 11. Lưu ý khi vẽ lại trong Altium

- Khai báo **PGND và SGND là hai net khác nhau**, không nối chung.
- Với các tín hiệu đi giữa hai trang (mục 10), dùng **Port** hoặc **Off-Sheet Connector** có tên trùng nhau.
- Đặt **VS_HOA/VS_HOB cùng net với SW_A/SW_B**, hoặc nối bằng Net Label.
- Chân NC của TPS7H6005 nên đặt **No ERC marker** để Altium không báo lỗi.
- Footprint EPC7018GC, T102, T103, L101 cần tạo thư viện riêng. Vì đây là bài vẽ thực hành, không dùng để chế tạo, nên có thể ước lượng kích thước.
- Ghi chú trên sơ đồ gốc: các linh kiện có số hiệu trên 300 không in nhãn lụa (silkscreen).

---

## 12. Điểm cần kiểm tra lại

1. **SRA/SRB và VG_SRA/VG_SRB ở U203:** trên bản mình đọc, SRA vào EN_HI (kênh HO) nhưng nhãn đầu ra HO lại là VG_SRB, còn LO là VG_SRA. Trông như bị đảo chéo. Cần so với bản gốc để biết đây là chủ ý của TI hay do mình đọc ảnh chưa rõ.
2. **Chức năng các chân PS, SP, RSC, HICC, SYNC của U101** và công thức tính tần số từ R105: cần tra datasheet TPS7H5005-SEP.
3. **Dead-time:** giá trị thực tế của U201/U202 (R203/R205, C305/C306) và của mạng trễ SRA/SRB cần tra datasheet TPS7H6005 để tính cụ thể.
4. **Các mạch đã đánh dấu (suy luận)**: R108, D109/D110/C122, L301, J106, Q107/Q108, D101/D102 cần đối chiếu với tài liệu TI (PMP23391) nếu cần độ chắc chắn.
5. **Tỉ số 3:2 của T103:** nếu hiểu theo cách khác thì bảng duty ở mục 4.5 sẽ thay đổi.

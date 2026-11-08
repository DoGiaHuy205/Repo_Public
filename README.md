# Phân tích mạch nguyên lý TI PMP23391

**Bộ nguồn DC-DC Full-Bridge ZVS cách ly, chỉnh lưu đồng bộ, 300 W (thiết kế chịu bức xạ 100 kRad)**

> Tài liệu phân tích 2 trang sơ đồ nguyên lý của thiết kế tham chiếu TI PMP23391 (Rev D, 03/2025), phục vụ bài tập vẽ lại trong Altium (Bài 9 – PMP391).
> File PDF gốc có 4 trang: trang 1 và 2 là sơ đồ nguyên lý, trang 3 và 4 là thông báo miễn trừ trách nhiệm của TI.

**Quy ước mức độ chắc chắn** (đặt cạnh các nhận định quan trọng):

| Nhãn | Ý nghĩa |
|---|---|
| **[S]** | Nhìn thấy và xác nhận được trực tiếp từ sơ đồ |
| **[D]** | Đã đối chiếu datasheet hoặc trang thiết kế của TI |
| **[B]** | Theo BOM hoặc báo cáo thử nghiệm của TI do bạn tìm được, mình chưa tự mở được để kiểm chứng |
| **[?]** | Suy luận từ cấu trúc mạch, cần xác minh thêm |

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
9. [Nguồn phụ cách ly và đầu nối J102/J104](#9-nguồn-phụ-cách-ly-và-đầu-nối-j102j104)
10. [Bảng tra nhãn tín hiệu giữa hai trang](#10-bảng-tra-nhãn-tín-hiệu-giữa-hai-trang)
11. [Lưu ý khi vẽ lại trong Altium](#11-lưu-ý-khi-vẽ-lại-trong-altium)
12. [Điểm còn cần xác minh](#12-điểm-còn-cần-xác-minh)
13. [Nguồn tham khảo](#13-nguồn-tham-khảo)

---

## 1. Tổng quan

| Thông số | Giá trị |
|---|---|
| Điện áp vào | 22 – 36 V (danh định 28 V) **[S][D]** |
| Điện áp ra | 12 V, tối đa 25 A (≈ 300 W) **[S][D]** |
| Topology | Full-Bridge ZVS cách ly, thứ cấp center-tap, chỉnh lưu đồng bộ. TI phân loại là "Full Bridge – Resonant" **[D]** |
| Linh kiện đóng cắt | GaN FET EPC7018GC (100 V) ở cả sơ cấp và thứ cấp **[S]** |
| Bộ điều khiển | TPS7H5005-SEP, đặt ở phía thứ cấp (tham chiếu SGND) **[S][D]** |
| Driver gate | 3 × TPS7H6005-SEP (U201, U202, U203) **[S][D]** |
| Ghi chú | Linh kiện chọn theo yêu cầu bức xạ quỹ đạo GEO, có thể thay để phù hợp nhiệm vụ khác **[D]** |

**Ý tưởng thiết kế:** bộ điều khiển nằm ở phía thứ cấp, cùng phía với điện áp ra, nên mạch hồi tiếp điện áp không cần opto hay tham chiếu cách ly. Việc cách ly điện (galvanic isolation) được thực hiện bằng ghép từ:
- T103: biến áp công suất,
- L102/L103: biến áp xung truyền lệnh đóng cắt từ thứ cấp sang sơ cấp,
- T102: biến áp dòng đưa thông tin dòng sơ cấp về điều khiển.

Theo TI, hai driver phía sơ cấp (U201/U202) làm việc ở **PWM mode** để đạt ZVS ở tải cao **[D]**.

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
1. U101 so sánh VSENSE với tham chiếu nội (0,613 V **[D]**) và qua mạng bù ở chân COMP để tạo độ rộng xung.
2. U101 xuất OUTA/OUTB, đi qua biến áp xung L102/L103 sang phía sơ cấp.
3. U201/U202 khuếch đại thành tín hiệu gate cho cầu GaN Q101–Q104.
4. Cầu GaN tạo xung vuông, đi qua L301 vào cuộn sơ cấp T103.
5. Thứ cấp T103 được chỉnh lưu đồng bộ bởi Q105/Q106, do U203 lái theo SRA/SRB từ U101.
6. L101 và dãy tụ lọc tạo điện áp ra 12 V.

---

## 3. Hai miền đất PGND và SGND

PGND (sơ cấp) và SGND (thứ cấp) là **hai miền mass độc lập, cách ly điện (galvanic isolation), không có kết nối điện dẫn trực tiếp giữa chúng** **[S]**.

| Miền | Gồm những gì |
|---|---|
| **PGND** | Đầu vào VIN, cầu GaN Q101–Q104, U201, U202, phía thứ cấp của L102/L103, Q109/Q110 |
| **SGND** | U101, chỉnh lưu đồng bộ Q105/Q106, U203, đầu ra J105, mạch hồi tiếp |

Năng lượng và tín hiệu đi qua hàng rào cách ly nhờ **ghép từ**, không qua đường dẫn điện:

| Phần tử | Vai trò |
|---|---|
| T103 | Biến áp công suất, truyền năng lượng từ sơ cấp sang thứ cấp |
| T102 | Biến áp dòng, truyền thông tin dòng sơ cấp về mạch điều khiển |
| L102/L103 | Biến áp xung, truyền tín hiệu điều khiển từ SGND sang PGND |

+12Vp và +12Vs là **hai nguồn bias độc lập**, cấp từ bên ngoài qua J102 và J104. Chúng không tạo đường liên lạc điện giữa hai miền mass.

Lưu ý: cuộn phía thứ cấp của L102/L103 thuộc PGND, còn cuộn phía sơ cấp của chúng thuộc SGND.

---

## 4. Trang 1: Tầng công suất

### 4.1. Đầu vào

- J101 là cổng vào VIN. TP101, TP102 là các điểm đo.
- Tụ lọc đầu vào: C102, C103 (22 µF/50 V) và C107, C108 (10 µF/50 V) **[S]**.
- **R108 (0,3 Ω)** nằm trên đường VIN, được vẽ mờ với hoa văn gạch chéo (kiểu hiển thị thường dùng cho linh kiện không lắp). Theo BOM của TI, R108 có **số lượng lắp = 0**, tức là **không lắp** **[B]**.
- Cuộn sơ cấp của T102 mắc **song song** với R108 **[S]**. Vì R108 không lắp, toàn bộ dòng VIN chạy qua cuộn sơ cấp T102; R108 chỉ là tuỳ chọn của một biến thể thiết kế **[B]**.

### 4.2. Cầu đầy đủ phía sơ cấp

| Nhánh | FET cao | FET thấp | Nút giữa |
|---|---|---|---|
| A | Q101 | Q102 | SW_A |
| B | Q103 | Q104 | SW_B |

- Cả 4 FET là **EPC7018GC** (GaN, 100 V) **[S]**.
- Tín hiệu gate (VG_HOA, VS_HOA, VG_LOA, VG_HOB, VS_HOB, VG_LOB) do trang 2 cung cấp.
- **L301 (110 nH)** nối tiếp cuộn sơ cấp T103. Cùng với điện cảm rò của T103 (250 nH), nó tham gia quá trình chuyển mạch cộng hưởng. Điều kiện đạt ZVS còn phụ thuộc dòng tải, điện dung ra của FET và trình tự điều khiển, nên không thể quy riêng cho hai cuộn cảm này **[?]**.
- TI mô tả thiết kế đạt ZVS ở tải cao, và báo cáo TIDT424 có dạng sóng switch node ở 28 V và 36 V **[B]**. ZVS trên toàn dải tải, đặc biệt ở tải nhẹ, chưa được chứng minh bởi các dạng sóng đó. Muốn xác nhận cần đo V_DS và V_GS của từng FET.

### 4.3. Biến áp công suất T103 (CP-D1549-BZ)

- Tỉ số PRI:SEC = 3:2, Lpri = 22 µH, Lleak = 250 nH **[S]**. BOM của TI ghi T103 là Coilcraft CP-D1549-BZ với T/R = 3:2 **[B]**.
- Thứ cấp có **trung tính giữa** (chân 9), hai đầu ngoài là chân 11 và chân 7 **[S]**.
- Sơ đồ và BOM đều chưa nói rõ 3:2 là tỉ lệ giữa sơ cấp và toàn bộ thứ cấp hay giữa sơ cấp và từng nửa thứ cấp. Cần tra datasheet hoặc bản vẽ cuộn dây của Coilcraft **[?]**.

### 4.4. Chỉnh lưu đồng bộ và bộ lọc ngõ ra

- **Q105, Q106** (EPC7018GC) là hai FET SR, nối mỗi đầu thứ cấp xuống SGND, được lái bởi VG_SRA và VG_SRB **[S]**.
- **D106, D107** (MBRS1100T3G) mắc song song với Q105/Q106, nhiều khả năng đóng vai trò diode dẫn trong thời gian chết **[?]**.
- **Snubber RC:** C117 (1200 pF) mắc nối tiếp với cặp R117 // R119 (33 Ω // 33 Ω ≈ **16,5 Ω**), đặt giữa drain Q105 và SGND. Tương tự, C118 với R118 // R120 cho Q106 **[S]**. Mục đích theo cấu trúc mạch là hạn chế dao động và xung quá áp khi chuyển mạch. Hiệu quả và giá trị tối ưu cần đo dạng sóng thực tế.
- **D109/D110 (MBRS1100T3G), C122 (0,1 µF), R121/R301 (10 kΩ):**
  - Anode của D109 và D110 nối vào hai nút thứ cấp. Hai cathode nối chung vào một nút, nút này nối với C122 và hai điện trở 10 kΩ xuống SGND **[S]**.
  - Trên sơ đồ, nút này **không nối tới nơi nào khác** (không nối vào VOUT hay các tụ C301–C304) **[S]**.
  - Đây là cấu trúc **kẹp điện áp thứ cấp (secondary clamp)**: hai diode chỉnh lưu đỉnh xung quá áp trên hai nút thứ cấp vào C122, năng lượng được tiêu tán trong R121/R301. BOM ghi hai điện trở này là loại 2 W, và báo cáo thử nghiệm của TI có vùng nhiệt riêng tên "Secondary Clamp" và "Secondary Snubbers" **[B]**.
  - Hằng số thời gian xả ≈ C122 × (R121 // R301) = 0,1 µF × 5 kΩ ≈ 0,5 ms (tính từ giá trị trên sơ đồ).
  - Đường phóng năng lượng chi tiết của từng nhánh chưa xác nhận được **[?]**.
- **L101 (1,3 µH, 7443556130)** nối từ trung tính giữa T103 tới VOUT **[S]**.
- **Tụ ngõ ra:** C115, C116 (1 µF), C119, C120 (10 µF), C121, C123, C124, C125 (100 µF/25 V), cùng C301–C304 (4,7 µF/25 V) **[S]**. Nhãn "2.06 Arms ripple" cho biết dòng ripple tụ phải chịu.
- Cổng ra J105, điểm đo TP103, TP104.

### 4.5. Ước lượng duty cycle (theo giả thiết)

Cần phân biệt hai đại lượng:
- **Duty của từng ngõ ra điều khiển (OUTA hoặc OUTB):** với DCL nối SGND, giới hạn là 50% mỗi ngõ ra **[D]** (xem mục 5.2).
- **Duty hiệu dụng sau chỉnh lưu thứ cấp (D_eff):** là tổng thời gian dẫn của OUTA và OUTB trong một chu kỳ, tối đa 100%.

**Giả thiết:** 3:2 là tỉ lệ giữa sơ cấp và **mỗi nửa** thứ cấp, mô hình lý tưởng: Vout ≈ Vin × (2/3) × D_eff. Mô hình này chưa tính sụt áp linh kiện, điện cảm rò và L301, dead-time, và cách điều chế PWM.

| Vin | D_eff (ước lượng) | Duty mỗi ngõ ra ≈ D_eff / 2 |
|---|---|---|
| 22 V | ≈ 0,82 | ≈ 0,41 |
| 28 V | ≈ 0,64 | ≈ 0,32 |
| 36 V | ≈ 0,50 | ≈ 0,25 |

Đây chỉ là **ước lượng theo giả thiết tỉ số biến áp**, không phải thông số vận hành thực tế của PMP23391. Duty mỗi ngõ ra tính trên đều nhỏ hơn giới hạn 50%.

---

## 5. Trang 1: Bộ điều khiển U101

**Linh kiện:** TPS7H5005MPWSEP, đặt ở phía thứ cấp (SGND), cấp nguồn bởi +12Vs. Đây là bộ điều khiển PWM chế độ dòng, hai ngõ ra, có sẵn ngõ ra điều khiển chỉnh lưu đồng bộ **[D]**.

### 5.1. Nguồn và điều khiển

| Chân | Linh kiện | Ghi chú |
|---|---|---|
| VIN (9) | C101 1 µF | Cấp từ +12Vs |
| VLDO (17) | C104 2,2 µF/16 V | Tụ cho LDO nội **[D]** |
| EN (8) | J103 | Chọn hoặc cấp tín hiệu enable |
| REFCAP (20) | C109 0,47 µF | Tham chiếu nội 1,2 V cần tụ 470 nF **[D]** |
| FAULT (19) | Nối AVSS | Theo datasheet, nối FAULT với AVSS là tắt tính năng FAULT **[D]** |

### 5.2. Các chân lập trình (đã đối chiếu datasheet)

| Chân | Linh kiện | Chức năng |
|---|---|---|
| RT (1) | R105 = 205 kΩ | Điện trở về AVSS nghĩa là dùng dao động nội, đặt tần số **[D]** |
| PS (2) | R101 = 147 kΩ | Đặt dead-time từ lúc **FET sơ cấp tắt đến lúc SR bật** **[D]** |
| SP (3) | R102 = 75 kΩ | Đặt dead-time từ lúc **SR tắt đến lúc FET sơ cấp bật** **[D]** |
| LEB (4) | R104 = 49,9 kΩ | Thời gian blanking đầu xung, datasheet cho giá trị điển hình ≈ 50 ns **[D]** |
| HICC (5) | C105 = 5,6 nF | Đặt thời gian trễ giới hạn dòng từng chu kỳ và thời gian hiccup **[D]** |
| SYNC (6) | R106 = 1 MΩ | Khi RT là điện trở, SYNC **xuất** xung clock có tần số **gấp đôi tần số đóng cắt** **[D]** |
| DCL (7) | Nối SGND (AVSS) | Giới hạn duty tối đa **50% mỗi ngõ ra**. Chỉ ở cấu hình này OUTB và SRB mới hoạt động **[D]** |
| RSC (21) | R110 = 34,8 kΩ | Đặt độ dốc bù dốc (slope compensation) **[D]** |
| SS (22) | C106 = 0,1 µF | Soft-start **[D]** |

**Dead-time PS/SP lập trình theo điện trở:** công thức R_PS = R_SP = 1,207 × DT − 8,858 (R tính bằng kΩ, DT tính bằng ns) **[B]**. Công thức này tái tạo đúng hai điểm trong bảng đặc tính của datasheet (49,9 kΩ ≈ 50 ns và 107 kΩ ≈ 100 ns) **[D]**. Áp dụng cho mạch:
- PS = 147 kΩ cho DT_PS ≈ (147 + 8,858) / 1,207 ≈ **129 ns** (OUTx tắt đến khi SRx được phép bật).
- SP = 75 kΩ cho DT_SP ≈ (75 + 8,858) / 1,207 ≈ **69,5 ns** (SRx tắt đến khi OUTx được phép bật).

Đây là dead-time lập trình theo điện trở, không phải giá trị đo trên mạch.

### 5.3. Phản hồi điện áp (VSENSE)

Chuỗi chia từ VOUT: `VOUT → R124 (10 Ω) → R125 (115 kΩ) → R126 (6,19 kΩ) → SGND` **[S]**.

- Điện áp tại VSENSE ≈ 12 V × 6,19 / (115 + 6,19 + 0,01) ≈ **0,61 V**, khớp với tham chiếu nội 0,613 V của TPS7H500x **[D]**.
- **J106 (3 chân) và R124 (10 Ω)** nằm trên đường hồi tiếp, nhiều khả năng phục vụ đo đáp ứng vòng. Báo cáo TIDT424 có mục đồ thị Bode, với phase margin 94,22° tại 23,86 kHz và gain margin −18,37 dB tại 131,92 kHz **[B]**. Chân nào là injection, ground hay feedback cần đối chiếu pinout J106 với sơ đồ đo trong báo cáo **[?]**.

### 5.4. Mạng bù (COMP)

- R112 = 30,1 kΩ, C111 = 6,8 nF, C112 = 120 pF **[S]**.
- Cực thấp: 1 / (2π × 30,1 kΩ × 6,8 nF) ≈ **777 Hz**, khớp nhãn *fp,comp = 777 Hz*.
- Cực tần cao theo nhãn trên sơ đồ: *fp,hf = 35,3 kHz*.

### 5.5. Lọc cảm biến dòng (CS_ILIM)

R111 (1,5 kΩ) và C110 (10 pF) tạo cực tại 1 / (2π × 1,5 kΩ × 10 pF) ≈ **10,61 MHz**, khớp nhãn trên sơ đồ **[S]**.

### 5.6. Đầu ra điều khiển

- **OUTA, OUTB** (chân 10, 11): đi sang khối truyền xung cách ly (mục 6).
- **SRA, SRB** (chân 15, 14): đi qua mạng R114/C114/D108 và R113/C113/D105 rồi sang U203 ở trang 2. Chức năng của các mạng này (định hình xung, tạo trễ) là suy luận **[?]**.

---

## 6. Trang 1: Truyền xung điều khiển qua barrier cách ly

Hai kênh giống hệt nhau cho OUTA và OUTB.

**Kênh A:** `OUTA → R115 (0 Ω) → Q107 (BSS138) + D111 → L102 (LPD4012-223MRB, 22 µH) → Q109 (MMBT2907A) + R122 (10 kΩ) + D113 (1N4148W) → nhãn OUTA`

**Kênh B:** tương tự với R116, Q108, D112, L103, Q110, R123, D114, ra nhãn OUTB.

- **L102/L103** là biến áp xung truyền tín hiệu điều khiển qua cách ly **[S]**.
- Phía thứ cấp của L102/L103 và Q109/Q110 nằm trên PGND.
- Vai trò chính xác của Q107/Q108 (BSS138) cùng D111/D112 (reset/xả cuộn lái?) và của Q109/Q110 (tắt nhanh/chuyển mức?) chưa được xác nhận **[?]**.

---

## 7. Trang 1: Cảm biến dòng sơ cấp

Cách đấu theo sơ đồ phóng to **[S]**:

- **T102 (63554-R01)** là biến áp dòng. Cuộn sơ cấp (chân 2–1) mắc song song với R108 trên đường VIN, nên khi R108 không lắp thì dòng VIN đi qua cuộn này.
- Cuộn thứ cấp (chân 4–3) nối vào mạng bốn diode BAT46GWJ (D101–D104):
  - Datasheet BAT46WJ/BAT46GWJ: chân 1 là **cathode**, chân 2 là **anode**. Ký hiệu trên sơ đồ khớp: vạch cathode nằm phía chân 1 **[S][D]**.
  - D103 và D104: cathode (chân 1) nối vào một đầu cuộn thứ cấp, anode (chân 2) nối SGND.
  - D102: cathode nối vào nút cảm biến của R103, anode nối đầu cuộn chân 4.
  - D101: cathode nối nhãn **VOUT**, anode nối đầu cuộn chân 3.
- **R103 = 1,69 Ω** nối từ nút cảm biến xuống SGND. Nút này đi qua lọc R111/C110 vào chân **CS_ILIM** của U101.

**Diễn giải (đã chỉnh lại chiều diode theo datasheet):**
- R103 là điện trở burden, đổi dòng thứ cấp thành điện áp cảm biến dòng.
- Đây là mạng bốn diode quanh hai đầu cuộn thứ cấp, nhưng nhánh "dương" bị tách đôi: đầu chân 4 thoát qua D102 vào nút cảm biến (qua R103 xuống SGND), còn đầu chân 3 thoát qua D101 vào **VOUT**. Đường về chung là SGND qua D103/D104.
- Khi đầu chân 4 dương, dòng đi D102 → R103 → SGND → D103 và được đo. Khi đầu chân 3 dương, dòng đi D101 → VOUT → (tụ ngõ ra) → SGND → D104 và **không qua R103**. Vì vậy đây **không phải cầu chỉnh lưu toàn sóng thông thường** đưa cả hai nửa chu kỳ vào R103 **[S][D]**.
- Cách đấu này phù hợp với dòng sơ cấp một chiều dạng xung (dòng đầu vào của cầu): phần đo dòng đi qua R103, phần ngược chiều có thể là đường reset lõi biến áp dòng, trả năng lượng về VOUT **[?]**. Chiều nào của dòng sơ cấp ứng với đầu chân 4 dương còn phụ thuộc vào cực tính cuộn dây, cần kiểm tra thêm.
- Pinout K/A đã xác nhận bằng datasheet, nhưng vẫn nên đối chiếu số chân của symbol Altium và footprint PCB với số chân thực tế của linh kiện.
- Nhờ biến áp dòng, tín hiệu dòng sơ cấp được đưa sang miền SGND mà không tạo kết nối điện dẫn giữa hai miền mass.

---

## 8. Trang 2: Các mạch driver gate

### 8.1. Tổng quan ba driver

| IC | Nguồn / đất | Tín hiệu vào | Tín hiệu ra | Điều khiển |
|---|---|---|---|---|
| U201 | +12Vp / PGND | OUTA | VG_HOA, VS_HOA, VG_LOA | Nhánh A (Q101, Q102) |
| U202 | +12Vp / PGND | OUTB | VG_HOB, VS_HOB, VG_LOB | Nhánh B (Q103, Q104) |
| U203 | +12Vs / SGND | SRA, SRB | VG_SRA, VG_SRB | Q105, Q106 (SR phía thứ cấp) |

Cả ba là **TPS7H6005-SEP**, driver nửa cầu cho GaN FET, mỗi IC có hai kênh (HO và LO) **[D]**.

### 8.2. U201 và U202: driver nhánh cầu sơ cấp (PWM mode)

**Chế độ làm việc:** theo ghi chú trên sơ đồ và bảng chế độ trong datasheet **[S][D]**, PWM mode yêu cầu điện trở từ DLH về AGND và từ DHL về AGND. Ở chế độ này, EN_HI là chân enable và PWM_LI nhận tín hiệu PWM; IC tự tạo hai ngõ ra HO và LO bù nhau **[D]**.

**Dead-time (đã đối chiếu datasheet) [D]:**

| Chân | Khoảng dead-time được đặt | Công thức |
|---|---|---|
| DLH | LO tắt → HO bật | R_LH = 1,064 × T_DLH − 0,630 |
| DHL | HO tắt → LO bật | R_HL = 1,077 × T_DHL + 1,812 |

(R tính bằng kΩ, T tính bằng ns. Dải đặt khoảng 5 ns đến 100 ns.)

Với R203 = R205 = 100 kΩ (U201) và R215 = R218 = 100 kΩ (U202), công thức cho T_DLH ≈ 94,6 ns và T_DHL ≈ 91,2 ns. Đây là giá trị tính theo công thức lập trình, chưa phải giá trị đo trên mạch, và nằm sát giới hạn trên của dải đặt. Vai trò của C305/C306 (100 pF) tại các chân này chưa được xác nhận **[?]**.

**Tín hiệu vào:**
- OUTA (hoặc OUTB) qua R204 (hoặc R217) = 100 Ω vào chân PWM_LI **[S]**.
- R202 (hoặc R216) = 0 Ω nối nó tới EN_HI, nên hai chân vào được nối chung **[S]**.
- R206 (hoặc R219) = 10 kΩ nối ở chân PGOOD **[S]**.

**Cấp nguồn và bypass [S]:**
- +12Vp qua R201 (hoặc R214) = 0 Ω vào VIN. C201 (hoặc C206) = 1 µF.
- BP7L có C202 (hoặc C207) = 1 µF.
- BP5L có C205 (hoặc C210) = 2,2 µF/16 V.

**Bootstrap cho FET phía cao (Q101 hoặc Q103) [S]:**
- R207 (hoặc R220) = 2 Ω và diode **D201 (hoặc D203) SS1H20LW** nạp tụ bootstrap C203/C204 (hoặc C208/C209).
- **D202 (hoặc D204) là Zener 13 V (MMSZ5243B-7-F)**, nhiều khả năng kẹp điện áp bootstrap để bảo vệ **[?]**.
- Chân PSW/ASW nối vào nút VS_HOA (hoặc VS_HOB), tức nút giữa của nhánh cầu (SW_A hoặc SW_B ở trang 1).

**Đầu ra gate [S]:**
- HOH và HOL qua hai điện trở **2,49 Ω** (R208/R209 hoặc R221/R222) rồi nối chung vào VG_HOA (hoặc VG_HOB). LOH/LOL qua R210/R211 (hoặc R223/R224) = 2,49 Ω vào VG_LOA (hoặc VG_LOB).
- R212/R213 (hoặc R225/R226) = 10 kΩ kéo gate xuống để giữ GaN FET ở trạng thái tắt khi chưa có xung.
- Việc tách hai điện trở cho đường kéo lên và kéo xuống cho phép chỉnh tốc độ bật và tắt riêng. Hiện hai giá trị bằng nhau.

U202 có cấu trúc giống hệt U201, với các linh kiện tương ứng R214–R226, C206–C210, D203, D204.

### 8.3. U203: driver chỉnh lưu đồng bộ phía thứ cấp

- **Chế độ độc lập, tắt interlock** (independent input mode, interlock disabled): ghi chú trên sơ đồ nói DLH = BP5L và DHL = điện trở về AGND (R230 = 49,9 kΩ). Cấu hình này khớp bảng chế độ của datasheet **[S][D]**. Hai kênh được điều khiển độc lập, không có dead-time cứng giữa chúng.
- **Sai khác đã xác nhận:** R230 = 49,9 kΩ (BOM của TI cũng ghi giá trị này **[B]**), trong khi datasheet TPS7H6005-SEP quy định điện trở từ DHL xuống AGND trong chế độ này là 100–220 kΩ **[D]**. Khi vẽ lại giữ 49,9 kΩ theo sơ đồ gốc, không tự đổi. Muốn tái tạo mạch thật cần xác nhận với TI.
- **Tín hiệu vào:** SRA vào EN_HI, SRB vào PWM_LI, lấy từ U101 **[S]**.
- **Nguồn:** +12Vs qua R227, SGND, có C211, C212 (1 µF) và C213, C214 (2,2 µF/16 V) **[S]**.
- **BOOT nối thẳng với VIN**, không dùng bootstrap, vì cả hai FET SR đều có source nối SGND. Chân PSW/ASW nối SGND, nên kênh HO được dùng như kênh thứ hai tham chiếu SGND **[S]**.
- **Đầu ra:** qua R231/R232 và R233/R234 (2,49 Ω), kéo xuống bằng R235/R236 (10 kΩ) **[S]**.

> **Lưu ý (kết nối chéo, đã xác nhận trên sơ đồ [S]):**
>
> | Tín hiệu vào | Chân U203 | Kênh ra | Nhãn đầu ra |
> |---|---|---|---|
> | SRA | EN_HI (23) | HOH/HOL | **VG_SRB** (tới Q106) |
> | SRB | PWM_LI (24) | LOH/LOL | **VG_SRA** (tới Q105) |
>
> Tên đầu vào và tên đầu ra bị đảo chéo. **Khi vẽ lại phải giữ nguyên kết nối theo sơ đồ gốc, không tự đổi hai nhãn VG_SRA/VG_SRB.** Lý do TI thiết kế như vậy cần kiểm tra thêm cực tính cuộn thứ cấp T103, trình tự pha và dạng sóng SRA/SRB và VG_SRA/VG_SRB **[?]**.

---

## 9. Nguồn phụ cách ly và đầu nối J102/J104

Khung vàng "Connections for isolated bias supply" trên trang 1, theo sơ đồ phóng to **[S]**:

| Đầu nối | Chân | Kết nối |
|---|---|---|
| J102 (61300611121, 6 chân) | 1, 2 | **VIN** |
| | 3, 4, 6 | **PGND** |
| | 5 | **+12Vp** |
| J104 (61300211121, 2 chân) | 1 | **+12Vs** |
| | 2 | **SGND** |

Như vậy J102 không chỉ cấp bias phía sơ cấp mà còn đưa **VIN** vào mạch. Bộ nguồn này **không tự tạo bias**: +12Vp và +12Vs phải được cấp từ nguồn bias cách ly bên ngoài.

---

## 10. Bảng tra nhãn tín hiệu giữa hai trang

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
- Với **J102**, nhớ nối đúng: chân 1, 2 vào VIN, chân 3, 4, 6 vào PGND, chân 5 vào +12Vp.
- Giữ nguyên **kết nối chéo SRA/SRB ở U203** (mục 8.3).
- Footprint EPC7018GC, T102, T103, L101 cần tạo thư viện riêng. Vì đây là bài vẽ thực hành, không dùng để chế tạo, nên có thể ước lượng kích thước.
- Ghi chú trên sơ đồ gốc: các linh kiện có số hiệu trên 300 không in nhãn lụa (silkscreen).

---

## 12. Điểm còn cần xác minh

### 12.1. Đã giải quyết

| Nội dung | Kết quả |
|---|---|
| R108 | BOM ghi số lượng lắp = 0, nghĩa là không lắp **[B]** |
| D109/D110, C122, R121/R301 | Thuộc nhóm secondary clamp, năng lượng tiêu tán qua R121/R301 (2 W) **[B]** |
| D101–D104 | Pinout cathode/anode đã xác nhận bằng datasheet **[D]**. Chức năng: một nửa chu kỳ đo qua R103, nửa còn lại về VOUT |
| Dead-time PS/SP của U101 | Lập trình ≈ 129 ns (PS) và ≈ 69,5 ns (SP) **[B][D]** |
| Dead-time U201/U202 | Lập trình ≈ 94,6 ns (DLH) và ≈ 91,2 ns (DHL) **[D]** |
| R230 = 49,9 kΩ (U203) | Sai khác với khoảng 100–220 kΩ của datasheet đã xác nhận. BOM cũng ghi 49,9 kΩ |
| ZVS | TI đã thử nghiệm và công bố dạng sóng ở 28 V và 36 V **[B]** |

### 12.2. Còn mở

| Nội dung | Việc cần kiểm tra |
|---|---|
| Tỉ số T103 = 3:2 | Xác định là tỉ lệ với toàn bộ hay từng nửa thứ cấp (ảnh hưởng bảng duty ở 4.5) |
| Q107/Q108, Q109/Q110 | Chức năng tổng thể nằm trên đường biến áp xung L102/L103 từ U101 sang U201/U202. Trình tự reset và vai trò chuyển mức chưa xác nhận. Cần lần theo trạng thái khi khởi động và khi OUTA/OUTB đổi trạng thái |
| J106 | Cần pinout J106 đối chiếu với sơ đồ đo Bode trong báo cáo TIDT424 |
| U203 | Kết nối chéo SRA/SRB đã xác nhận trên sơ đồ. Lý do cần dạng sóng SRA/SRB và VG_SRA/VG_SRB, cùng cực tính cuộn thứ cấp T103 |
| D101–D104 | Đối chiếu mapping chân trong SchLib và PCB của Altium |
| Dead-time thực tế | Các giá trị ở trên là lập trình theo điện trở. Muốn có giá trị đo cần quan sát đồng thời gate cao và gate thấp ở tải và điện áp vào xác định |
| ZVS toàn dải tải | Cần đo V_DS và V_GS của từng FET. Tải nhẹ không nên mặc định đạt ZVS |

---

## 13. Nguồn tham khảo

- TI, trang thiết kế tham chiếu PMP23391: https://www.ti.com/tool/PMP23391
- TI, báo cáo thử nghiệm TIDT424: https://ti.com/document-viewer/lit/html/TIDT424
- TI, datasheet TPS7H5005-SEP / TPS7H500x-SEP (SLVSGG1A): https://ti.com/document-viewer/TPS7H5005-SEP/datasheet/GUID-9C38C994-FA63-4A5D-BB13-6B2F0A558776
- TI, datasheet TPS7H6005-SEP (SNOSDH4): https://www.ti.com/document-viewer/lit/html/SNOSDH4C/GUID-052194FF-73D3-464E-94FC-10A27024B838
- Nexperia, datasheet BAT46WJ (pinout: chân 1 cathode, chân 2 anode): https://radiolocman.com/datasheet/pdf.html?di=171445
- TI, schematic PMP23391 (TIDME07) và BOM (TIDME11): các mục đánh dấu **[B]** lấy từ kết quả bạn tìm được, mình chưa tự mở được hai tài liệu này

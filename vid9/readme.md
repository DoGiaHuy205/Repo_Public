# TÀI LIỆU CHI TIẾT VIDEO 9: THIẾT KẾ MODULE RTL AXI STREAM WRAPPER (TÍCH HỢP HỆ THỐNG ZYNQ)

Tài liệu này được biên soạn chi tiết dựa trên nội dung bài giảng **Video 10: The RTL Module of AXI Stream Wrapper**. Mục tiêu của bài là đóng gói lõi CNN Accelerator đã hoàn thiện ở bài trước thành một Module tương thích với chuẩn giao tiếp AXI Stream, phục vụ cho việc tích hợp lên hệ thống SoC Zynq (kết hợp vi xử lý ARM).

---

## MỤC LỤC

1. [Kiến trúc tổng thể tích hợp hệ thống Zynq (Zynq System Architecture)](#1-kiến-trúc-tổng-thể-tích-hợp-hệ-thống-zynq-zynq-system-architecture)
2. [Giao thức AXI Stream và Tín hiệu giao tiếp](#2-giao-thức-axi-stream-và-tín-hiệu-giao-tiếp)
3. [Thiết kế Logic cho AXI Stream Wrapper](#3-thiết-kế-logic-cho-axi-stream-wrapper)
   - [3.1. Phía ngõ vào (Input Side / Slave AXI Stream)](#31-phía-ngõ-vào-input-side--slave-axi-stream)
   - [3.2. Phía ngõ ra (Output Side / Master AXI Stream)](#32-phía-ngõ-ra-output-side--master-axi-stream)
4. [Xây dựng Testbench và Đánh giá độ chính xác (Accuracy Evaluation)](#4-xây-dựng-testbench-và-đánh-giá-độ-chính-xác-accuracy-evaluation)
5. [Tổng kết và Nguyên nhân suy giảm độ chính xác](#5-tổng-kết-và-nguyên-nhân-suy-giảm-độ-chính-xác)

---

## 1. Kiến trúc tổng thể tích hợp hệ thống Zynq (Zynq System Architecture)

Hệ thống Zynq được chia thành hai miền chính: **Processing System (PS)** (chứa chip ARM) và **Programmable Logic (PL)** (chứa FPGA). Dữ liệu ảnh sẽ được truyền từ bộ nhớ hệ thống vào bộ tăng tốc CNN phần cứng và trả kết quả về.

Sơ đồ kết nối chi tiết (Dựa trên hình ảnh tham khảo):

```text
+-------------------------------------------------------+
| Processing System (PS)                                |        +-------------+
|                                                       |        |             |
|   +-----------+             +----------------+        |        |     DDR     |
|   |           |             |                |<--------------->|    Memory   |
|   |  ARM CPU  |<----------->| DDR Controller |        |        |             |
|   |           |             |                |        |        +-------------+
|   +-----+-----+             +-------+--------+        |
|         | AXI_GP_0                  | AXI_HP_0        |
+---------|---------------------------|-----------------+
          |                           |
+---------|---------------------------|------------------------------------------+
|         v                           v                 Programmable Logic (PL)  |
|  +-------------+             +--------------+                                  |
|  |     AXI     |             |      AXI     |                                  |
|  | Interconnect|             | Interconnect |                                  |
|  +------+------+             +------+-------+                                  |
|         | AXI-Lite                  | AXI-Full                                 |
|         |                           |                                          |
|         |                   +-------v-------+   AXI-Stream    +-------------+  |
|         +------------------>|               |---------------->|   AXIS CNN  |  |
|                             |    AXI DMA    |                 | Accelerator |  |
|                             |               |<----------------|             |  |
|                             +---------------+                 +-------------+  |
+--------------------------------------------------------------------------------+
```

* **Luồng dữ liệu:** Điểm ảnh (Pixels) được lưu trữ tại bộ nhớ DDR (DDR Memory). Bộ AXI DMA sẽ đọc dữ liệu từ DDR thông qua giao thức Memory-Mapped (AXI-Full nối với AXI_HP_0) và chuyển đổi thành luồng dữ liệu (AXI-Stream) đẩy vào khối AXIS CNN Accelerator. 
* **Luồng điều khiển:** Vi xử lý ARM CPU cấu hình và điều khiển khối AXI DMA thông qua cổng AXI_GP_0, đi qua AXI Interconnect và sử dụng giao thức AXI-Lite.
* **Mục tiêu của Wrapper:** Bọc lõi CNN bằng một lớp giao tiếp AXI Stream chuẩn (tạo thành khối AXIS CNN Accelerator) để khối AXI DMA có thể hiểu và truyền/nhận dữ liệu được.

---

## 2. Giao thức AXI Stream và Tín hiệu giao tiếp

Module AXI Stream đóng vai trò là khối trung gian (Wrapper) có hai mặt (2 sides):
* **Input Side (Nhận dữ liệu từ DMA):** Tiền tố `s_axis_` (Slave).
* **Output Side (Gửi kết quả về DMA):** Tiền tố `m_axis_` (Master).

Mỗi mặt bao gồm 4 tín hiệu bắt buộc của chuẩn AXI Stream:
1. `tready`: Tín hiệu báo sẵn sàng nhận dữ liệu.
2. `tdata`: Đường truyền dữ liệu (Data bus).
3. `tvalid`: Tín hiệu báo dữ liệu trên bus đang hợp lệ.
4. `tlast`: Tín hiệu cờ báo gói dữ liệu cuối cùng của luồng (Stream packet).

**Quá trình bắt tay (Handshaking):** Dữ liệu chỉ được truyền thành công ở một chu kỳ xung nhịp khi CẢ HAI tín hiệu `tready` (bên nhận) và `tvalid` (bên gửi) đều ở mức logic 1.

---

## 3. Thiết kế Logic cho AXI Stream Wrapper

### 3.1. Phía ngõ vào (Input Side / Slave AXI Stream)
* Lõi CNN đóng vai trò là bên nhận, DMA là bên gửi.
* Khi hệ thống rảnh, Wrapper kích mức 1 cho `s_axis_tready`, báo cho DMA biết CNN đã sẵn sàng nhận ảnh.
* Sử dụng một **bộ đếm (Counter) / Sequencer** bắt đầu đếm khi phát hiện sườn lên (rising edge) của tín hiệu `s_axis_tvalid`.
* **Cơ chế điều khiển `tready`:**
  - Nhận đủ 784 pixels: Kéo `s_axis_tready` xuống mức 0 (D-assert) để báo DMA ngừng gửi dữ liệu, tránh mất mát dữ liệu do CNN đang bận tính toán. Điều kiện: Bộ đếm nằm trong khoảng từ `784` đến `1281`.
  - Hoàn thành tính toán: Sau khi chu kỳ đếm vượt qua `1281` (lõi CNN tính xong), `s_axis_tready` được kéo trở lại mức 1 để sẵn sàng nhận bức ảnh tiếp theo.

### 3.2. Phía ngõ ra (Output Side / Master AXI Stream)
* Lõi CNN tính xong sẽ gửi kết quả (Prediction) ngược về DMA.
* Vì kết quả chỉ là 1 giá trị duy nhất (nhãn từ 0-9), nên gói dữ liệu luồng (stream packet) này có độ dài bằng 1.
* Do đó, hai tín hiệu `m_axis_tvalid` và `m_axis_tlast` sẽ được **kích hoạt đồng thời cùng lúc**.
* Điều kiện kích hoạt: Khi bộ đếm đạt giá trị `1279` (thời điểm lõi CNN xuất ra valid output theo bài trước), Wrapper sẽ assert `tvalid` và `tlast` để đẩy kết quả ra ngoài.

---

## 4. Xây dựng Testbench và Đánh giá độ chính xác (Accuracy Evaluation)

Bài giảng xây dựng hai kịch bản Testbench để kiểm tra:

1. **Testbench 1 (Mô phỏng 2 ảnh):**
   * Truyền liên tiếp 2 gói ảnh vào Module.
   * Dạng sóng (Waveform) cho thấy Wrapper nhận thành công 2 ảnh, tự động ngắt `tready` trong lúc xử lý, và sau đó xuất ra 2 kết quả dự đoán (Prediction 1 và Prediction 2) thành công.

2. **Testbench 2 (Đánh giá độ chính xác với 1000 ảnh):**
   * File test vector (`input_1000.txt`) chứa 1000 ảnh ngẫu nhiên từ tập MNIST, kèm theo nhãn thực tế (label) được sắp xếp tuần tự.
   * Testbench tự động truyền 1000 ảnh vào hệ thống RTL tuần tự.
   * Ở mỗi kết quả đầu ra, Testbench tự động so sánh nhãn do phần cứng suy luận ra với nhãn thực tế. Nếu đúng, bộ đếm `accuracy` sẽ tăng lên 1.
   * **Kết quả mô phỏng:** In ra Console thông báo đạt **900/1000** ảnh chính xác $	o$ **Accuracy = 90%**.

---

## 5. Tổng kết và Nguyên nhân suy giảm độ chính xác

* Mô hình ban đầu huấn luyện trên phần mềm (PyTorch) có độ chính xác khoảng >92%. Tuy nhiên, trên mô phỏng RTL phần cứng, độ chính xác giảm xuống còn **90%**.
* **Nguyên nhân cốt lõi (Precision Loss):** Thiết kế phần cứng sử dụng kiến trúc **Fixed-point arithmetic (Số thực dấu phẩy tĩnh) / Lượng tử hóa Int8** để tối ưu hóa tài nguyên logic và tốc độ, thay vì sử dụng **Floating-point (Số thực dấu phẩy động)** như trên Python/PyTorch. Quá trình làm tròn và cắt xén bit này gây ra sự suy giảm độ chính xác nhẹ (khoảng 2%), đổi lại tốc độ suy luận nhanh hơn rất nhiều và tiêu thụ ít tài nguyên FPGA.

> **Tóm tắt:** Việc đóng gói thành công AXI Stream Wrapper hoàn thành quá trình phát triển IP Core cho mạng CNN, biến nó thành một khối chuẩn hóa (Standardized IP) sẵn sàng để kết nối đồ họa (Block Design) với lõi ARM Cortex-A9 trên FPGA Zynq (VD: kit PYNQ-Z2, Zybo) thông qua khối AXI DMA.

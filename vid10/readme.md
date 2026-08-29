# TÀI LIỆU CHI TIẾT VIDEO 10: THIẾT KẾ KHỐI HỆ THỐNG ZYNQ (BLOCK DESIGN OF ZYNQ SYSTEM)

Tài liệu này được biên soạn chi tiết dựa trên nội dung bài giảng **Video 11: Block Design of ZYNQ System**. Nội dung tập trung vào việc tạo đồ án Vivado, cấu hình phần cứng (Block Design) để kết nối module AXI Stream CNN Accelerator (đã thiết kế) với hệ thống Zynq Processing System (PS), và phân tích các báo cáo về tài nguyên, thời gian (Timing).

---

## MỤC LỤC

1. [Cấu hình phần cứng và Board mạch (Hardware Setup)](#1-cấu-hình-phần-cứng-và-board-mạch-hardware-setup)
2. [Sơ đồ khối hệ thống Zynq (Block Design)](#2-sơ-đồ-khối-hệ-thống-zynq-block-design)
3. [Cấu hình các IP Core và Bộ nhớ](#3-cấu-hình-các-ip-core-và-bộ-nhớ)
4. [Đánh giá thời gian và Tần số xung nhịp (Timing & Fmax)](#4-đánh-giá-thời-gian-và-tần-số-xung-nhịp-timing--fmax)
5. [Báo cáo tài nguyên sử dụng (Utilization Report)](#5-báo-cáo-tài-nguyên-sử-dụng-utilization-report)

---

## 1. Cấu hình phần cứng và Board mạch (Hardware Setup)

*   **Board mạch sử dụng:** Kria KV260 Vision AI Starter Kit (chạy framework **PYNQ**).
*   **Tính tương thích:** Người dùng có thể sử dụng bất kỳ board Zynq nào khác hỗ trợ PYNQ (VD: PYNQ-Z1, PYNQ-Z2, Zybo, Zedboard) mà không cần chỉnh sửa nhiều, miễn là dung lượng FPGA đủ để chứa lõi CNN (cần kiểm tra Resource Usage).
*   **Thành phần mã nguồn:** Bao gồm file Wrapper AXI Stream, lõi CNN Accelerator (được cấu thành từ các file `.v` trong các bài trước) và file Testbench dùng cho mô phỏng.

---

## 2. Sơ đồ khối hệ thống Zynq (Block Design)

Trong phần mềm Vivado, hệ thống được kết nối đồ họa (Block Design) với các thành phần chính sau:

```text
+-------------------------------------------------------------+
|                 Zynq UltraScale+ MPSoC (PS)                 |
+------------------------------+------------------------------+
|        M_AXI_HPM0_FPD        |   S_AXI_HP0_FPD / HP1_FPD    |
+--------------+---------------+---------------+--------------+
               | (AXI-Lite)                    ^ (AXI-Full)
               v                               |
+------------------------------+               |
|      AXI Interconnect        |               |
|      (ps8_0_axi_periph)      |               |
+--------------+---------------+               |
               |                               |
               v                               |
+----------------------------------------------+--------------+
|                         AXI DMA                             |
|  +-------------+     +-------------------+                  |
|  | S_AXI_LITE  |     | M_AXI_MM2S / S2MM +------------------+
|  +-------------+     +---------+---------+                  |
|                                |                            |
|  +-------------+     +---------+---------+                  |
|  | M_AXIS_MM2S |     | S_AXIS_S2MM       |                  |
+--+-------------+-----+---------+---------+------------------+
         | (Dữ liệu ảnh)             ^ (Dự đoán)
         v                           |
+-----------------+        +---------+---------+
| AXI Stream FIFO |        | AXI Stream FIFO   |
+--------+--------+        +---------+---------+
         |                           ^
         v                           |
+-------------------------------------------------------------+
|               AXI Stream CNN Accelerator Wrapper            |
+-------------------------------------------------------------+
```

*   **Zynq PS:** Là bộ não trung tâm (chip ARM), cấu hình và ra lệnh cho hệ thống.
*   **AXI DMA:** Giao tiếp chuyển đổi giữa Memory-Mapped (đọc/ghi từ DDR) và AXI Stream.
*   **AXI Stream Data FIFO:** Được bổ sung làm bộ đệm (buffer) ở cả đường vào và đường ra giữa DMA và CNN Accelerator, giúp dữ liệu truyền tải trơn tru, tránh tắc nghẽn (bottleneck).
*   **CNN Accelerator:** Bộ tăng tốc nhận diện ảnh do chúng ta tự thiết kế ở bài 10.

---

## 3. Cấu hình các IP Core và Bộ nhớ

*   **AXI DMA:** Được cấu hình tắt Scatter Gather (để dùng Direct Mode), với độ rộng dữ liệu `Memory Map Data Width` và `Stream Data Width` phù hợp với phần cứng.
*   **Address Editor (Memory Map):** Hệ thống phân bổ địa chỉ cơ sở (Base Address) cho các IP (đặc biệt là cổng AXI-Lite của DMA). Khi lập trình trên Python/PYNQ, CPU ARM sẽ dùng các địa chỉ này để ghi các thanh ghi điều khiển DMA.

---

## 4. Đánh giá thời gian và Tần số xung nhịp (Timing & Fmax)

Sau quá trình Tổng hợp (Synthesis) và Triển khai (Implementation), Vivado trả về báo cáo Timing:
*   **Yêu cầu xung nhịp (Target Clock):** Zynq PS cấp xung nhịp là $100	ext{ MHz}$ (tương đương chu kỳ $T_{clk} = 10	ext{ ns}$).
*   **Worst Negative Slack (WNS):** $+2.173	ext{ ns}$. Việc WNS có giá trị **dương** khẳng định thiết kế của chúng ta đáp ứng chuẩn timing, không bị vi phạm thời gian (Timing Met).
*   **Tính toán tần số tối đa (Fmax):** Dựa vào WNS, ta có thể tính được xung nhịp cao nhất mà lõi CNN có thể chịu đựng được:
    $$f_{max} = \frac{1}{T_{clk} - WNS_{setup}} = \frac{1}{10	ext{ ns} - 2.173	ext{ ns}} = 127.76	ext{ MHz}$$
    *(Như vậy thiết kế hoàn toàn an toàn khi chạy ở mức 100 MHz).*

---

## 5. Báo cáo tài nguyên sử dụng (Utilization Report)

Phần mềm Vivado cho phép xem chi tiết lượng tài nguyên FPGA (Flip-flops, Look-Up Tables, DSP Blocks, BRAM) bị tiêu tốn cho từng phân lớp mạng bên trong lõi Accelerator (như `conv1_layer`, `conv2_layer`, `fully_connected`, v.v.).

Điều này rất hữu ích để tối ưu phần cứng: Nếu muốn chạy trên một chip Zynq nhỏ hơn (VD: Z-7010 trên board Zybo), người thiết kế có thể nhìn vào báo cáo này để cân đối lại số lượng tham số, giảm kích thước buffer, hoặc tối ưu các phép nhân DSP.

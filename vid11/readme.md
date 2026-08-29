# TÀI LIỆU CHI TIẾT VIDEO 11: KIỂM THỬ CHỨC NĂNG VÀ HIỆU NĂNG (FUNCTIONAL AND PERFORMANCE TESTING)

Tài liệu này được biên soạn chi tiết dựa trên nội dung bài giảng **Video 12: Functional and Performance Testing**. Nội dung tập trung vào việc triển khai phần cứng đã thiết kế lên board mạch thực tế (Kria KV260) và sử dụng Jupyter Notebook trên môi trường PYNQ để kiểm thử chức năng nhận dạng ảnh cũng như so sánh hiệu năng giữa Hardware Accelerator (FPGA) và Software Inference (ARM CPU).

---

## MỤC LỤC

1. [Chuẩn bị môi trường PYNQ và Nạp Bitstream](#1-chuẩn-bị-môi-trường-pynq-và-nạp-bitstream)
2. [Kiểm thử chức năng phần cứng với ảnh Bitmap (Functional Testing)](#2-kiểm-thử-chức-năng-phần-cứng-với-ảnh-bitmap-functional-testing)
3. [Đánh giá hiệu năng và So sánh Hardware vs. Software (Performance Testing)](#3-đánh-giá-hiệu-năng-và-so-sánh-hardware-vs-software-performance-testing)
4. [Tính toán hệ số tăng tốc (Speedup Factor)](#4-tính-toán-hệ-số-tăng-tốc-speedup-factor)

---

## 1. Chuẩn bị môi trường PYNQ và Nạp Bitstream

*   **Kết nối hệ thống:** Board mạch Kria KV260 được kết nối mạng, cho phép người dùng truy cập vào môi trường Jupyter Notebook thông qua trình duyệt web trên máy tính.
*   **Chuẩn bị file:** Cần tải 3 file kết quả từ Vivado (ở Video 11) lên bo mạch:
    1.  `cnn_mnist.bit` (Bitstream - cấu hình phần cứng FPGA).
    2.  `cnn_mnist.tcl` (Chứa thông tin cấu hình Block Design).
    3.  `cnn_mnist.hwh` (Hardware handoff file).
*   **Lập trình Python trên PYNQ:**
    -   Sử dụng thư viện `pynq` để nạp bitstream: `overlay = Overlay('cnn_mnist.bit')`
    -   Khởi tạo đối tượng DMA để điều khiển truyền dữ liệu: `dma = overlay.axi_dma_0`
    -   Cấp phát bộ nhớ liên tục (contiguous memory) cho các buffer truyền và nhận dữ liệu qua DMA:
        -   `input_buffer = allocate(shape=(784,), dtype=np.uint8)`
        -   `output_buffer = allocate(shape=(1,), dtype=np.uint8)`

---

## 2. Kiểm thử chức năng phần cứng với ảnh Bitmap (Functional Testing)

Mục tiêu là kiểm tra xem lõi CNN trên FPGA có nhận dạng đúng ảnh thực tế hay không.

1.  **Đọc ảnh đầu vào:** Sử dụng thư viện `PIL.Image` để đọc một file ảnh Bitmap (ví dụ: ảnh chữ số '5').
2.  **Chuẩn bị dữ liệu:** Chuyển đổi ảnh thành mảng numpy 1 chiều gồm 784 phần tử (pixel) và copy vào `input_buffer`.
3.  **Truyền dữ liệu (DMA Transfer):**
    -   Kích hoạt DMA gửi dữ liệu từ PS xuống FPGA: `dma.sendchannel.transfer(input_buffer)`
    -   Kích hoạt DMA nhận kết quả từ FPGA về PS: `dma.recvchannel.transfer(output_buffer)`
    -   Lệnh `wait()` được gọi để đảm bảo quá trình truyền hoàn tất.
4.  **Kết quả:** In giá trị trong `output_buffer`. Trong video, hệ thống nhận dạng chính xác chữ số **5** và chữ số **4** từ các file ảnh Bitmap thử nghiệm.

---

## 3. Đánh giá hiệu năng và So sánh Hardware vs. Software (Performance Testing)

Để thấy được lợi ích của việc dùng FPGA, bài giảng thiết lập một bài kiểm tra so sánh thời gian suy luận (inference time) giữa việc chạy mô hình bằng phần mềm (Software) trên chip ARM và chạy bằng phần cứng (Hardware) trên FPGA.

### 3.1. Chạy trên Software (Zynq ARM Processor)
*   **Cài đặt:** Cần có thư viện `torch` (PyTorch) chạy trên Linux của hệ thống PYNQ.
*   **Thực thi:** Khởi tạo cấu trúc mạng CNN (conv1, conv2, fc) bằng mã Python, tải trọng số (weights) đã huấn luyện trước đó vào mô hình.
*   **Đo thời gian:** Đưa ảnh vào hàm `forward()` của mô hình PyTorch và sử dụng thư viện `time` để đo khoảng thời gian từ lúc bắt đầu đến khi kết thúc.

### 3.2. Chạy trên Hardware (FPGA Accelerator)
*   **Thực thi:** Lặp lại quy trình truyền DMA như phần 2.
*   **Đo thời gian:** Đo toàn bộ thời gian từ lúc bắt đầu gọi hàm transfer của DMA (đẩy dữ liệu ảnh) cho đến khi `dma.recvchannel.wait()` hoàn thành (nhận xong kết quả). Việc này bao gồm cả thời gian truyền dữ liệu qua AXI Stream và thời gian lõi CNN xử lý.

---

## 4. Tính toán hệ số tăng tốc (Speedup Factor)

*   Hệ số tăng tốc (Speedup Factor) được tính bằng công thức:
    $$Speedup = \frac{T_{software}}{T_{hardware}}$$
*   **Kết quả:** Mặc dù video không hiển thị con số chính xác trên màn hình console lúc chạy (phụ thuộc vào thời gian thực thi OS), nhưng thông thường việc offload tính toán mạng nơ-ron từ một vi xử lý ARM nhúng sang một lõi FPGA pipeline chuyên dụng sẽ đem lại mức tăng tốc (Speedup) rất lớn (thường từ vài chục đến hàng trăm lần).
*   **Lưu ý:** Thời gian đo đạc trên Software có thể bị dao động nhẹ do cơ chế lập lịch tiến trình (scheduling) của hệ điều hành Linux chạy trên chip Zynq ARM.

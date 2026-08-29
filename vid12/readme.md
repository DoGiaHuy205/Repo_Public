# TÀI LIỆU CHI TIẾT VIDEO 12: PHÁT TRIỂN BACKEND VÀ FRONTEND (BACKEND AND FRONTEND DEVELOPMENT)

Tài liệu này được biên soạn chi tiết dựa trên nội dung bài giảng **Video 13: Backend and Frontend Development**. Đây là phần cuối cùng của dự án, tập trung vào việc xây dựng một giao diện Web App nhúng (Embedded Web Application) để người dùng có thể tương tác trực tiếp với lõi CNN Accelerator trên FPGA một cách trực quan, bao gồm tính năng vẽ số, dự đoán và so sánh hiệu năng.

---

## MỤC LỤC

1. [Tổng quan về Giao diện Web Application (UI)](#1-tổng-quan-về-giao-diện-web-application-ui)
2. [Cấu trúc thư mục dự án Web](#2-cấu-trúc-thư-mục-dự-án-web)
3. [Phát triển Backend (Python + Flask)](#3-phát-triển-backend-python--flask)
4. [Phát triển Frontend (HTML + CSS + JS)](#4-phát-triển-frontend-html--css--js)
5. [Thực thi và Đánh giá tổng kết](#5-thực-thi-và-đánh-giá-tổng-kết)

---

## 1. Tổng quan về Giao diện Web Application (UI)

*   **Mục đích:** Thay vì sử dụng Jupyter Notebook khô khan, người dùng sẽ có một trang web trực quan.
*   **Chức năng chính:**
    *   **Vẽ số:** Một khung canvas (màu đen) để người dùng có thể dùng chuột/cảm ứng vẽ một chữ số bất kỳ (từ 0 đến 9).
    *   **Dự đoán (Predict):** Gửi nét vẽ xuống FPGA để nhận dạng và hiển thị kết quả.
    *   **So sánh hiệu năng:** Một biểu đồ cột (Bar chart) hiển thị trực tiếp sự chênh lệch thời gian thực thi (Execution time) giữa CPU (Software) và FPGA (Hardware), kèm theo hệ số tăng tốc (Speedup Factor).
    *   *Trong video, các ví dụ cho thấy FPGA tăng tốc độ suy luận lên khoảng **39x đến hơn 56x** lần so với CPU.*

---

## 2. Cấu trúc thư mục dự án Web

Ứng dụng web được chia thành hai phần chính: Backend (Flask) và Frontend (HTML/CSS/JS). Cấu trúc thư mục bắt buộc như sau:

```text
app_folder/
│
├── app.py                     # File Python chính chạy Backend (Flask)
├── cn_mnist.py                # Class mô hình PyTorch (Software Inference)
├── weights.pt                 # File trọng số (cho Software Inference)
│
├── templates/                 # Chứa các file giao diện
│   └── index.html             # Trang web chính (HTML)
│
└── static/                    # Chứa các tài nguyên tĩnh
    ├── css/                   # File giao diện (Bootstrap, Custom CSS)
    └── js/                    # Các file xử lý logic Frontend
        ├── bootstrap.min.js
        ├── d3.v3.min.js       # Thư viện D3.js dùng để vẽ biểu đồ cột
        ├── paper-core.min.js  # Thư viện Paper.js dùng để xử lý canvas vẽ
        └── my_script.js       # File xử lý sự kiện click, gọi API, vẽ biểu đồ
```

---

## 3. Phát triển Backend (Python + Flask)

*   **Framework sử dụng:** Flask (Một framework web nhẹ dành cho Python).
*   **File `app.py`:**
    *   Khởi tạo Board FPGA: Gọi hàm để nạp bitstream (`Overlay`) và cấp phát DMA input/output buffer tương tự như bài trước.
    *   Khởi tạo Software Model: Load mô hình PyTorch lên CPU.
    *   **Route `/`:** Render ra trang chủ `index.html`.
    *   **Route `/predict` (POST):** API quan trọng nhất.
        1. Nhận chuỗi ảnh (dưới định dạng Base64) từ Frontend gửi lên.
        2. Giải mã (decode) Base64 thành mảng pixel (array).
        3. Truyền mảng này vào FPGA (qua DMA) và đo thời gian xử lý.
        4. Truyền mảng này vào CPU (PyTorch) và đo thời gian xử lý.
        5. Đóng gói kết quả (Dự đoán, Thời gian CPU, Thời gian FPGA, Speedup) vào một chuỗi JSON và gửi trả lại cho Frontend.

---

## 4. Phát triển Frontend (HTML + CSS + JS)

*   **HTML & CSS (`index.html`):** Sử dụng hệ thống lưới (Grid system) của Bootstrap để chia màn hình thành 2 cột:
    *   *Cột trái:* Khung Canvas để vẽ, nút "Predict", nút "Clear" và text hiển thị kết quả.
    *   *Cột phải:* Khu vực dành cho biểu đồ so sánh hiệu năng.
*   **JavaScript (`my_script.js`):**
    *   Khởi tạo canvas cho phép người dùng dùng chuột vẽ nét màu trắng trên nền đen (sử dụng `paper.js`).
    *   Bắt sự kiện nút "Predict": Lấy dữ liệu hình ảnh từ canvas, chuyển thành chuỗi Base64, và gửi HTTP POST request lên backend (`/predict`).
    *   Nhận JSON kết quả từ Backend:
        *   Cập nhật text hiển thị số được dự đoán.
        *   Cập nhật biểu đồ cột (Sử dụng `D3.js`) để thể hiện trực quan thời gian chạy của CPU (cột cao) và FPGA (cột thấp).
        *   Cập nhật con số Speedup (VD: "42.5x").

---

## 5. Thực thi và Đánh giá tổng kết

*   **Cách chạy:** Mở terminal trên môi trường PYNQ (dùng lệnh `sudo` vì việc ghi vào FPGA cần quyền root) và chạy lệnh: `sudo python3 app.py`
*   **Truy cập:** Mở trình duyệt và truy cập vào địa chỉ IP của board mạng (VD: `http://192.168.1.151:5000`).
*   **Kết luận:** Thông qua giao diện web trực quan, chúng ta đã chứng minh được thiết kế lõi phần cứng (Hardware Accelerator) trên FPGA không chỉ hoạt động chính xác (nhận diện đúng các số được vẽ bằng tay) mà còn mang lại hiệu năng vượt trội so với việc chạy trên vi xử lý nhúng thông thường.

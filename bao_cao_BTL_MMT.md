
# BÁO CÁO BÀI TẬP LỚN
## MÔN: MẠNG MÁY TÍNH

---

| Thông tin | Chi tiết |
|-----------|---------|
| **Họ và tên** | Nguyễn Minh Hiếu |
| **Mã sinh viên** | B23DCDT090 |
| **Nhóm/Lớp** | Nhóm 06 – Lớp *(điền lớp 8/9/10)* |
| **Ngày nộp** | 29/04/2026 |

---

## I. GIỚI THIỆU ĐỀ TÀI

Thiết kế và cấu hình hệ thống mạng doanh nghiệp nhỏ trên Cisco Packet Tracer, bao gồm **4 mạng con** với đầy đủ các dịch vụ cơ bản: DNS, Web, Mail, FTP và DHCP.

**Địa chỉ mạng ban đầu:** `192.168.10.0/24`  
**Phương pháp chia subnet:** VLSM /26 (chia thành 4 subnet, mỗi subnet tối đa 62 host)

---

## II. SƠ ĐỒ MẠNG

```
                    ┌─────────────────┐
                    │   Router0 2811  │
                    │  Fa0/0 (trunk)  │
                    └────────┬────────┘
                             │
                    ┌────────┴────────┐
                    │   Switch0 Core  │
                    │  (SW0 – 2960)   │
                    └──┬──┬──┬──┬─────┘
                       │  │  │  │
          ┌────────────┘  │  │  └────────────┐
          │               │  │               │
    ┌─────┴──────┐  ┌─────┴──┴────┐  ┌───────┴────┐
    │  Switch1   │  │  Switch2    │  │  Switch3   │
    │ (VLAN 10)  │  │ (VLAN 20)   │  │ (VLAN 30)  │
    │Server Farm │  │ Khu vực A   │  │ Khu vực B  │
    └─────┬──────┘  └──────┬──────┘  └─────┬──────┘
          │                │               │
    ┌─────┴──────┐   ┌─────┴──────┐  ┌─────┴──────┐
    │5 Servers   │   │PC0,PC1,PC2 │  │PC3,PC4,PC5 │
    │DNS,Web,    │   │(DHCP auto) │  │(DHCP auto) │
    │Mail,FTP,   │   └────────────┘  └────────────┘
    │DHCP        │
    └────────────┘
                    ┌────────────┐
                    │  Switch4   │
                    │ (VLAN 40)  │
                    │ Khu vực C  │
                    └─────┬──────┘
                          │
                    ┌─────┴──────┐
                    │PC6,PC7,PC8 │
                    │(DHCP auto) │
                    └────────────┘
```

**Phương thức định tuyến:** Router-on-a-Stick (1 cổng Fa0/0 + 4 subinterface)

---

## III. BẢNG CHIA SUBNET

| VLAN | Tên mạng | Địa chỉ mạng | Subnet Mask | Gateway | Host range | Broadcast | Mục đích |
|------|----------|--------------|-------------|---------|------------|-----------|----------|
| 10 | ServerFarm | 192.168.10.0/26 | 255.255.255.192 | 192.168.10.1 | .2–.62 | 192.168.10.63 | Server Farm |
| 20 | KhuVucA | 192.168.10.64/26 | 255.255.255.192 | 192.168.10.65 | .66–.126 | 192.168.10.127 | Client khu A |
| 30 | KhuVucB | 192.168.10.128/26 | 255.255.255.192 | 192.168.10.129 | .130–.190 | 192.168.10.191 | Client khu B |
| 40 | KhuVucC | 192.168.10.192/26 | 255.255.255.192 | 192.168.10.193 | .194–.254 | 192.168.10.255 | Client khu C |

**Thông số /26:**
- Số bit mượn: 2 bit
- Số subnet: 4
- Số host/subnet: 62 host

---

## IV. BẢNG ĐỊA CHỈ IP CÁC THIẾT BỊ

### 4.1 Router0 (Subinterface)

| Interface | VLAN | IP Address | Subnet Mask | Chức năng |
|-----------|------|------------|-------------|-----------|
| Fa0/0.10 | 10 | 192.168.10.1 | 255.255.255.192 | Gateway VLAN 10 |
| Fa0/0.20 | 20 | 192.168.10.65 | 255.255.255.192 | Gateway VLAN 20 |
| Fa0/0.30 | 30 | 192.168.10.129 | 255.255.255.192 | Gateway VLAN 30 |
| Fa0/0.40 | 40 | 192.168.10.193 | 255.255.255.192 | Gateway VLAN 40 |

### 4.2 Server Farm (VLAN 10 – Static IP)

| Thiết bị | Tên miền | IP Address | Subnet Mask | Gateway | DNS |
|----------|----------|------------|-------------|---------|-----|
| Server0 | – | 192.168.10.10 | 255.255.255.192 | 192.168.10.1 | 192.168.10.10 |
| Server1 | www.ptit.edu.vn | 192.168.10.11 | 255.255.255.192 | 192.168.10.1 | 192.168.10.10 |
| Server2 | mail.ptit.edu.vn | 192.168.10.12 | 255.255.255.192 | 192.168.10.1 | 192.168.10.10 |
| Server3 | ftp.ptit.edu.vn | 192.168.10.13 | 255.255.255.192 | 192.168.10.1 | 192.168.10.10 |
| Server4 | – (DHCP) | 192.168.10.14 | 255.255.255.192 | 192.168.10.1 | 192.168.10.10 |

### 4.3 Client PC (DHCP – IP cấp tự động)

| Thiết bị | VLAN | IP nhận từ DHCP | Gateway | DNS |
|----------|------|-----------------|---------|-----|
| PC0 | 20 | 192.168.10.66 | 192.168.10.65 | 192.168.10.10 |
| PC1 | 20 | 192.168.10.67 | 192.168.10.65 | 192.168.10.10 |
| PC2 | 20 | 192.168.10.68 | 192.168.10.65 | 192.168.10.10 |
| PC3 | 30 | 192.168.10.130 | 192.168.10.129 | 192.168.10.10 |
| PC4 | 30 | 192.168.10.131 | 192.168.10.129 | 192.168.10.10 |
| PC5 | 30 | 192.168.10.132 | 192.168.10.129 | 192.168.10.10 |
| PC6 | 40 | 192.168.10.194 | 192.168.10.193 | 192.168.10.10 |
| PC7 | 40 | 192.168.10.195 | 192.168.10.193 | 192.168.10.10 |
| PC8 | 40 | 192.168.10.196 | 192.168.10.193 | 192.168.10.10 |

---

## V. CẤU HÌNH CÁC DỊCH VỤ

### 5.1 DNS Server (Server0 – 192.168.10.10)

**Dịch vụ:** DNS Service ON

| STT | Tên miền | Loại | IP trỏ đến |
|-----|----------|------|------------|
| 1 | www.ptit.edu.vn | A Record | 192.168.10.11 |
| 2 | mail.ptit.edu.vn | A Record | 192.168.10.12 |
| 3 | ftp.ptit.edu.vn | A Record | 192.168.10.13 |

> *(Chèn ảnh chụp DNS Server configuration)*

---

### 5.2 Web Server (Server1 – 192.168.10.11)

**Dịch vụ:** HTTP ON, HTTPS ON

**Nội dung file index.html:**
```html
<html>
<body>
<center>
<h1>Xin chào bạn, tôi là: Nguyễn Minh Hiếu, mã sinh viên là B23DCDT090</h1>
</center>
</body>
</html>
```

> *(Chèn ảnh chụp Web Server – HTTP service)*

---

### 5.3 Mail Server (Server2 – 192.168.10.12)

**Dịch vụ:** SMTP ON, POP3 ON  
**Domain:** ptit.edu.vn

| STT | Username | Password |
|-----|----------|----------|
| 1 | user1 | 123456 |
| 2 | user2 | 123456 |

> *(Chèn ảnh chụp Mail Server – EMAIL service)*

---

### 5.4 FTP Server (Server3 – 192.168.10.13)

**Dịch vụ:** FTP Service ON

| STT | Username | Password | Quyền |
|-----|----------|----------|-------|
| 1 | cisco | cisco | RWDNL |
| 2 | ftpuser | 123456 | RWDNL |

> *(Chèn ảnh chụp FTP Server configuration)*

---

### 5.5 DHCP Server (Server4 – 192.168.10.14)

**Dịch vụ:** DHCP Service ON  
**DHCP Relay:** ip helper-address 192.168.10.14 (cấu hình trên Router)

| Pool Name | Gateway | DNS | Start IP | Subnet Mask | Max Users |
|-----------|---------|-----|----------|-------------|-----------|
| VLAN20 | 192.168.10.65 | 192.168.10.10 | 192.168.10.66 | 255.255.255.192 | 50 |
| VLAN30 | 192.168.10.129 | 192.168.10.10 | 192.168.10.130 | 255.255.255.192 | 50 |
| VLAN40 | 192.168.10.193 | 192.168.10.10 | 192.168.10.194 | 255.255.255.192 | 50 |

---

## VI. KẾT QUẢ KIỂM TRA

### 6.1 PC0 Nhận IP Từ DHCP

| Thông số | Giá trị |
|----------|---------|
| IPv4 Address | 192.168.10.66 |
| Subnet Mask | 255.255.255.192 |
| Default Gateway | 192.168.10.65 |
| DNS Server | 192.168.10.10 |
| Trạng thái | **DHCP request successful** |

> *(Chèn ảnh chụp IP Configuration PC0)*

---

### 6.2 Kết Quả Ping

**PC0 ping Web Server (192.168.10.11) – Inter-VLAN qua Router:**

```
Packets: Sent = 4, Received = 3, Lost = 1 (25% loss)
TTL = 127 (xác nhận đi qua Router)
```

> Giải thích: Gói đầu tiên bị drop do ARP resolution, các gói sau Reply thành công. TTL=127 (128-1) xác nhận gói tin đã đi qua Router.

> *(Chèn ảnh chụp kết quả ping)*

---

### 6.3 Truy Cập Website

Từ **PC0** → Web Browser → `http://www.ptit.edu.vn`

**Kết quả:** Trang web hiển thị thành công với nội dung:

> *"Xin chào bạn, tôi là: Nguyễn Minh Hiếu, mã sinh viên là B23DCDT090"*

Xác nhận: DNS resolve đúng → HTTP kết nối thành công

> *(Chèn ảnh chụp Web Browser)*

---

### 6.4 Gửi/Nhận Email

**Gửi email từ PC0 (user1 → user2):**

```
Mail Server: mail.ptit.edu.vn
DNS resolved ip address: 192.168.10.12
Send Success.
```

**Nhận email tại PC0:**

```
Receiving mail from POP3 Server mail.ptit.edu.vn
DNS resolved ip address: 192.168.10.12
Receive Mail Success.
```

> *(Chèn ảnh chụp Mail Browser – gửi và nhận email)*

---

### 6.5 Đăng Nhập FTP

Từ **PC0** → Command Prompt:

```
C:\>ftp 192.168.10.13
Trying to connect...192.168.10.13
Connected to 192.168.10.13
220- Welcome to PT Ftp server
Username: ftpuser
331- Username ok, need password
Password: ******
230- Logged in
(passive mode On)
ftp>
```

> *(Chèn ảnh chụp FTP login)*

---

## VII. KẾT LUẬN

Bài tập đã thiết kế và cấu hình thành công hệ thống mạng doanh nghiệp nhỏ trên Cisco Packet Tracer với đầy đủ các thành phần:

| Yêu cầu | Kết quả |
|---------|---------|
| 4 mạng con /26 (VLAN 10/20/30/40) | ✅ Hoàn thành |
| Router-on-a-Stick inter-VLAN routing | ✅ Hoàn thành |
| DNS Server – 3 bản ghi A | ✅ Hoạt động |
| Web Server – www.ptit.edu.vn | ✅ Hiển thị trang cá nhân |
| Mail Server – SMTP/POP3 | ✅ Gửi nhận email thành công |
| FTP Server – xác thực tài khoản | ✅ Đăng nhập thành công |
| DHCP Server – cấp IP tự động | ✅ PC nhận IP đúng subnet |
| Kết nối liên VLAN qua Router | ✅ TTL=127 xác nhận |

Hệ thống hoạt động ổn định, đáp ứng đầy đủ các yêu cầu của đề bài.

---

*Báo cáo được thực hiện bởi: Nguyễn Minh Hiếu – B23DCDT090*

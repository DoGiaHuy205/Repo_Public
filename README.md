# Hướng dẫn cài đặt và thiết kế chương trình trên phần mềm Quartus. 

## A. Thực hiện cài  đặt Quartus và các file devices

**Bước 1** : Truy cập vào link : 

[Link download](https://www.intel.com/content/www/us/en/software-kit/669444/intel-quartus-prime-lite-edition-design-software-version-17-1-for-windows.html)

**Bước 2 : Chọn phiên bản theo hình dưới :**
 
![image](h1.png)

**Bước 3 : Chọn vào phần Individuals Files và cài những file sau :** 

![image](h2.png)

![image](h3.png)

![image](h4.png)

**Bước 4 : Cài đặt**  

> Di chuyển tất cả các file vừa tải vào cùng 1 thự mục sau đó tiến hành cài đặt 2 file :

> - ModelSim-Intel® FPGA Edition (includes Starter Edition)
> - Intel® Quartus® Prime (includes Nios® II EDS)

## B. Viết chương trình thiết kế và mô phỏng.

**Bước 1 :  Mở Quartus và  tạo project mới**

> Chọn vào new project wizard :

![image](h5.png)

> Chọn next :

![image](h6.png)


>  Chọn nơi lưu project , đặt tên cho project rồi bấm next:

![image](h7.png)

 > Chọn Empty project -> Chọn Next

![image](h8.png)

> Chọn Next

![image](h9.png)

> Chọn Board và Divice tương ứng 
> Tùy theo từng Board ta sẽ chọn Family và Board khác nhau. Ở đây lấy ví dụ cho Board DE10-Lite
> Nếu chưa nạp vào Board mà chỉ dùng để mô phỏng thì ta có thể chọn bất kỳ.

![image](h10.png)

> Chọn EDA Tool
> Ở mục Simulation chọn ModelSim-Altera

![image](h11.png)

> Chọn Finish

![image](h12.png)

> Giao diện sau khi hoàn thành : 

![image](h13.png)

**Bước 2 : Tạo file RTL** 

- Cách 1 : Tạo file mới : 

> Tổ hợp phím Ctrl + N  -> Sau đó chọn định dạng file là Verilog rồi code.

![image](h14.png)

> Từ đây chúng ta thực hiện quá trình viết code cho project.
> Ví dụ Code phần design cho Mạch AND 2 lối vào : 
```Verilog
module and2gate(
input a,
input b,
output c
);

assign c = a & b;

endmodule
```

> Lưu file -> Chọn vào Hierarchy -> Chọn file -> Chuột phải vào file -> Set as top-level entity.
> Lưu ý đặt tên file trùng với tên của module.

![image](h15.png)
![image](h16.png)

- Cách 2 : Add file có sẵn : 

> Chuột phải vào file : 

![image](h17.png)

> Chọn Add/Remove file 

![image](h18.png)

> Add

![image](h19.png)

> Sau đó chọn file có sẵn rồi bấm OK.

> Sau khi có file design  thì : 

> Click đúp vào Complie design :

![image](h20.png)

> Sau khi Comply thành công, Code không có lỗi thì giao hiện sẽ như sau : 

![image](h21.png)

> Xem nestlist : Chọn tool -> Nestlist viewer -> RTL viewer

![image](h22.png)

> Sau đó sẽ hiện ra cửa sổ nestlist : 

![image](h23.png)

**Bước 3 : Tạo file Testbench và mô phỏng.**

- Liên kết ModelSim vào Quartus : 

> Chọn tools -> Chọn Options

![image](h24.png)

> Cửa sổ hiện ra như sau : 

![image](h25.png)

> Chọn EDA Tool options -> Ở phần ModelSim-Alterta : Copy đường dẫn sau vào : `C:\intelFPGA\17.1\modelsim_ase\win32aloem`
> Chọn OK

> Lưu ý, đường dẫn trên là mặc định, nếu bạn lưu ModelSim ở nơi khác thì chỉ cần tìm đường dẫn dẫn tới win32aloem là được.

![image](h26.png)

- Tạo file Testbench và mô phỏng

> Tạo file Testbench : Tạo bình thường như tạo file design.
> Code Testbench cho mạch And 2 đầu vào : 
```Verilog
module tb;
reg a;
reg b;
wire c;

//instance 
and2gate dut(
	.a(a),
	.b(b),
	.c(c)
);

initial begin
	a = 0;
	b = 0;
	#10
	a = 0;
	b = 1;
	#10
	a = 1;
	b = 0;
	#10
	a = 1;
	b = 1;
        #10;
end 

endmodule 
```
> Sau khi tạo xong thì ADD file Testbench như sau : 
> Chọn Assignments -> Settings

![image](h27.png)

> Chọn Simulation -> Tích vào Complie test bench -> Test Benches 

![image](h28.png)

> Chọn theo thứ tự 1 -> 2 -> 3
![image](h29.png)

> Chọn file tb -> bấm OK

![image](h30.png)

> ADD -> OK

![image](h31.png)

> OK

![image](h32.png)

> Complie project

> Mở trình mô phỏng xem waveform : Chọn Tools -> Run Simulation tool -> RTL Simulation : 

![image](h33.png)

> Kết quả : 

![image](h34.png)

> Điều chỉnh Zoom để phóng to hoặc thu nhỏ Waveform
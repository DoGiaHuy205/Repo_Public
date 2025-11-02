Chào bạn, đây là phân tích và bình luận chi tiết toàn bộ code game "Rapid Roll" bằng Assembly.

Code này là một game hoàn chỉnh viết cho DOS, sử dụng chế độ đồ họa 13H (320x200, 256 màu). Dưới đây là giải thích từng phần một.

-----

## 🧭 Phần 1: Khai báo Mô hình, Stack và Dữ liệu (.DATA)

Phần này thiết lập môi trường và định nghĩa tất cả các biến mà chương trình sẽ sử dụng.

```assembly
.MODEL SMALL          ; Khai báo mô hình bộ nhớ SMALL (1 code segment, 1 data segment)
.STACK 100H           ; Dành 100H (256 bytes) cho stack (ngăn xếp)

.DATA                 ; Bắt đầu vùng khai báo dữ liệu (biến)
 
 ; --- Biến tọa độ cho khối vuông (người chơi) ---
 XCUBE DW 184          ; Tọa độ X ban đầu của khối vuông
 X1CUBE DW ?           ; Tọa độ X cuối (dùng để vẽ)
 X2CUBE DW ?           ; Tọa độ X gốc (dùng để xóa/vẽ lại)
 YCUBE DW 50           ; Tọa độ Y ban đầu
 Y1CUBE DW ?           ; Tọa độ Y cuối
 Y2CUBE DW ?           ; Tọa độ Y gốc
 
 ; --- Biến tọa độ cho thanh ngang 1 (LINE - Màu đỏ, trừ mạng) ---
 XLINE DW 100
 X1LINE DW ?
 X2LINE DW ?
 YLINE DW 190
 Y1LINE DW ?  
 Y2LINE DW ?
 
 ; --- Biến tọa độ cho thanh ngang 2 (NLINE - Màu Cyan) ---
 NXLINE DW 160 
 NX1LINE DW ?
 NX2LINE DW ?
 NYLINE DW 70 
 NY1LINE DW ?  
 NY2LINE DW ?
             
 ; --- Biến tọa độ cho thanh ngang 3 (NNLINE - Màu Magenta) ---
 NNXLINE DW 30
 NNX1LINE DW ?
 NNX2LINE DW ?
 NNYLINE DW 140
 NNY1LINE DW ?  
 NNY2LINE DW ?   
 
 ; --- Biến tọa độ cho thanh ngang 4 (NNNLINE - Màu Xanh lá, cộng mạng) ---
 NNNXLINE DW 230
 NNNX1LINE DW ?
 NNNX2LINE DW ?
 NNNYLINE DW 100
 NNNY1LINE DW ?  
 NNNY2LINE DW ?
             
 CHECK DW 1            ; Biến cờ (flag) dùng trong hàm DELAY, có vẻ để kiểm soát input
 
 RANDNUMBER DW 0         ; Biến lưu trữ số ngẫu nhiên (dùng để tạo vị trí X mới cho thanh)
 
 TIK DW ?              ; Biến lưu "tick" đồng hồ hệ thống (dùng cho DELAY)
 
 ; --- Các biến cờ (flag) kiểm tra va chạm ---
 CHECK_UND DW 0        ; Cờ = 1 nếu khối vuông đang ở trên thanh LINE
 NCHECK_UND DW 0        ; Cờ = 1 nếu khối vuông đang ở trên thanh NLINE
 NNCHECK_UND DW 0       ; Cờ = 1 nếu khối vuông đang ở trên thanh NNLINE
 NNNCHECK_UND DW 0      ; Cờ = 1 nếu khối vuông đang ở trên thanh NNNLINE
  
 RATE DW 1             ; (Không được sử dụng nhiều trong logic chính)
 
 ; --- Biến điểm số ---
 SCOREMSG DB 'Score: $'  ; Chuỗi thông báo (không được dùng, thay bằng MSG9)
 SCORE DW 0            ; Điểm số hiện tại
 SCORE1 DW 0            ; Giá trị điểm số = 0 (dùng để reset)
 HIGHSCORE DW 0         ; Điểm số cao nhất
 
 ; --- Biến mạng sống (Life) ---
 COUNT DW 3             ; Số mạng sống hiện tại
 COUNT1 DW 3             ; Giá trị mạng sống = 3 (dùng để reset)
 
 ; --- Biến cờ (flag) kiểm soát cộng/trừ mạng ---
 X DW 1                 ; Cờ = 1: cho phép trừ mạng (khi chạm thanh đỏ)
 Y DW 1                 ; Cờ = 1: cho phép cộng mạng (khi chạm thanh xanh)
 
 ; --- Biến tọa độ vẽ viền ---
 BORDERX DW 1
 BORDERY DW 16 
                
 ; --- Các chuỗi thông báo (Message) cho Menu và UI ---
  MSG DB 0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,'             RAPID ROLL$'  
 MSG1 DB 0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,'       PRESS 1 TO START THE GAME$'
 MSG2 DB 0AH,0DH,0AH,0DH,'       PRESS 2 FOR HELP$'
 MSG3 DB 0AH,0DH,0AH,0DH,'       PRESS 3 TO EXIT THE GAME$'
 MSG4 DB 0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,'           CSE 214 PROJECT$'   
 MSG5 DB 0AH,0DH,0AH,0DH,'   PREPARED BY SONET,NABIL,BAKIBILLAH$' 
 MSG6 DB 0AH,0DH,' LIFE REMAINING: 3$' ; (Không dùng)
 MSG7 DB 0AH,0DH,' LIFE REMAINING: 2$' ; (Không dùng)
 MSG8 DB 0AH,0DH,' LIFE REMAINING: 1$' ; (Không dùng)
 MSG9 DB '                SCORE: $' ; Dùng để in UI điểm số
 MSG10 DB 0AH,0DH,'LIFE REMAINING:$'  ; Dùng để in UI mạng sống
 
 ; --- Chuỗi hướng dẫn (Instructions) ---
 IMSG1 DB 0AH,0DH,' YOU HAVE A SMALL CUBE THAT FALLS ALONG'
      db 0AH,0DH,0AH,0DH,' THROUGH THE SCREEN. BY CONTINUING IN'
      db 0AH,0DH,0AH,0DH,' YOUR DESCEND THROUGH LEVELS OF THE '
      db 0AH,0DH,0AH,0DH,' GAME,YOU GAIN POINTS'
      db 0AH,0DH,0AH,0DH,0AH,0DH,' CONTROLS:'
      db 0AH,0DH,0AH,0DH,' PRESS A TO MOVE LEFT'
      db 0AH,0DH,0AH,0DH,' PRESS D TO MOVE RIGHT'
      db 0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,' PRESS ANY KEY TO GET BACK$'      

 ; --- Các chuỗi thông báo Game Over ---
  GAMEOVERMSG DB 0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,'              GAME OVER'
              DB 0AH,0DH,0AH,0DH,0AH,0DH,'            YOUR SCORE: $'
        
  GAMEOVERMSG1 DB 0AH,0DH,0AH,0DH,0AH,0DH,'         PRESS P TO PLAY AGAIN$'  
  
  GAMEOVERMSG2 DB 0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,0AH,0DH,'        YOU FORFEITED THE GAME$' 
  
  GAMEOVERMSG3 DB 0AH,0DH,0AH,0DH,0AH,0DH,'           PRESS X TO EXIT$'  
  
 ; --- Chuỗi thông báo Điểm cao nhất ---
  HIGHESTMSG  DB  0AH,0DH,0AH,0DH,'           HIGHEST SCORE: 100$' ; (Hardcoded, không dùng)
  HIGHESTMSG2 DB  0AH,0DH,0AH,0DH,0AH,0DH,'           HIGHEST SCORE: $' ; Dùng để in điểm cao
  
  SCORE_ARRAY DB ? ; (Không được sử dụng)
  
.CODE                 ; Bắt đầu vùng code (lệnh)
```

-----

## 🏛️ Phần 2: Các Thủ tục (PROC) - Hàm chức năng

Đây là các khối code được xây dựng để thực hiện các tác vụ cụ thể như vẽ, xóa, kiểm tra va chạm, tạo trễ...

### Các thủ tục Reset Game

```assembly
PLAY_AGAIN PROC   
    ; --- Đặt lại vị trí ban đầu của khối vuông và 4 thanh ngang khi chơi lại từ đầu ---
  MOV XCUBE , 184
  MOV YCUBE , 50
  MOV XLINE , 100
  MOV YLINE , 190
  MOV NXLINE , 160
  MOV NYLINE , 70
  MOV NNXLINE , 30
  MOV NNYLINE , 140
  MOV NNNXLINE , 230
  MOV NNNYLINE , 100
  RET                 ; Quay về nơi đã gọi
PLAY_AGAIN ENDP 

PLAY_AGAIN_2 PROC
  ; --- Đặt lại vị trí khối vuông sau khi mất 1 mạng (vị trí khác với lúc bắt đầu) ---
  MOV XCUBE , 36
  MOV YCUBE , 56
  RET
PLAY_AGAIN_2 ENDP
```

### Các thủ tục Vẽ và Xóa Khối vuông (Player)

```assembly
UPDRAWCUBE PROC 
    ; --- Vẽ khối vuông (7x7 pixel) ---
    MOV BX,XCUBE      ; Lấy tọa độ X hiện tại
    MOV X1CUBE,BX     ; X1 là X cuối (X + 7)
    MOV X2CUBE,BX     ; X2 là X đầu (để lưu lại)
    ADD X1CUBE,7      ; Đặt X cuối = X + 7
    
    MOV BX,YCUBE      ; Lấy tọa độ Y hiện tại
    MOV Y1CUBE,BX     ; Y1 là Y cuối (Y - 7)
    MOV Y2CUBE,BX     ; Y2 là Y đầu (để lưu lại)
    SUB Y1CUBE,7      ; Đặt Y cuối = Y - 7 (Vẽ từ dưới lên trên, vì Y giảm khi đi lên)

LUP: ; Vòng lặp vẽ
    MOV AH,0CH        ; Dịch vụ 0CH: Vẽ 1 pixel
    MOV AL,12         ; Màu 12 (Đỏ nhạt)
    MOV CX,XCUBE      ; Đặt tọa độ X (cột)
    MOV DX,YCUBE      ; Đặt tọa độ Y (hàng)
    INT 10H           ; Gọi ngắt BIOS Video
    INC XCUBE         ; Di chuyển X sang phải 1 pixel
    MOV BX,X1CUBE
    CMP XCUBE,BX      ; Đã vẽ hết hàng ngang (X == X+7) chưa?
    JLE LUP           ; Nếu chưa, lặp lại (vẽ pixel tiếp theo)
    ; --- Đã vẽ xong 1 hàng ngang ---
    MOV BX,X2CUBE
    MOV XCUBE,BX      ; Reset X về X ban đầu
    DEC YCUBE         ; Di chuyển Y lên trên 1 pixel
    MOV BX,Y1CUBE
    CMP YCUBE,BX      ; Đã vẽ hết (Y == Y-7) chưa?
    JNE LUP           ; Nếu chưa, lặp lại (vẽ hàng ngang tiếp theo)
    
    RET               ; Hoàn thành vẽ khối vuông
    
ENDP UPDRAWCUBE

; --- DOWNDRAWCUBE, UPRMVCUBE, DOWNRMVCUBE ---
; Các hàm này có logic tương tự UPDRAWCUBE:
; DOWNDRAWCUBE: Vẽ khối vuông (nhưng đi từ trên xuống)
; UPRMVCUBE: Xóa khối vuông (bằng cách vẽ lại với MÀU 0 - Đen)
; DOWNRMVCUBE: Xóa khối vuông (từ trên xuống, MÀU 0)

DOWNDRAWCUBE PROC 
    MOV BX,XCUBE
    MOV X1CUBE,BX
    MOV X2CUBE,BX
    ADD X1CUBE,7
    
    MOV BX,YCUBE
    MOV Y1CUBE,BX
    MOV Y2CUBE,BX
    ADD Y1CUBE,7      ; Khác biệt: Cộng Y (vẽ từ trên xuống)

LDOWN:
    MOV AH,0CH
    MOV AL,12         ; Màu 12
    MOV CX,XCUBE
    MOV DX,YCUBE
    INT 10H
    INC XCUBE
    MOV BX,X1CUBE
    CMP XCUBE,BX
    JLE LDOWN
    
    MOV BX,X2CUBE
    MOV XCUBE,BX
    INC YCUBE         ; Khác biệt: Tăng Y
    MOV BX,Y1CUBE
    CMP YCUBE,BX
    JNE LDOWN
    
    RET
    
ENDP DOWNDRAWCUBE

UPRMVCUBE PROC  
    ; --- Xóa khối vuông (vẽ màu 0 - Đen), logic như UPDRAWCUBE ---
    MOV BX,X2CUBE          
    MOV XCUBE,BX
    MOV BX,XCUBE
    MOV X1CUBE,BX
    MOV X2CUBE,BX
    ADD X1CUBE,7
    MOV BX,Y2CUBE
    MOV YCUBE,BX

L1:
    MOV AH,0CH
    MOV AL,0          ; Khác biệt: Màu 0 (Đen)
    MOV CX,XCUBE
    MOV DX,YCUBE
    INT 10H
    INC XCUBE
    MOV BX,X1CUBE
    CMP XCUBE,BX
    JLE L1
    
    MOV BX,X2CUBE
    MOV XCUBE,BX
    DEC YCUBE
    
    MOV BX,Y1CUBE
    CMP YCUBE,BX
    JNE L1
    
    MOV BX,X2CUBE
    MOV XCUBE,BX
    MOV BX,Y2CUBE
    MOV YCUBE,BX 
    RET
ENDP UPRMVCUBE 

DOWNRMVCUBE PROC 
    ; --- Xóa khối vuông (vẽ màu 0), logic như DOWNDRAWCUBE ---
    MOV BX,X2CUBE           
    MOV XCUBE,BX
    MOV BX,XCUBE
    MOV X1CUBE,BX
    MOV X2CUBE,BX
    ADD X1CUBE,7 
    MOV BX,Y2CUBE
    MOV YCUBE,BX

L1DOWN:
    MOV AH,0CH
    MOV AL,0          ; Khác biệt: Màu 0 (Đen)
    MOV CX,XCUBE
    MOV DX,YCUBE
    INT 10H
    INC XCUBE
    MOV BX,X1CUBE
    CMP XCUBE,BX
    JLE L1DOWN
    MOV BX,X2CUBE
    MOV XCUBE,BX
    INC YCUBE
    MOV BX,Y1CUBE
    CMP YCUBE,BX
    JNE L1DOWN
    
    MOV BX,X2CUBE
    MOV XCUBE,BX
    MOV BX,Y2CUBE
    MOV YCUBE,BX 
    RET
ENDP DOWNRMVCUBE 
```

### Các thủ tục Kiểm tra Va chạm

```assembly
CHECK_UP_OR_DOWN PROC  
    ; --- Kiểm tra va chạm giữa khối vuông (CUBE) và thanh (LINE) ---
    
    MOV BX,YLINE
    SUB BX,3
    CMP YCUBE,BX      ; Kiểm tra YCUBE có bằng YLINE-3 không?
    JE NEXTPHASE
    DEC BX     
    CMP YCUBE,BX      ; Kiểm tra YCUBE có bằng YLINE-4 không?
    JE  NEXTPHASE     ; Nếu Y trùng khớp -> kiểm tra X
    MOV CHECK_UND,0   ; Nếu Y không trùng, Cờ = 0 (không va chạm)
    JMP DID           ; Thoát
    
    NEXTPHASE:
    ; --- Đã trùng Y, giờ kiểm tra X ---
    MOV BX,XLINE
    ADD BX,57         ; X cuối của thanh (X + 55 + 2 pixel đệm)
    CMP XCUBE,BX      ; XCUBE có vượt quá bên phải thanh không?
    JL NEXTPHASE1     ; Nếu chưa, tiếp tục kiểm tra
    MOV CHECK_UND,0   ; Nếu vượt quá, Cờ = 0 (không va chạm)
    JMP DID           ; Thoát
    
    NEXTPHASE1:
    MOV BX,XLINE
    SUB BX,9          ; X đầu của thanh (X - 9 pixel đệm, tính cả chiều rộng khối)
    CMP BX,XCUBE      ; XCUBE có ở bên trái thanh không?
    JL LASTPHASE      ; Nếu không (tức là nằm trong), tiếp tục
    MOV CHECK_UND,0   ; Nếu ở bên trái, Cờ = 0 (không va chạm)
    JMP DID           ; Thoát
    
    LASTPHASE:
    ; --- Đã trùng cả X và Y ---
    MOV CHECK_UND,1   ; Đặt cờ = 1 (CÓ va chạm)
    
    DID:
    RET
ENDP CHECK_UP_OR_DOWN

; --- NCHECK_UP_OR_DOWN, NNCHECK_UP_OR_DOWN, NNNCHECK_UP_OR_DOWN ---
; Ba thủ tục này y hệt CHECK_UP_OR_DOWN, nhưng dùng để kiểm tra
; va chạm với các thanh NLINE, NNLINE, và NNNLINE,
; và cập nhật các cờ NCHECK_UND, NNCHECK_UND, NNNCHECK_UND.
; (Code được lặp lại 3 lần)

NCHECK_UP_OR_DOWN PROC
    ; ... (Logic y hệt, dùng NYLINE, NXLINE, NCHECK_UND) ...
ENDP NCHECK_UP_OR_DOWN

NNCHECK_UP_OR_DOWN PROC
    ; ... (Logic y hệt, dùng NNYLINE, NNXLINE, NNCHECK_UND) ...
ENDP NNCHECK_UP_OR_DOWN

NNNCHECK_UP_OR_DOWN PROC    
    ; ... (Logic y hệt, dùng NNNYLINE, NNNXLINE, NNNCHECK_UND) ...
ENDP NNNCHECK_UP_OR_DOWN
```

### Các thủ tục Delay (Tạo độ trễ)

```assembly
DELAY PROC    
    ; --- Tạo độ trễ (cấp 1, chậm nhất) ---
    MOV AX,00H
    INT 1AH           ; Ngắt 1AH: Lấy thời gian hệ thống
    MOV TIK,DX        ; Lưu "tick" đồng hồ (phần thấp) vào TIK
    ADD TIK,3H        ; Đặt thời gian mục tiêu = hiện tại + 3 ticks
   
  DELAYL:
    ; --- Vòng lặp chờ ---
    MOV AX,00H
    INT 1AH           ; Lấy thời gian hiện tại
    CMP TIK,DX        ; So sánh thời gian mục tiêu (TIK) với hiện tại (DX)
    JGE DELAYL        ; Nếu TIK >= DX (chưa đến giờ), lặp lại
    
    ; --- Phần kiểm tra input trong lúc chờ (có vẻ bị lỗi hoặc không cần thiết) ---
    CMP CHECK,0
    JE DDD
    MOV AH,7          ; Dịch vụ 7: Chờ nhập 1 phím (không echo)
    INT 21H
    DEC CHECK         ; Chỉ chạy 1 lần vì CHECK=1 và bị giảm
  
   DDD:
    RET
DELAY ENDP  

; --- DELAY2, DELAY3, DELAY4, DELAY5 ---
; Các hàm này y hệt DELAY, nhưng thời gian chờ khác nhau
; để tăng tốc độ game khi điểm cao.
; DELAY2: ADD TIK, 2H (Nhanh hơn)
; DELAY3: ADD TIK, 1H (Nhanh hơn nữa)
; DELAY4: ADD TIK, 0H (Gần như không trễ, chỉ 1 vòng lặp)
; DELAY5: SUB TIK, 1H (Chắc chắn không trễ)

DELAY2 PROC    
    MOV AX,00H
    INT 1AH
    MOV TIK,DX 
    ADD TIK,2H        ; Khác biệt: Trễ 2 ticks
...
DELAY2 ENDP       

DELAY3 PROC    
    MOV AX,00H
    INT 1AH
    MOV TIK,DX 
    ADD TIK,1H        ; Khác biệt: Trễ 1 tick
...
DELAY3 ENDP  

DELAY4 PROC    
    MOV AX,00H
    INT 1AH
    MOV TIK,DX 
    ; (Không cộng thêm, trễ rất ngắn)
...
DELAY4 ENDP 

DELAY5 PROC    
    MOV AX,00H
    INT 1AH
    MOV TIK,DX 
    SUB TIK,1H        ; (Trừ đi, đảm bảo không trễ)
...
DELAY5 ENDP 
```

### Các thủ tục Vẽ và Xóa Thanh ngang (Platforms)

```assembly
DRAWLINE PROC
    ; --- Vẽ thanh LINE (Màu 4 - Đỏ) ---
    ; (Logic vẽ hình chữ nhật 55x3 pixel, tương tự UPDRAWCUBE)
    MOV BX,XLINE
    MOV X1LINE,BX
    MOV X2LINE,BX
    ADD X1LINE,55     ; Chiều rộng 55
    
    MOV BX,YLINE
    MOV Y1LINE,BX
    MOV Y2LINE,BX
    SUB Y1LINE,3      ; Chiều cao 3 (vẽ từ dưới lên)
    
  LINE:
    MOV AH,0CH
    MOV AL,4          ; Màu 4 (Đỏ)
    MOV CX,XLINE
    MOV DX,YLINE
    INT 10H
    INC XLINE
    MOV BX,X1LINE
    CMP XLINE,BX
    JLE LINE
    
    MOV BX,X2LINE
    MOV XLINE,BX
    DEC YLINE
    MOV BX,Y1LINE
    CMP YLINE,BX
    JNE LINE
    
    RET 
DRAWLINE ENDP   
 
RMVLINE PROC 
    ; --- Xóa thanh LINE (Vẽ lại bằng màu 0 - Đen) ---
    ; (Logic tương tự DRAWLINE)
    MOV BX,XLINE
    MOV X1LINE,BX
    MOV X2LINE,BX
    ADD X1LINE,55
    MOV XLINE,BX
    MOV BX,Y2LINE
    MOV YLINE,BX

LINE1: 
    MOV AH,0CH
    MOV AL,0          ; Khác biệt: Màu 0 (Đen)
    MOV CX,XLINE
    MOV DX,YLINE
    INT 10H
    INC XLINE
    MOV BX,X1LINE
    CMP XLINE,BX
    JLE LINE1 
    
    MOV BX,X2LINE
    MOV XLINE,BX
    DEC YLINE
    MOV BX,Y1LINE
    CMP YLINE,BX
    JNE LINE1
    
    MOV BX,X2LINE
    MOV XLINE,BX
    MOV BX,Y2LINE
    MOV YLINE,BX 
    RET
RMVLINE ENDP  

; --- DRAWNLINE, RMVNLINE, DRAWNNLINE, RMVNNLINE, DRAWNNNLINE, RMVNNNLINE ---
; Các thủ tục này y hệt DRAWLINE và RMVLINE, chỉ khác:
; - Dùng biến (NXLINE, NNXLINE, NNNXLINE...)
; - Dùng màu khác nhau:
;   - NLINE: Màu 3 (Cyan)
;   - NNLINE: Màu 13 (Magenta)
;   - NNNLINE: Màu 2 (Xanh lá)
; (Code được lặp lại 3 lần)
DRAWNLINE PROC
    ; ... (Logic y hệt, dùng NXLINE, NYLINE, Màu 3) ...
ENDP DRAWNLINE

RMVNLINE PROC
    ; ... (Logic y hệt, dùng NXLINE, NYLINE, Màu 0) ...
ENDP RMVNLINE

DRAWNNLINE PROC
    ; ... (Logic y hệt, dùng NNXLINE, NNYLINE, Màu 13) ...
ENDP DRAWNNLINE

RMVNNLINE PROC
    ; ... (Logic y hệt, dùng NNXLINE, NNYLINE, Màu 0) ...
ENDP RMVNNLINE

DRAWNNNLINE PROC
    ; ... (Logic y hệt, dùng NNNXLINE, NNNYLINE, Màu 2) ...
ENDP DRAWNNNLINE

RMVNNNLINE PROC
    ; ... (Logic y hệt, dùng NNNXLINE, NNNYLINE, Màu 0) ...
ENDP RMVNNNLINE
```

### Các thủ tục Logic Game (Tạo số ngẫu nhiên, Di chuyển thanh)

```assembly
GENERATE_RANDOM_NUMBER PROC
    ; --- Tạo số ngẫu nhiên từ 0-199 ---
    
  PUSHALL MACRO       ; Macro (định nghĩa) để lưu các thanh ghi
    PUSH AX
    PUSH BX
    PUSH CX
    PUSH DX
  ENDM

  POPALL MACRO        ; Macro (định nghĩa) để khôi phục các thanh ghi
    POP DX 
    POP CX
    POP BX
    POP AX
  ENDM
    
  GETRAND MACRO CUR 
    ; --- Macro (định nghĩa) để tính số ngẫu nhiên ---
    PUSHALL
    MOV AH,0
    INT 1AH           ; Lấy thời gian hệ thống (CX:DX)
    
    MOV AX,DX         ; Dùng phần thấp của đồng hồ
    MOV DX,CX         ; (Phần này có vẻ không dùng DX:AX)
    
    ; --- Thuật toán LCG (Linear Congruential Generator) đơn giản ---
    MOV BX,7261
    MUL AX            ; AX = AX * 7261 (Kết quả trong DX:AX)
    ADD AX,1          ; (DX:AX) = (time * 7261) + 1
    MOV DX,0          ; Bỏ qua phần cao (chỉ dùng AX)
    MOV BX,200
    DIV BX            ; Chia AX cho 200. AX = Thương, DX = Số dư
    
    MOV CUR,DX        ; Lưu số dư (0-199) vào biến CUR
    POPALL
  ENDM   
    
    MOV CX,0
    GETRAND RANDNUMBER ; Gọi macro, lưu kết quả vào RANDNUMBER
    
    RET    
GENERATE_RANDOM_NUMBER ENDP     

NEXT_XLINE PROC
    ; --- Cập nhật vị trí thanh LINE khi nó đi qua đỉnh màn hình ---
          
    CMP YLINE,18      ; So sánh Y hiện tại với mốc 18 (gần đỉnh)
    JGE NOCHANGE      ; Nếu Y >= 18 (vẫn còn trên màn hình), không làm gì
    ; --- Nếu Y < 18 (đã đi qua đỉnh) ---
    MOV YLINE,196     ; Đặt lại Y về gần cuối màn hình
    MOV Y2LINE,196    ; Cập nhật Y gốc
    
    MOV BX,RANDNUMBER ; Lấy số ngẫu nhiên đã tạo
    MOV XLINE,BX      ; Đặt X mới cho thanh
    MOV X1LINE,BX     ; Cập nhật X cuối
    
    NOCHANGE:
    RET
NEXT_XLINE ENDP 

; --- NEXT_NXLINE, NEXT_NNXLINE, NEXT_NNNXLINE ---
; Ba thủ tục này y hệt NEXT_XLINE, nhưng dùng cho 3 thanh còn lại
; (NLINE, NNLINE, NNNLINE)
; (Code được lặp lại 3 lần)

NEXT_NXLINE PROC
    ; ... (Logic y hệt, dùng NYLINE, NXLINE, RANDNUMBER) ...
ENDP NEXT_NXLINE

NEXT_NNXLINE PROC
    ; ... (Logic y hệt, dùng NNYLINE, NNXLINE, RANDNUMBER) ...
ENDP NEXT_NNXLINE

NEXT_NNNXLINE PROC
    ; ... (Logic y hệt, dùng NNNYLINE, NNNXLINE, RANDNUMBER) ...
ENDP NEXT_NNNXLINE
```

### Các thủ tục Giao diện (UI) và Hệ thống

```assembly
BORDER PROC
    ; --- Vẽ 4 cạnh viền (Màu 6 - Nâu) ---
    
    BOR: ; Vẽ cạnh trên
    MOV AH,0CH
    MOV AL,6          ; Màu 6 (Nâu)
    MOV CX,BORDERX
    MOV DX,BORDERY    ; Y = 16
    INT 10H
    INC BORDERX
    CMP BORDERX,318
    JE NNP
    JMP BOR
    
    NNP: ; Vẽ cạnh dưới
    MOV BORDERX,1
    MOV BORDERY,198   ; Y = 198
    BOR1:
    MOV AH,0CH
    MOV AL,6
    MOV CX,BORDERX
    MOV DX,BORDERY
    INT 10H
    INC BORDERX
    CMP BORDERX,318
    JE NNP1
    JMP BOR1
    
    NNP1: ; Vẽ cạnh trái
    MOV BORDERX,1     ; X = 1
    MOV BORDERY,16
    BOR2:
    MOV AH,0CH
    MOV AL,6
    MOV CX,BORDERX
    MOV DX,BORDERY
    INT 10H
    INC BORDERY
    CMP BORDERY,198
    JE NNP2
    JMP BOR2
    
    NNP2: ; Vẽ cạnh phải
    MOV BORDERX,318   ; X = 318
    MOV BORDERY,16
    BOR3:
    MOV AH,0CH
    MOV AL,6
    MOV CX,BORDERX
    MOV DX,BORDERY
    INT 10H
    INC BORDERY
    CMP BORDERY,198
    JG DADA
    JMP BOR3
    
    DADA:
    MOV BORDERX,1     ; Reset biến viền (chuẩn bị cho lần gọi sau)
    MOV BORDERY,16
    RET 
BORDER ENDP 

MAIN_MENU PROC
    ; --- Hiển thị Menu chính (ở chế độ Text) ---
  MPR:
    ; --- In các chuỗi menu ---
    MOV AH,9
    LEA DX,MSG        ; In tiêu đề "RAPID ROLL"
    INT 21H
    LEA DX,MSG1       ; In "1. Start"
    INT 21H
    LEA DX,MSG2       ; In "2. Help"
    INT 21H
    LEA DX,MSG3       ; In "3. Exit"
    INT 21H
    LEA DX,MSG4
    INT 21H
    LEA DX,MSG5
    INT 21H
    
  LL1:
    ; --- Chờ người dùng nhập phím ---
    MOV AH,7          ; Dịch vụ 7: Đọc 1 phím (không echo)
    INT 21H
    CMP AL,'1'
    JE  STG           ; Nhấn '1' -> STG (Start Game)
    CMP AL,'2'
    JE  INSTRUC       ; Nhấn '2' -> INSTRUC (Instructions)
    CMP AL,'3'
    JE  EXIT1         ; Nhấn '3' -> EXIT1 (Thoát)
    JMP LL1           ; Phím khác -> Chờ tiếp
    
  INSTRUC:
    ; --- Hiển thị màn hình Hướng dẫn ---
    CALL RESET_THE_SCREEN ; Xóa màn hình (về chế độ 13H)
    
    MOV AH,9
    LEA DX,IMSG1      ; In chuỗi hướng dẫn
    INT 21H
    
    MOV AH,1          ; Dịch vụ 1: Chờ 1 phím (có echo)
    INT 21H           ; (Chỉ để tạm dừng màn hình)
    
    CALL RESET_THE_SCREEN ; Xóa màn hình
    
    JMP MPR           ; Quay lại Menu chính
    
  STG: 
    ; --- Bắt đầu Game ---
    CALL RESET_THE_SCREEN ; Xóa màn hình
    MOV AH,9
    LEA DX,MSG10      ; In "LIFE REMAINING:"
    INT 21H  
    MOV AH,9 
    LEA DX,MSG9       ; In "SCORE: "
    INT 21H  
    
    JMP GAME          ; Nhảy đến vòng lặp game chính (trong MAIN)
    
  EXIT1:
    ; --- Thoát game từ Menu ---
    MOV AH,0
    MOV AL,2          ; Chuyển về chế độ Text (80x25)
    INT 10H
       
    MOV AH,4CH        ; Dịch vụ 4CH: Thoát chương trình
    INT 21H

  RET
MAIN_MENU ENDP 

RESET_THE_SCREEN PROC
    ; --- Xóa màn hình đồ họa 13H ---
    MOV AH,0
    MOV AL,2          ; Chuyển về chế độ Text (để reset)
    INT 10H
    MOV AX,13H        ; Chuyển lại chế độ Đồ họa 13H (320x200)
    INT 10H           ; (Cách này sẽ xóa sạch màn hình)
    
    RET
RESET_THE_SCREEN ENDP 
```

### Các thủ tục In số (Decimal Output)

```assembly
OUTDEC PROC   
  ; --- In một số (trong AX) ra màn hình (chế độ Text) ---
  
   PUSH BX            ; Lưu các thanh ghi
   PUSH CX                       
   PUSH DX                        

   XOR CX, CX          ; CX = 0 (Bộ đếm số chữ số)
   MOV BX, 10          ; BX = 10 (số chia)

 OUTPUT:              
    ; --- Vòng lặp chia 10 (để tách số) ---
     XOR DX, DX        ; DX = 0 (DX:AX là số bị chia)
     DIV BX           ; (DX:AX) / 10. AX = Thương, DX = Số dư
     PUSH DX           ; Đẩy số dư (chữ số) vào stack
     INC CX           ; Tăng bộ đếm
     OR AX, AX         ; Kiểm tra AX (thương) có bằng 0 không?
   JNE OUTPUT          ; Nếu AX != 0, tiếp tục chia

     MOV AH, 2          ; Dịch vụ 2: In 1 ký tự

 DISP:              
    ; --- Vòng lặp lấy từ stack ra in (để in đúng thứ tự) ---
     POP DX           ; Lấy chữ số (số dư) ra DX
     OR DL, 30H         ; Chuyển số (0-9) thành ký tự ASCII ('0'-'9')
     INT 21H           ; In ký tự
  
   LOOP DISP            ; Lặp lại cho đến khi CX = 0

    POP DX             ; Khôi phục các thanh ghi
    POP CX                         
    POP BX 
    RET                            
OUTDEC ENDP      

OUTDECGPS PROC   
  ; --- In số (chế độ Đồ họa) tại vị trí (Hàng 1, Cột 39) - Dùng cho ĐIỂM SỐ (Hàng đơn vị) ---
  
   PUSH BX            ; (Logic chia 10 y hệt OUTDEC)
   PUSH CX                       
   PUSH DX                        
   XOR CX, CX                     
   MOV BX, 10                     
 OUTPUTGP:                       
     XOR DX, DX                   
     DIV BX                       
     PUSH DX                     
     INC CX                       
     OR AX, AX                    
   JNE OUTPUTGP                    

     ; --- Phần khác biệt: In bằng INT 10H (đồ họa) ---
     MOV AH, 0BH      ; (Không rõ tác dụng, có thể là set palette)
     INT 10H
     MOV AH,02        ; Dịch vụ 2: Đặt vị trí con trỏ
     MOV BH,0         ; Trang (Page) 0
     MOV DH,1         ; Hàng (Row) 1
     MOV DL,39        ; Cột (Column) 39
     INT 10H
     MOV AH,9          ; Dịch vụ 9: In ký tự (ở vị trí con trỏ)

 DISPGP:              
     POP DX           ; Lấy chữ số
     OR DL, 30H         ; Chuyển sang ASCII
     MOV AL,DL        ; AL = Ký tự cần in
     MOV BL,2         ; Màu 2 (Xanh lá)
     MOV CX,1         ; In 1 lần
     INT 10H           ; In
  
   LOOP DISPGP        

    POP DX             ; Khôi phục thanh ghi
    POP CX                         
    POP BX 
    RET                            
OUTDECGPS ENDP           

; --- OUTDECGPS1 đến OUTDECGPS9 ---
; Đây là các hàm "hard-code" (viết cố định) để in chữ số HÀNG CHỤC
; của điểm số. (Ví dụ, nếu điểm là 25, SC3 sẽ gọi OUTDECGPS2 để in số '2'
; và FJUMP sẽ gọi OUTDECGPS để in số '5').
; Các hàm này chỉ in 1 ký tự ('1', '2', '3'...) tại Cột 38.

OUTDECGPS1 PROC   
    ; --- In ký tự '1' tại (Hàng 1, Cột 38) ---
     MOV AH, 0BH
     INT 10H
     MOV AH,02
     MOV BH,0
     MOV DH,1
     MOV DL,38
     INT 10H
     MOV AH,9                     
     MOV AL,'1'        ; Ký tự '1'
     MOV BL,2
     MOV CX,1
     INT 10H                     
    RET                            
OUTDECGPS1 ENDP    

; (Các hàm OUTDECGPS2 đến 9 tương tự, chỉ thay MOV AL, '...')

OUTDECGPC PROC   
    ; --- In số (chế độ Đồ họa) tại vị trí (Hàng 1, Cột 16) - Dùng cho MẠNG SỐNG (Count) ---
    ; (Logic y hệt OUTDECGPS, chỉ khác vị trí)
   PUSH BX                       
   PUSH CX                       
   PUSH DX                        
   XOR CX, CX                     
   MOV BX, 10                     
 OUTPUTGP1:                       
     XOR DX, DX                   
     DIV BX                       
     PUSH DX                     
     INC CX                       
     OR AX, AX                    
   JNE OUTPUTGP1                    
     MOV AH, 0BH
     INT 10H
     MOV AH,02
     MOV BH,0
     MOV DH,1         ; Hàng 1
     MOV DL,16        ; Cột 16 (khác biệt)
     INT 10H
     MOV AH,9                     
 DISPGP1:                      
     POP DX                       
     OR DL, 30H                   
     MOV AL,DL
     MOV BL,14        ; Màu 14 (Vàng)
     MOV CX,1
     INT 10H                     
   LOOP DISPGP1                   
    POP DX                       
    POP CX                         
    POP BX 
    RET                            
OUTDECGPC ENDP
```

### Các thủ tục Xử lý kết thúc Game

```assembly
GAME_OVER PROC
    ; --- Được gọi khi người chơi rơi xuống đáy màn hình ---
    
    MOV DI,0
    GLO:
    
    DEC COUNT         ; Giảm 1 mạng sống
    CMP COUNT,0
    JE  GLO1          ; Nếu hết mạng (COUNT=0) -> Nhảy đến GLO1 (Game Over thật)
    
    ; --- Nếu vẫn còn mạng (COUNT > 0) ---
    CALL RESET_THE_SCREEN ; Xóa màn hình
    MOV AH,9
    LEA DX,MSG10      ; In "LIFE REMAINING:"
    INT 21H  
    MOV AH,9 
    LEA DX,MSG9       ; In "SCORE:"
    INT 21H 
    
    MOV AX,COUNT
    CALL OUTDECGPC      ; In số mạng còn lại
    
    XOR AX,AX           ; Xóa các thanh ghi (chuẩn bị)
    XOR BX,BX
    XOR CX,CX
    XOR DX,DX
    
    MOV AX,SCORE
    CALL OUTDECGPS      ; In điểm số hiện tại
    CALL PLAY_AGAIN_2 ; Đặt lại vị trí khối vuông
    JMP GAME          ; Quay lại Vòng lặp Game chính
    
   GLO1:
    ; --- Nếu hết mạng (COUNT = 0) ---
    CALL RESET_THE_SCREEN ; Xóa màn hình
    
    MOV AH,9
    LEA DX,GAMEOVERMSG ; In "GAME OVER" và "YOUR SCORE:"
    INT 21H    
    
    MOV AX,SCORE
    CALL OUTDEC         ; In điểm số (dùng hàm OUTDEC text mode)
    
    MOV AH,9  
    LEA DX,HIGHESTMSG2 ; In "HIGHEST SCORE:"
    INT 21H  
    
    MOV AX,SCORE
    CMP AX,HIGHSCORE  ; So sánh điểm hiện tại với điểm cao nhất
    JG  HSCORE        ; Nếu cao hơn -> HSCORE
    JMP NHSCORE
    
 HSCORE: 
    ; --- Đạt điểm cao mới ---
    MOV AX,SCORE
    MOV HIGHSCORE,AX  ; Lưu điểm cao mới
    CALL OUTDEC       ; In điểm (cũng là điểm cao mới)
    JMP FNSH 
    
 NHSCORE:
    ; --- Không phải điểm cao mới ---
    MOV AX,HIGHSCORE
    CALL OUTDEC       ; In điểm cao cũ
    JMP FNSH 
    
 FNSH:
    ; --- In tùy chọn chơi lại ---
    MOV AH,9
    LEA DX,GAMEOVERMSG1 ; In "Press P to Play Again"
    INT 21H   
    
    LEA DX,GAMEOVERMSG3 ; In "Press X to Exit"
    INT 21H 
    
    MOV BX,SCORE1       ; Lấy giá trị 0
    MOV SCORE,BX        ; Reset điểm về 0
    
    AGA:
    ; --- Chờ nhập P hoặc X ---
    MOV AH,7
    INT 21H
    CMP AL,'X'
    JE GGG
    CMP AL,'x'
    JE GGG            ; Nhấn 'X' -> Thoát (GGG)
    CMP AL,'P'
    JE DIDA 
    CMP AL,'p'
    JE DIDA           ; Nhấn 'P' -> Chơi lại (DIDA)
    JMP AGA           ; Phím khác -> Chờ tiếp
    
    DIDA:
    ; --- Chơi lại ---
    MOV BX,COUNT1
    MOV COUNT,BX      ; Reset mạng sống về 3
    CALL RESET_THE_SCREEN   
    MOV AH,9
    LEA DX,MSG10
    INT 21H  
    MOV AH,9 
    LEA DX,MSG9
    INT 21H  
    CALL PLAY_AGAIN   ; Reset vị trí ban đầu
    JMP GAME          ; Quay lại Vòng lặp Game chính
    
    GGG:
     RET              ; Quay về (sẽ dẫn đến FINAL_EXIT)
GAME_OVER ENDP   

HIGHEST PROC
    ; --- Được gọi khi SCORE >= 100 (Thắng game) ---
    ; (Logic gần như y hệt GLO1 trong GAME_OVER, nhưng
    ; không kiểm tra điểm cao vì 100 là cao nhất rồi)
    CALL RESET_THE_SCREEN 
    MOV AH,9
    LEA DX,GAMEOVERMSG
    INT 21H    
    MOV AX,SCORE
    CALL OUTDEC 
    MOV AH,9
    LEA DX,GAMEOVERMSG1
    INT 21H 
    MOV BX,SCORE1
    MOV SCORE,BX  
    MOV AH,9
    LEA DX,HIGHESTMSG   ; In "HIGHEST SCORE: 100" (bị hardcode)
    INT 21H    
    LEA DX,GAMEOVERMSG3
    INT 21H 
    ; ... (Phần chờ P/X y hệt GAME_OVER) ...
    AGAH:
    MOV AH,7
    INT 21H
    CMP AL,'X'
    JE GGGH
    CMP AL,'x'
    JE GGGH
    CMP AL,'P'
    JE DIDAH 
    CMP AL,'p'
    JE DIDAH
    JMP AGAH
    
    DIDAH:
    MOV BX,COUNT1
    MOV COUNT,BX 
    CALL RESET_THE_SCREEN   
    MOV AH,9
    LEA DX,MSG10
    INT 21H  
    MOV AH,9 
    LEA DX,MSG9
    INT 21H  
    CALL PLAY_AGAIN
    JMP GAME
    
    GGGH:
     RET  
HIGHEST ENDP  

FINAL_EX PROC
    ; --- Thủ tục thoát cuối cùng ---
    MOV AH,0
    MOV AL,2          ; Chuyển về chế độ Text
    INT 10H
    MOV AH,4CH        ; Thoát về DOS
    INT 21H
FINAL_EX ENDP
```

-----

## 🎮 Phần 3: Thủ tục Chính (MAIN PROC) - Vòng lặp Game

Đây là nơi logic chính của game diễn ra, kết nối tất cả các thủ tục ở trên.

```assembly
 MAIN PROC
    ; --- Khởi tạo chương trình ---
    MOV AX,@DATA      ; Lấy địa chỉ của Data Segment
    MOV DS,AX         ; Nạp vào thanh ghi DS
    
    MOV AX,13H        ; Chế độ đồ họa 320x200, 256 màu
    INT 10H
    CALL MAIN_MENU    ; Hiển thị Menu (Chương trình sẽ tạm dừng ở đây)
                      ; (Nếu người dùng chọn '1', MAIN_MENU sẽ JMP GAME)
    
    MOV BX,COUNT1
    MOV COUNT,BX      ; Reset mạng sống (phòng trường hợp)
    
; ==================================================================
; =================== VÒNG LẶP GAME CHÍNH (GAME LOOP) ================
; ==================================================================
  GAME:
    
    ; --- 1. KIỂM TRA INPUT ---
    MOV AH,1
    INT 16H           ; Dịch vụ 16H/AH=1: Kiểm tra xem có phím nào trong buffer không? (Non-blocking)
    
    JZ NOKEYPRESS     ; JZ (Jump if Zero): Nếu cờ Zero=1 (Không có phím) -> Nhảy đến NOKEYPRESS
    JNZ KEYPRESS      ; JNZ (Jump if Not Zero): Nếu cờ Zero=0 (Có phím) -> Nhảy đến KEYPRESS
    
  NOKEYPRESS:
    ; --- 2A. LOGIC KHI KHÔNG NHẤN PHÍM (Tự động) ---
  
    CALL NEXT_XLINE   ; Cập nhật (nếu cần) vị trí X, Y của thanh 1
    CALL NEXT_NXLINE  ; Cập nhật (nếu cần) vị trí X, Y của thanh 2
    CALL NEXT_NNXLINE ; Cập nhật (nếu cần) vị trí X, Y của thanh 3
    CALL NEXT_NNNXLINE; Cập nhật (nếu cần) vị trí X, Y của thanh 4
    CALL GENERATE_RANDOM_NUMBER ; Tạo 1 số ngẫu nhiên mới (chuẩn bị cho lần sau)
    
    CALL BORDER       ; Vẽ lại viền
    
    CALL UPDRAWCUBE   ; Vẽ khối vuông tại vị trí (XCUBE, YCUBE)
   
    CALL DRAWLINE     ; Vẽ 4 thanh ngang
    CALL DRAWNLINE
    CALL DRAWNNLINE  
    CALL DRAWNNNLINE
    
    ; --- Chọn tốc độ game (Delay) dựa trên điểm số ---
    MOV  BX,SCORE
    CMP  BX,80 
    JG   DLA5       ; > 80: Delay 5 (nhanh nhất)
    CMP  BX,60
    JG   DLA4       ; > 60: Delay 4
    CMP  BX,40
    JG   DLA3       ; > 40: Delay 3
    CMP  BX,20
    JG   DLA2       ; > 20: Delay 2
    CMP  BX,0
    JGE  DLA         ; > 0: Delay 1 (chậm nhất)
  
  DLA5: 
    CALL DELAY5
    JMP  NXT  
  DLA4: 
    CALL DELAY4
    JMP  NXT 
  DLA3: 
    CALL DELAY3
    JMP  NXT  
  DLA2: 
    CALL DELAY2
    JMP  NXT
  DLA: 
    CALL DELAY
    JMP  NXT  
   
  NXT:
    ; --- Xóa mọi thứ (chuẩn bị cho khung hình tiếp theo) ---
    CALL UPRMVCUBE    ; Xóa khối vuông
    CALL RMVLINE      ; Xóa 4 thanh ngang
    CALL RMVNLINE
    CALL RMVNNLINE    
    CALL RMVNNNLINE
    JMP AGAIN         ; Nhảy đến phần cập nhật logic
   
  KEYPRESS:  
    ; --- 2B. LOGIC KHI CÓ NHẤN PHÍM (Xử lý di chuyển) ---
    MOV AH,0
    INT 16H           ; Dịch vụ 16H/AH=0: Lấy phím ra khỏi buffer (Blocking)
    CMP AL,'E'
    JE  EXIT2         ; 'E' -> Thoát (Forfeit)
    CMP AL,'e'
    JE  EXIT2
    CMP AL,'A'
    JE MOVELEFT       ; 'A' -> Di chuyển trái
    CMP AL,'a'
    JE MOVELEFT
    CMP AL,'D'
    JE MOVERIGHT      ; 'D' -> Di chuyển phải
    CMP AL,'d'
    JE MOVERIGHT
    JMP AGAIN         ; Phím khác (không phải A, D, E) -> Bỏ qua, đi đến cập nhật
       
  MOVELEFT: 
    MOV BX,XCUBE
    CMP BX,4          ; Kiểm tra va chạm viền trái
    JL  AGAIN         ; Nếu < 4, không di chuyển
    SUB XCUBE,2       ; Giảm X (di chuyển trái)
    SUB X2CUBE,2      ; Cập nhật X gốc
    JMP AGAIN 
     
  MOVERIGHT:  
    MOV BX,XCUBE 
    ADD BX,7
    CMP BX,315        ; Kiểm tra va chạm viền phải
    JG  AGAIN         ; Nếu > 315, không di chuyển
    ADD XCUBE,2       ; Tăng X (di chuyển phải)
    ADD X2CUBE,2      ; Cập nhật X gốc
    JMP AGAIN
    
  EXIT2: 
    ; --- Xử lý khi người chơi chủ động thoát (Forfeit) ---
    CALL RESET_THE_SCREEN
    MOV AH,9
    LEA DX,GAMEOVERMSG2 ; In "You Forfeited"
    INT 21H       
    LEA DX,GAMEOVERMSG1 ; In "Play Again"
    INT 21H    
    LEA DX,GAMEOVERMSG3 ; In "Exit"
    INT 21H 
    JMP AGA2          ; Nhảy đến phần chờ P/X (ở cuối)
    
  AGAIN:  
    ; --- 3. CẬP NHẬT TRẠNG THÁI LOGIC (Va chạm & Di chuyển) ---
  
    CALL CHECK_UP_OR_DOWN   ; Kiểm tra va chạm thanh 1
    CALL NCHECK_UP_OR_DOWN  ; Kiểm tra va chạm thanh 2
    CALL NNCHECK_UP_OR_DOWN ; Kiểm tra va chạm thanh 3
    CALL NNNCHECK_UP_OR_DOWN; Kiểm tra va chạm thanh 4
    
    ; --- Kiểm tra xem có đang va chạm với BẤT KỲ thanh nào không ---
    CMP CHECK_UND,1
    JE AGAIN1
    CMP NCHECK_UND,1
    JE AGAIN1
    CMP NNCHECK_UND,1
    JE AGAIN1
    CMP NNNCHECK_UND,1
    JE AGAIN1  
    JMP AGAIN3        ; Nếu không va chạm thanh nào -> Nhảy đến AGAIN3 (Rơi)
    
   AGAIN1: 
    ; --- LOGIC KHI ĐANG ĐỨNG TRÊN THANH ---
    CMP CHECK_UND,1
    JE  LIFEDEC       ; Đứng trên thanh Đỏ (1) -> Trừ mạng
    
    CMP NNNCHECK_UND,1
    JE  LIFEINC       ; Đứng trên thanh Xanh lá (4) -> Cộng mạng
    JMP LIFEADJ       ; Đứng trên thanh Cyan/Magenta (2,3) -> Không làm gì
    
   LIFEINC: 
    ; --- Cộng mạng (chỉ 1 lần) ---
     MOV BX,Y          ; Lấy cờ Y
     CMP BX,1
     JE  YES2        ; Nếu cờ = 1 (chưa cộng) -> Nhảy đến YES2
     JMP LIFEADJ      ; Nếu cờ = 0 (đã cộng rồi) -> Bỏ qua
     
   YES2:
     MOV BX,0
     MOV Y,BX          ; Đặt cờ Y = 0 (đánh dấu đã cộng)
     INC COUNT        ; Tăng mạng
     CMP COUNT,9
     JG  CNTG         ; Giới hạn tối đa 9 mạng
     JMP CNTL
   CNTG:
     MOV COUNT,9
   CNTL:
     MOV AX,COUNT
     CALL OUTDECGPC     ; Cập nhật số mạng trên màn hình
     JMP LIFEADJ
    
   LIFEDEC:
    ; --- Trừ mạng (chỉ 1 lần) ---
    MOV BX,X
    CMP BX,1  
    JE  YES           ; Nếu cờ = 1 (chưa trừ) -> Nhảy đến YES
    JMP LIFEADJ       ; Nếu cờ = 0 (đã trừ rồi) -> Bỏ qua
    
   YES: 
    MOV BX,0    
    MOV X,BX          ; Đặt cờ X = 0 (đánh dấu đã trừ)
    DEC COUNT         ; Giảm mạng
    
    MOV AX,COUNT
    CALL OUTDECGPC      ; Cập nhật số mạng trên màn hình
    
    CMP COUNT,0
    JE  DEAD          ; Nếu hết mạng -> DEAD
    JMP ALIVE         ; Nếu còn mạng -> ALIVE
   
   DEAD: 
    ; --- Xử lý khi hết mạng (logic Game Over tại chỗ) ---
    CALL RESET_THE_SCREEN 
    MOV AH,9
    LEA DX,GAMEOVERMSG ; In "GAME OVER"
    INT 21H    
    MOV AX,SCORE
    CALL OUTDEC        ; In điểm
    MOV AH,9  
    LEA DX,HIGHESTMSG2 ; In "HIGHEST SCORE"
    INT 21H  
    ; ... (Logic kiểm tra/cập nhật Điểm cao y hệt GAME_OVER) ...
    MOV AX,SCORE
    CMP AX,HIGHSCORE
    JG  HSCORE1  
    JMP NHSCORE1
 HSCORE1: 
    MOV AX,SCORE
    MOV HIGHSCORE,AX
    CALL OUTDEC 
    JMP FNSH1 
 NHSCORE1:
    MOV AX,HIGHSCORE
    CALL OUTDEC
    JMP FNSH1 
 FNSH1:
    MOV AH,9
    LEA DX,GAMEOVERMSG1 ; In "Play Again"
    INT 21H   
    LEA DX,GAMEOVERMSG3 ; In "Exit"
    INT 21H 
    MOV BX,SCORE1
    MOV SCORE,BX        ; Reset điểm
    ; ... (Chờ P hoặc X) ...
    AGAD:
    MOV AH,7
    INT 21H
    CMP AL,'P'
    JE DIDAD 
    CMP AL,'p'
    JE DIDAD 
    CMP AL,'X'
    JE  FEX
    CMP AL,'x'
    JE  FEX
    JMP AGAD   
    FEX:
    CALL FINAL_EX     ; Nhấn 'X' -> Thoát hẳn
    DIDAD:
    MOV BX,COUNT1
    MOV COUNT,BX      ; Nhấn 'P' -> Reset mạng
    CALL RESET_THE_SCREEN   
  t; ... (In lại UI và chơi lại) ...
    MOV AH,9
    LEA DX,MSG10
    INT 21H  
    MOV AH,9 
    LEA DX,MSG9
    INT 21H  
    CALL PLAY_AGAIN
    JMP GAME          ; Quay lại Vòng lặp Game
     
   ALIVE: 
    JMP LIFEADJ 
    
   LIFEADJ:
     DEC YCUBE        ; *** QUAN TRỌNG: Di chuyển khối vuông LÊN 1 pixel ***
     JMP AGAIN2       ; (Vì khối vuông đang đứng trên thanh, mà thanh đi lên)
     
  AGAIN3:
    ; --- LOGIC KHI ĐANG RƠI TỰ DO (Không va chạm) ---
   
    ; --- Reset cờ cộng/trừ mạng (để chuẩn bị cho lần chạm tiếp theo) ---
    LIFEADJX: 
    MOV BX,YLINE
    CMP YCUBE,BX
    JE  XNEW 
    DEC BX     
    CMP YCUBE,BX
    JE  XNEW 
    JMP LIFEADJY
   XNEW:
    MOV BX,1
    MOV X,BX          ; Reset cờ X (trừ mạng)
    
   LIFEADJY: 
    MOV BX,NNNYLINE
    CMP YCUBE,BX
    JE  YNEW 
    DEC BX     
    CMP YCUBE,BX
    JE  YNEW 
    JMP FINAL
   YNEW:
    MOV BX,1
    MOV Y,BX          ; Reset cờ Y (cộng mạng)
    
  FINAL:  
    ; --- Kiểm tra xem có VỪA đi qua 1 thanh để tính điểm không ---
    MOV BX,YLINE
    CMP YCUBE,BX
    JE  SCOREL        ; Vừa qua thanh 1
    DEC BX     
    CMP YCUBE,BX
    JE  SCOREL 
    MOV BX,NYLINE
    CMP YCUBE,BX
    JE  SCOREL        ; Vừa qua thanh 2
    DEC BX     
    CMP YCUBE,BX
    JE  SCOREL 
    MOV BX,NNYLINE
    CMP YCUBE,BX
    JE  SCOREL        ; Vừa qua thanh 3
    DEC BX     
    CMP YCUBE,BX
    JE  SCOREL 
    MOV BX,NNNYLINE
    CMP YCUBE,BX
    JE  SCOREL        ; Vừa qua thanh 4
    DEC BX     
    CMP YCUBE,BX
    JE  SCOREL 
   
    INC YCUBE         ; *** QUAN TRỌNG: Di chuyển khối vuông XUỐNG 1 pixel (Rơi) ***
    JMP AGAIN2
  
 SCOREL: 
    ; --- Tăng điểm (vì vừa đi qua 1 thanh) ---
    INC YCUBE         ; Vẫn di chuyển xuống
    INC SCORE         ; Tăng 1 điểm
    
    MOV AX,COUNT
    CALL OUTDECGPC    ; Cập nhật (vẽ lại) số mạng (vì bị điểm số đè lên?)
    
    ; --- Logic phức tạp để in điểm số (vì không có hàm `itoa` tử tế) ---
    CMP SCORE,10
    JL  SC1           ; < 10 (In hàng đơn vị)
    CMP SCORE,20
    JL  SC2           ; < 20 (In '1' và hàng đơn vị)
    CMP SCORE,30
    JL  SC3           ; < 30 (In '2' và hàng đơn vị)
    CMP SCORE,40
    JL SC4
    CMP SCORE,50
    JL SC5  
    CMP SCORE,60
    JL SC6    
    CMP SCORE,70
    JL SC7  
    CMP SCORE,80
    JL SC8
    CMP SCORE,90
    JL  SC9   
    CMP SCORE,100
    JL  SC10
    
    CMP SCORE,100
    JGE EXITH         ; >= 100 -> Thắng game
    
   EXITH:
    CALL HIGHEST      ; Gọi thủ tục thắng game
    
   SC1: 
    MOV AX,SCORE      ; AX = (0-9)
    JMP FJUMP
   SC2:   
    CALL OUTDECGPS1   ; In số '1'
    MOV AX,SCORE
    SUB AX,10         ; AX = (10-19) - 10 = (0-9)
    JMP FJUMP 
   SC3:
    CALL OUTDECGPS2   ; In số '2'
    MOV AX,SCORE
    SUB AX,20         ; AX = (20-29) - 20 = (0-9)
    JMP FJUMP
    ; ... (Tương tự cho SC4 đến SC10) ...
   SC10:
    CALL OUTDECGPS9   ; In số '9'
    MOV AX,SCORE
    SUB AX,90         ; AX = (90-99) - 90 = (0-9)
    JMP FJUMP 
  
  FJUMP:  
    CALL OUTDECGPS    ; In hàng đơn vị (số dư trong AX)
    JMP AGAIN2
    
 AGAIN2:
    ; --- 4. CẬP NHẬT MÀN HÌNH (Di chuyển các thanh) ---
    DEC YLINE         ; Di chuyển thanh 1 LÊN 1 pixel
    DEC NYLINE        ; Di chuyển thanh 2 LÊN 1 pixel
    DEC NNYLINE       ; Di chuyển thanh 3 LÊN 1 pixel
    DEC NNNYLINE      ; Di chuyển thanh 4 LÊN 1 pixel
    
    ; --- 5. KIỂM TRA ĐIỀU KIỆN THUA (Chạm biên) ---
    CMP YCUBE,198     ; Khối vuông có rơi ra khỏi đáy (Y > 198) không?
    JE EXIT
    CMP YCUBE,25      ; Khối vuông có bay ra khỏi đỉnh (Y < 25) không?
    JE EXIT
    CMP YCUBE,26      ; (Kiểm tra 2 lần cho chắc?)
    JE EXIT
    JMP GAME          ; *** Nếu chưa thua -> QUAY LẠI ĐẦU VÒNG LẶP (GAME:) ***

  
  ; --- Phần chờ P/X của EXIT2 (Forfeit) ---
 AGA2:
    MOV AH,7
    INT 21H
    CMP AL,'X'
    JE  FINAL_EXIT
    CMP AL,'x'
    JE  FINAL_EXIT    ; Nhấn 'X' -> Thoát
    CMP AL,'P'
    JE  DIDA2 
    CMP AL,'p'
    JE  DIDA2         ; Nhấn 'P' -> Chơi lại
    JMP AGA2
 DIDA2:
    ; --- Chơi lại (sau khi Forfeit) ---
    MOV BX,COUNT1
    MOV COUNT,BX      ; Reset mạng
    MOV BX,SCORE1
    MOV SCORE,BX      ; Reset điểm
    CALL RESET_THE_SCREEN 
    MOV AH,9
    LEA DX,MSG10
    INT 21H  
    MOV AH,9 
    LEA DX,MSG9
    INT 21H  
    CALL PLAY_AGAIN
    JMP GAME          ; Quay lại Vòng lặp Game
    
  EXIT:
    ; --- Được gọi khi chạm biên trên/dưới (AGAIN2) ---
    CALL GAME_OVER    ; Gọi thủ tục Game Over (để trừ mạng hoặc kết thúc)
    JMP  FINAL_EXIT    ; (Sau khi GAME_OVER xong, sẽ thoát)
    
  FINAL_EXIT:           
    ; --- Kết thúc chương trình ---
    MOV AH,0
    MOV AL,2          ; Chuyển về chế độ Text
    INT 10H
    MOV AH,4CH        ; Thoát về DOS
    INT 21H
    
    MAIN ENDP         ; Kết thúc thủ tục MAIN

END MAIN              ; Kết thúc chương trình
```
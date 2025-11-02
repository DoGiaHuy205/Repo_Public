.MODEL SMALL                        ; Khai báo mô hình bộ nhớ nhỏ
.STACK 100H                         ; Cấp phát 256 bytes cho stack

.DATA                               ; Bắt đầu phân đoạn dữ liệu
    ; Vị trí hộp người chơi
    CUBE_X DW 36                    ; Tọa độ X của cube (cột)
    CUBE_Y DW 56                    ; Tọa độ Y của cube (hàng)
    
    ; Vị trí thanh ngang 1
    LINE1_X DW 70                   ; Tọa độ X của thanh ngang 1
    LINE1_Y DW 120                  ; Tọa độ Y của thanh ngang 1
    
    ; Vị trí thanh ngang 2
    LINE2_X DW 150                  ; Tọa độ X của thanh ngang 2
    LINE2_Y DW 180                  ; Tọa độ Y của thanh ngang 2
    
    ; Biến game
    SCORE DW 0                      ; Điểm số của người chơi
    ON_LINE DB 0                    ; Cờ kiểm tra cube có đứng trên thanh không (1=có, 0=không)
    RAND_NUM DW 0                   ; Biến lưu số ngẫu nhiên
    
    ; Menu
    MSG_TITLE DB 0AH,0DH,'     RAPID ROLL$'      ; Tiêu đề game
    MSG_START DB 0AH,0DH,0AH,0DH,' Press 1 to START$'  ; Thông báo bắt đầu
    MSG_EXIT  DB 0AH,0DH,' Press 2 to EXIT$'     ; Thông báo thoát
    
    ; Game Over
    MSG_OVER DB 0AH,0DH,0AH,0DH,' GAME OVER!$'   ; Thông báo game over
    MSG_SCORE DB 0AH,0DH,' Score: $'             ; Nhãn hiển thị điểm

.CODE                               ; Bắt đầu phân đoạn code

; Vẽ hộp người chơi (8x8 pixels)
DRAW_CUBE PROC                      ; Khai báo procedure vẽ cube
    MOV CX, CUBE_X                  ; Lấy tọa độ X của cube vào CX
    MOV DX, CUBE_Y                  ; Lấy tọa độ Y của cube vào DX
    MOV BX, 8                       ; Đặt bộ đếm 8 hàng
    
    DRAW_ROW:                       ; Nhãn vẽ từng hàng
    PUSH CX                         ; Lưu giá trị X ban đầu
    MOV SI, 8                       ; Đặt bộ đếm 8 cột
    
    DRAW_PIXEL:                     ; Nhãn vẽ từng pixel
    MOV AH, 0CH                     ; Function 0CH: Write pixel
    MOV AL, 11                      ; Màu cyan (11)
    INT 10H                         ; Gọi interrupt video để vẽ pixel
    INC CX                          ; Tăng X (sang phải 1 pixel)
    DEC SI                          ; Giảm bộ đếm cột
    JNZ DRAW_PIXEL                  ; Nếu chưa đủ 8 pixel thì lặp lại
    
    POP CX                          ; Khôi phục X ban đầu
    INC DX                          ; Tăng Y (xuống dưới 1 hàng)
    DEC BX                          ; Giảm bộ đếm hàng
    JNZ DRAW_ROW                    ; Nếu chưa đủ 8 hàng thì lặp lại
    RET                             ; Trả về
ENDP DRAW_CUBE                      ; Kết thúc procedure

; Xóa hộp người chơi
ERASE_CUBE PROC                     ; Khai báo procedure xóa cube
    MOV CX, CUBE_X                  ; Lấy tọa độ X của cube vào CX
    MOV DX, CUBE_Y                  ; Lấy tọa độ Y của cube vào DX
    MOV BX, 8                       ; Đặt bộ đếm 8 hàng
    
    ERASE_ROW:                      ; Nhãn xóa từng hàng
    PUSH CX                         ; Lưu giá trị X ban đầu
    MOV SI, 8                       ; Đặt bộ đếm 8 cột
    
    ERASE_PIXEL:                    ; Nhãn xóa từng pixel
    MOV AH, 0CH                     ; Function 0CH: Write pixel
    MOV AL, 0                       ; Màu đen (0) để xóa
    INT 10H                         ; Gọi interrupt video để vẽ pixel đen
    INC CX                          ; Tăng X (sang phải 1 pixel)
    DEC SI                          ; Giảm bộ đếm cột
    JNZ ERASE_PIXEL                 ; Nếu chưa đủ 8 pixel thì lặp lại
    
    POP CX                          ; Khôi phục X ban đầu
    INC DX                          ; Tăng Y (xuống dưới 1 hàng)
    DEC BX                          ; Giảm bộ đếm hàng
    JNZ ERASE_ROW                   ; Nếu chưa đủ 8 hàng thì lặp lại
    RET                             ; Trả về
ENDP ERASE_CUBE                     ; Kết thúc procedure

; Vẽ thanh ngang (40 pixels rộng, 3 pixels cao)
DRAW_LINE PROC                      ; Khai báo procedure vẽ thanh ngang
    ; DI = LINE_X, SI = LINE_Y      ; Tham số đầu vào qua thanh ghi
    MOV CX, DI                      ; Lấy tọa độ X của thanh vào CX
    MOV DX, SI                      ; Lấy tọa độ Y của thanh vào DX
    MOV BX, 3                       ; Đặt bộ đếm 3 hàng (chiều cao thanh)
    
    LINE_ROW:                       ; Nhãn vẽ từng hàng của thanh
    PUSH CX                         ; Lưu giá trị X ban đầu
    PUSH BX                         ; Lưu bộ đếm hàng
    MOV BX, 40                      ; Đặt bộ đếm 40 cột (chiều rộng thanh)
    
    LINE_PIXEL:                     ; Nhãn vẽ từng pixel của thanh
    MOV AH, 0CH                     ; Function 0CH: Write pixel
    MOV AL, 13                      ; Màu magenta (13)
    INT 10H                         ; Gọi interrupt video để vẽ pixel
    INC CX                          ; Tăng X (sang phải 1 pixel)
    DEC BX                          ; Giảm bộ đếm cột
    JNZ LINE_PIXEL                  ; Nếu chưa đủ 40 pixel thì lặp lại
    
    POP BX                          ; Khôi phục bộ đếm hàng
    POP CX                          ; Khôi phục X ban đầu
    INC DX                          ; Tăng Y (xuống dưới 1 hàng)
    DEC BX                          ; Giảm bộ đếm hàng
    JNZ LINE_ROW                    ; Nếu chưa đủ 3 hàng thì lặp lại
    RET                             ; Trả về
ENDP DRAW_LINE                      ; Kết thúc procedure

; Kiểm tra va chạm với thanh ngang
CHECK_COLLISION PROC                ; Khai báo procedure kiểm tra va chạm
    ; DI = LINE_X, SI = LINE_Y      ; Tham số: tọa độ thanh ngang
    ; Kiểm tra nếu đáy cube chạm thanh (Y+8 >= LINE_Y và Y+8 <= LINE_Y+3)
    MOV AX, CUBE_Y                  ; Lấy tọa độ Y của cube
    ADD AX, 8                       ; Tính tọa độ đáy cube (Y + 8)
    
    CMP AX, SI                      ; So sánh đáy cube với đỉnh thanh
    JL NO_COLLISION                 ; Nếu đáy cube < đỉnh thanh -> không va chạm
    
    MOV BX, SI                      ; Sao chép Y của thanh
    ADD BX, 3                       ; Tính tọa độ đáy thanh (Y + 3)
    CMP AX, BX                      ; So sánh đáy cube với đáy thanh
    JG NO_COLLISION                 ; Nếu đáy cube > đáy thanh -> không va chạm
    
    ; Kiểm tra X: cube phải nằm trong khoảng [LINE_X - 8, LINE_X + 40]
    MOV AX, CUBE_X                  ; Lấy tọa độ X của cube
    MOV BX, DI                      ; Sao chép X của thanh
    SUB BX, 8                       ; Tính biên trái cho phép (LINE_X - 8)
    CMP AX, BX                      ; So sánh CUBE_X với biên trái
    JL NO_COLLISION                 ; Nếu CUBE_X < biên trái -> không va chạm
    
    MOV BX, DI                      ; Sao chép X của thanh
    ADD BX, 40                      ; Tính biên phải (LINE_X + 40)
    CMP AX, BX                      ; So sánh CUBE_X với biên phải
    JGE NO_COLLISION                ; Nếu CUBE_X >= biên phải -> không va chạm
    
    ; Va chạm! Đặt cube đứng trên thanh
    MOV AX, SI                      ; Lấy Y của thanh
    SUB AX, 8                       ; Tính vị trí cube đứng trên thanh (LINE_Y - 8)
    MOV CUBE_Y, AX                  ; Cập nhật Y của cube
    MOV ON_LINE, 1                  ; Đặt cờ va chạm = 1
    RET                             ; Trả về
    
    NO_COLLISION:                   ; Nhãn không có va chạm
    MOV ON_LINE, 0                  ; Đặt cờ va chạm = 0
    RET                             ; Trả về
ENDP CHECK_COLLISION                ; Kết thúc procedure

; Delay ngắn
DELAY PROC                          ; Khai báo procedure delay
    PUSH CX                         ; Lưu giá trị CX
    MOV CX, 3                       ; Đặt bộ đếm vòng lặp ngoài = 3
    OUTER_LOOP:                     ; Nhãn vòng lặp ngoài
    PUSH CX                         ; Lưu bộ đếm vòng ngoài
    MOV CX, 0FFFFH                  ; Đặt bộ đếm vòng trong = 65535
    INNER_LOOP:                     ; Nhãn vòng lặp trong
    NOP                             ; No operation - chờ 1 chu kỳ
    NOP                             ; No operation - chờ 1 chu kỳ
    LOOP INNER_LOOP                 ; Lặp lại vòng trong (CX--)
    POP CX                          ; Khôi phục bộ đếm vòng ngoài
    LOOP OUTER_LOOP                 ; Lặp lại vòng ngoài (CX--)
    POP CX                          ; Khôi phục giá trị CX ban đầu
    RET                             ; Trả về
ENDP DELAY                          ; Kết thúc procedure

; Tạo số ngẫu nhiên (0-200)
RANDOM PROC                         ; Khai báo procedure tạo số ngẫu nhiên
    MOV AH, 0                       ; Function 00H: Get system time
    INT 1AH                         ; Gọi interrupt lấy thời gian (kết quả: CX:DX)
    MOV AX, DX                      ; Lấy phần thấp của thời gian
    MOV DX, 0                       ; Xóa DX để chuẩn bị chia
    MOV BX, 200                     ; Số chia = 200
    DIV BX                          ; Chia AX cho 200, dư lưu trong DX
    MOV RAND_NUM, DX                ; Lưu số dư (0-199) vào RAND_NUM
    RET                             ; Trả về
ENDP RANDOM                         ; Kết thúc procedure

; Menu chính
SHOW_MENU PROC                      ; Khai báo procedure hiển thị menu
    MOV AH, 0                       ; Function 00H: Set video mode
    MOV AL, 2                       ; Mode 2: Text mode 80x25
    INT 10H                         ; Gọi interrupt video để chuyển mode
    
    MOV AH, 9                       ; Function 09H: Display string
    LEA DX, MSG_TITLE               ; Nạp địa chỉ chuỗi tiêu đề
    INT 21H                         ; Gọi interrupt DOS để in chuỗi
    LEA DX, MSG_START               ; Nạp địa chỉ chuỗi hướng dẫn bắt đầu
    INT 21H                         ; In chuỗi
    LEA DX, MSG_EXIT                ; Nạp địa chỉ chuỗi hướng dẫn thoát
    INT 21H                         ; In chuỗi
    
    MENU_WAIT:                      ; Nhãn chờ người dùng chọn
    MOV AH, 7                       ; Function 07H: Direct console input (không echo)
    INT 21H                         ; Đọc phím từ bàn phím
    CMP AL, '1'                     ; So sánh với phím '1'
    JE START_GAME                   ; Nếu bằng '1' -> bắt đầu game
    CMP AL, '2'                     ; So sánh với phím '2'
    JE EXIT_GAME                    ; Nếu bằng '2' -> thoát game
    JMP MENU_WAIT                   ; Ngược lại tiếp tục chờ
    
    START_GAME:                     ; Nhãn bắt đầu game
    RET                             ; Trả về để vào game loop
    
    EXIT_GAME:                      ; Nhãn thoát game
    MOV AH, 4CH                     ; Function 4CH: Terminate program
    INT 21H                         ; Gọi interrupt DOS để thoát
ENDP SHOW_MENU                      ; Kết thúc procedure

; Game Over
GAME_OVER PROC                      ; Khai báo procedure game over
    MOV AH, 0                       ; Function 00H: Set video mode
    MOV AL, 2                       ; Mode 2: Text mode 80x25
    INT 10H                         ; Chuyển về chế độ text
    
    MOV AH, 9                       ; Function 09H: Display string
    LEA DX, MSG_OVER                ; Nạp địa chỉ chuỗi "GAME OVER"
    INT 21H                         ; In chuỗi
    LEA DX, MSG_SCORE               ; Nạp địa chỉ chuỗi "Score: "
    INT 21H                         ; In chuỗi
    
    ; Hiển thị điểm
    MOV AX, SCORE                   ; Lấy điểm số
    MOV BX, 10                      ; Số chia = 10 (để tách từng chữ số)
    MOV CX, 0                       ; Bộ đếm số chữ số = 0
    
    CONVERT:                        ; Nhãn chuyển đổi số thành chữ số
    MOV DX, 0                       ; Xóa DX để chuẩn bị chia
    DIV BX                          ; Chia AX cho 10, dư (chữ số) trong DX
    PUSH DX                         ; Đẩy chữ số vào stack
    INC CX                          ; Tăng bộ đếm chữ số
    CMP AX, 0                       ; Kiểm tra còn số nào không
    JNE CONVERT                     ; Nếu còn thì tiếp tục tách
    
    PRINT_SCORE:                    ; Nhãn in điểm số
    POP DX                          ; Lấy chữ số từ stack (theo thứ tự ngược)
    ADD DL, 48                      ; Chuyển số (0-9) thành ký tự ASCII ('0'-'9')
    MOV AH, 2                       ; Function 02H: Display character
    INT 21H                         ; In ký tự
    LOOP PRINT_SCORE                ; Lặp lại cho đến hết chữ số
    
    MOV AH, 1                       ; Function 01H: Read character with echo
    INT 21H                         ; Chờ người dùng nhấn phím
    RET                             ; Trả về
ENDP GAME_OVER                      ; Kết thúc procedure

MAIN PROC                           ; Khai báo procedure chính
    MOV AX, @DATA                   ; Nạp địa chỉ phân đoạn dữ liệu
    MOV DS, AX                      ; Gán vào thanh ghi DS
    
    CALL SHOW_MENU                  ; Gọi procedure hiển thị menu
    
    ; Chế độ đồ họa 320x200
    MOV AX, 13H                     ; Mode 13H: Graphics 320x200 256 màu
    INT 10H                         ; Chuyển sang chế độ đồ họa
    
GAME_LOOP:                          ; Nhãn vòng lặp game chính
    ; Kiểm tra phím (xóa buffer)
    MOV AH, 1                       ; Function 01H: Check keyboard status
    INT 16H                         ; Kiểm tra có phím nào không (ZF=1 nếu không có)
    JZ NO_KEY                       ; Nếu không có phím -> nhảy đến NO_KEY
    
    ; Đọc và xóa phím khỏi buffer
    MOV AH, 0                       ; Function 00H: Read keyboard character
    INT 16H                         ; Đọc phím (xóa khỏi buffer), kết quả: AL=ASCII
    
    ; Xử lý di chuyển
    CMP AL, 'a'                     ; So sánh với phím 'a'
    JE MOVE_LEFT                    ; Nếu bằng 'a' -> di chuyển trái
    CMP AL, 'A'                     ; So sánh với phím 'A'
    JE MOVE_LEFT                    ; Nếu bằng 'A' -> di chuyển trái
    CMP AL, 'd'                     ; So sánh với phím 'd'
    JE MOVE_RIGHT                   ; Nếu bằng 'd' -> di chuyển phải
    CMP AL, 'D'                     ; So sánh với phím 'D'
    JE MOVE_RIGHT                   ; Nếu bằng 'D' -> di chuyển phải
    JMP NO_KEY                      ; Phím khác -> bỏ qua
    
    MOVE_LEFT:                      ; Nhãn di chuyển trái
    CMP CUBE_X, 5                   ; Kiểm tra có chạm biên trái không
    JLE NO_KEY                      ; Nếu <= 5 -> không di chuyển
    SUB CUBE_X, 3                   ; Giảm X của cube đi 3 (sang trái)
    JMP NO_KEY                      ; Nhảy đến NO_KEY
    
    MOVE_RIGHT:                     ; Nhãn di chuyển phải
    CMP CUBE_X, 310                 ; Kiểm tra có chạm biên phải không
    JGE NO_KEY                      ; Nếu >= 310 -> không di chuyển
    ADD CUBE_X, 3                   ; Tăng X của cube lên 3 (sang phải)
    
NO_KEY:                             ; Nhãn không có phím hoặc đã xử lý xong
    ; Xóa hết buffer phím còn lại
    CLEAR_BUFFER:                   ; Nhãn xóa buffer phím
    MOV AH, 1                       ; Function 01H: Check keyboard status
    INT 16H                         ; Kiểm tra có phím nào trong buffer không
    JZ BUFFER_CLEAR                 ; Nếu không có -> buffer đã sạch
    MOV AH, 0                       ; Function 00H: Read keyboard character
    INT 16H                         ; Đọc và xóa phím khỏi buffer
    JMP CLEAR_BUFFER                ; Tiếp tục kiểm tra buffer
    
BUFFER_CLEAR:                       ; Nhãn buffer đã sạch
    ; Kiểm tra va chạm với thanh 1
    MOV DI, LINE1_X                 ; Nạp X của thanh 1 vào DI
    MOV SI, LINE1_Y                 ; Nạp Y của thanh 1 vào SI
    CALL CHECK_COLLISION            ; Gọi procedure kiểm tra va chạm
    CMP ON_LINE, 1                  ; Kiểm tra cờ va chạm
    JE STAY                         ; Nếu va chạm -> nhảy đến STAY
    
    ; Kiểm tra va chạm với thanh 2
    MOV DI, LINE2_X                 ; Nạp X của thanh 2 vào DI
    MOV SI, LINE2_Y                 ; Nạp Y của thanh 2 vào SI
    CALL CHECK_COLLISION            ; Gọi procedure kiểm tra va chạm
    CMP ON_LINE, 1                  ; Kiểm tra cờ va chạm
    JE STAY                         ; Nếu va chạm -> nhảy đến STAY
    
    ; Không có va chạm - rơi xuống
    INC CUBE_Y                      ; Tăng Y của cube (rơi xuống 1 pixel)
    INC SCORE                       ; Tăng điểm số
    JMP MOVE_LINES                  ; Nhảy đến MOVE_LINES
    
STAY:                               ; Nhãn cube đứng trên thanh
    ; Đứng yên trên thanh - không tăng điểm
    
MOVE_LINES:                         ; Nhãn di chuyển các thanh
    ; Di chuyển các thanh lên
    DEC LINE1_Y                     ; Giảm Y của thanh 1 (di chuyển lên 1 pixel)
    DEC LINE2_Y                     ; Giảm Y của thanh 2 (di chuyển lên 1 pixel)
    
    ; Reset thanh 1 nếu vượt màn hình
    CMP LINE1_Y, 10                 ; Kiểm tra thanh 1 có vượt lên trên không
    JGE CHECK_LINE2                 ; Nếu >= 10 -> chưa vượt, kiểm tra thanh 2
    MOV LINE1_Y, 195                ; Reset Y của thanh 1 xuống đáy màn hình
    CALL RANDOM                     ; Tạo số ngẫu nhiên
    MOV AX, RAND_NUM                ; Lấy số ngẫu nhiên
    MOV LINE1_X, AX                 ; Gán X ngẫu nhiên cho thanh 1
    
CHECK_LINE2:                        ; Nhãn kiểm tra thanh 2
    ; Reset thanh 2
    CMP LINE2_Y, 10                 ; Kiểm tra thanh 2 có vượt lên trên không
    JGE DRAW_ALL                    ; Nếu >= 10 -> chưa vượt, vẽ lại màn hình
    MOV LINE2_Y, 195                ; Reset Y của thanh 2 xuống đáy màn hình
    CALL RANDOM                     ; Tạo số ngẫu nhiên
    MOV AX, RAND_NUM                ; Lấy số ngẫu nhiên
    MOV LINE2_X, AX                 ; Gán X ngẫu nhiên cho thanh 2
    
DRAW_ALL:                           ; Nhãn vẽ lại màn hình
    ; Xóa màn hình bằng cách tô đen toàn bộ
    MOV AH, 06H                     ; Function 06H: Scroll window up
    MOV AL, 0                       ; Scroll 0 lines = xóa toàn bộ
    MOV BH, 0                       ; Thuộc tính: màu đen
    MOV CX, 0                       ; Góc trên trái (row=0, col=0)
    MOV DX, 184FH                   ; Góc dưới phải (row=24, col=79)
    INT 10H                         ; Gọi interrupt video để xóa màn hình
    
    ; Vẽ lại các thanh
    MOV DI, LINE1_X                 ; Nạp X của thanh 1
    MOV SI, LINE1_Y                 ; Nạp Y của thanh 1
    CALL DRAW_LINE                  ; Vẽ thanh 1
    
    MOV DI, LINE2_X                 ; Nạp X của thanh 2
    MOV SI, LINE2_Y                 ; Nạp Y của thanh 2
    CALL DRAW_LINE                  ; Vẽ thanh 2
    
    ; Vẽ cube
    CALL DRAW_CUBE                  ; Vẽ hộp người chơi
    CALL DELAY                      ; Gọi delay để điều chỉnh tốc độ game
    
    ; Kiểm tra thua
    CMP CUBE_Y, 190                 ; Kiểm tra cube có rơi xuống đáy không
    JGE END_GAME                    ; Nếu >= 190 -> game over
    CMP CUBE_Y, 15                  ; Kiểm tra cube có vượt lên trên không
    JLE END_GAME                    ; Nếu <= 15 -> game over
    
    JMP GAME_LOOP                   ; Lặp lại vòng lặp game
    
END_GAME:                           ; Nhãn kết thúc game
    CALL GAME_OVER                  ; Gọi procedure hiển thị game over
    
    MOV AH, 4CH                     ; Function 4CH: Terminate program
    INT 21H                         ; Kết thúc chương trình
MAIN ENDP                           ; Kết thúc procedure chính
END MAIN                            ; Kết thúc chương trình
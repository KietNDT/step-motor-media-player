# 📘 Hướng Dẫn Tích Hợp Phần Cứng Màn Hình LCD ILI9341 & 6 Nút Bấm GPIOC

Tài liệu này cung cấp hướng dẫn đầy đủ bằng Tiếng Việt về sơ đồ đấu nối phần cứng, nguyên lý hoạt động của mã nguồn đồ họa và hướng dẫn vận hành chi tiết hệ thống phát nhạc bằng động cơ bước kết hợp màn hình LCD ILI9341 và bộ 6 phím bấm ngắt ngoài trên cổng GPIOC.

---

## 🔌 1. Sơ đồ đấu nối dây phần cứng (Hardware Connections)

Để hệ thống hoạt động chính xác và an toàn, vui lòng thực hiện đấu nối các chân tín hiệu từ board vi điều khiển **STM32F411E-DISCO** tới màn hình LCD ILI9341 và 6 nút nhấn điều hướng theo sơ đồ chi tiết dưới đây:

### 📍 A. Kết nối màn hình LCD ILI9341 (Giao tiếp SPI1 + GPIO)

Màn hình LCD ILI9341 sử dụng chuẩn giao tiếp SPI1 tốc độ cao (25 MHz) kết hợp với các chân GPIO thông thường để điều khiển:

| Chân trên LCD ILI9341 | Chân trên STM32F411 | Chức năng tín hiệu | Loại tín hiệu trên STM32 |
| :--- | :--- | :--- | :--- |
| **VCC** | **3.3V** hoặc **5V** | Cấp nguồn cho LCD | Nguồn DC |
| **GND** | **GND** | Nối đất chung | Nối đất |
| **CS** (Chip Select) | **PA4** | Chọn chip tích cực mức THẤP | GPIO Output (Mức 0 = Chọn) |
| **RESET** (RES) | **PA2** | Reset cứng màn hình tích cực mức THẤP | GPIO Output (Mức 0 = Reset) |
| **D/C** (Data/Command) | **PA3** | Phân biệt Lệnh và Dữ liệu | GPIO Output (Mức 0 = Lệnh, 1 = Dữ liệu) |
| **SDI (MOSI)** | **PA7** | Đường truyền dữ liệu SPI Master Out | SPI1 Hardware (MOSI) |
| **SCK** (Clock) | **PA5** | Đường truyền xung nhịp SPI Clock | SPI1 Hardware (SCK) |
| **LED** (Backlight) | **3.3V** | Cấp nguồn cho đèn nền LCD | Nguồn DC |
| **SDO (MISO)** | Không kết nối | Đường nhận dữ liệu (không dùng) | — |

---

### 📍 B. Kết nối 6 nút nhấn điều hướng (Cổng GPIOC)

Bộ 6 phím nhấn được kết nối ở chế độ ngắt ngoài EXTI với trở kéo lên tích hợp sẵn trên chip (`GPIO_PULLUP`). Một đầu nút nhấn nối vào chân vi điều khiển, đầu còn lại nối trực tiếp vào chân **GND**:

| Tên nút nhấn | Chân trên STM32 | Sự kiện ngắt kích hoạt | Chức năng điều hướng chính |
| :--- | :--- | :--- | :--- |
| **BTN_UP** | **PC0** | EXTI0 (Cạnh xuống) | Di chuyển lên trong Menu / Cuộn thông tin Credits lên |
| **BTN_DOWN** | **PC1** | EXTI1 (Cạnh xuống) | Di chuyển xuống trong Menu / Cuộn thông tin Credits xuống |
| **BTN_LEFT** | **PC2** | EXTI2 (Cạnh xuống) | Phát bài hát phía trước (Tua lùi) |
| **BTN_SELECT** | **PC4** | EXTI4 (Cạnh xuống) | Chọn mục / Tạm dừng (Pause) hoặc tiếp tục (Play) phát nhạc |
| **BTN_BACK** | **PC5** | EXTI9_5 (Cạnh xuống) | Quay trở về Menu hoặc Hub chính phía trước |
| **BTN_RIGHT** | **PC6** | EXTI9_5 (Cạnh xuống) | Phát bài hát tiếp theo (Tua tiến) |

---

### 📍 C. Kết nối các Động cơ Bước & Driver DRV8825

Tín hiệu xung phát âm thanh từ các Timer hệ thống tới chân STEP và chân điều khiển chiều quay DIR trên các Driver DRV8825:

| Bè Nhạc | Timer | Chân STEP (Xung nhịp) | Chân DIR (Chiều quay) | Kênh Driver |
| :--- | :--- | :--- | :--- | :--- |
| **Melody (Bè chính)** | **TIM1_CH1** | **PA8** | **PB0** | Driver #1 |
| **Harmony (Bè đệm)**  | **TIM2_CH1** | **PA0** | **PB1** | Driver #2 |
| **Bass (Bè trầm)**     | **TIM9_CH1** | **PE5** | **PB2** | Driver #3 |

---

## 📐 2. Kiến trúc mã nguồn & Nguyên lý hoạt động (Software Architecture)

Hệ thống được thiết kế theo kiến trúc **Hybrid (Lai)** tối ưu hóa tài nguyên cực kỳ chặt chẽ:

```
                  ┌─────────────────────────────────────┐
                  │              main.c                 │
                  │ (Khởi chạy HAL, FSM UI & Redraw)     │
                  └──────────────────┬──────────────────┘
                                     │
                 Gửi lệnh và         │         Nhận sự kiện
                 tần số nhạc         │         nút nhấn
                                     ▼
                  ┌─────────────────────────────────────┐
                  │             player.c                │
                  │ (TIM1/2/9 PWM, TIM4 Sequencer 1ms)  │
                  └──────────────────┬──────────────────┘
                                     │
                                     ▼
                  ┌─────────────────────────────────────┐
                  │          Các Động Cơ Bước           │
                  │  (Rung động cơ học tạo âm thanh)    │
                  └─────────────────────────────────────┘
```

### 1. Phân tách ngoại vi TIM4 và SysTick:
*   **SysTick Interrupt:** Chạy ngầm từ thư viện HAL để cung cấp trục thời gian chính xác cho các hàm `HAL_Delay()` và `HAL_GetTick()`. Nó được khởi động ngay đầu hàm `main()` bằng lệnh `HAL_Init()`.
*   **TIM4 1ms IRQ (Bare-metal):** Chạy độc lập hoàn toàn để định thì cho sequencer nốt nhạc của 3 kênh motor.
*   **Hiệu quả:** Cả hai hệ thống ngắt co-exist song song mượt mà, không gặp bất cứ xung đột linker hayタイミング jitter nào nhờ tách biệt biểu tượng ngắt.

### 2. Loại bỏ chế độ truyền dữ liệu DMA:
*   **Vấn đề:** Dự án STM32CubeIDE gốc sử dụng `HAL_SPI_Transmit_DMA` nhưng chưa cấu hình bộ điều khiển DMA (`hdma_spi1_tx`) cũng như các trình phục vụ ngắt DMA, dẫn đến lỗi **Hard Fault** làm đứng kit vi điều khiển.
*   **Giải pháp:** Chuyển đổi toàn bộ các hàm truyền tải hình chữ nhật hoặc pixel sang chế độ **Polling (`HAL_SPI_Transmit`)** ở xung nhịp **25 MHz**. Với kích thước màn hình nhỏ 320x240, chế độ Polling chạy cực kỳ mượt mà, loại bỏ hoàn toàn mã DMA phức tạp và mang lại sự ổn định tuyệt đối 100%.

### 3. Cơ chế dập dội nút bấm & Hàng đợi Critical Section:
*   Khi có nút nhấn, ngắt ngoài EXTI tương ứng sẽ được gọi.
*   Trình phục vụ ngắt đọc thời gian thực tế bằng `HAL_GetTick()`, nếu khoảng cách với lần nhấn phím trước đó lớn hơn **200ms**, sự kiện phím mới được chấp nhận (Dập dội phần mềm - Debouncing).
*   Sự kiện phím hợp lệ được đẩy vào biến hàng đợi toàn cục `pending_event`.
*   Trong hàm `main()`, việc lấy sự kiện ra để xử lý được bọc trong khối Critical Section (`__disable_irq()` và `__enable_irq()`) để ngăn ngừa tranh chấp dữ liệu (Race Condition) với tiến trình ngắt nút bấm.

---

## 🖥️ 3. Mô tả các màn hình Giao diện (FSM UI States)

Giao diện đồ họa được thiết kế sắc nét trên nền tối (Sleek Dark Mode) sử dụng màu sắc Tailored hài hòa và hỗ trợ chuyển động mượt mà:

1.  **`STATE_MAIN_CHOICE` (Màn hình chính):**
    *   Hiển thị logo tiêu đề "SYSTEM MAIN HUB" màu xanh dương.
    *   Cho phép lựa chọn giữa **"1. MENU MUSIC"** hoặc **"2. CREDITS TEAM"**.
    *   Mục đang được chọn sẽ được bôi sáng bằng khung màu Vàng nổi bật với chữ màu Đen.
2.  **`STATE_MENU` (Danh sách nhạc):**
    *   Hiển thị danh sách 3 bài hát trượt trong cửa sổ hiển thị.
    *   Hỗ trợ **tự động cuộn chữ (autoscroll) tên bài hát** sang bên trái nếu tên bài hát dài hơn 24 ký tự, giúp hiển thị đầy đủ tiêu đề bài hát một cách thông minh.
    *   Hỗ trợ chuyển trang cuộn lên/cuộn xuống nhịp nhàng khi di chuyển con trỏ lựa chọn.
3.  **`STATE_MUSIC_PLAYER` (Trình phát nhạc - Now Playing):**
    *   Giao diện tuyệt đẹp với tiêu đề "NOW PLAYING" màu xanh lá cây đậm.
    *   Hiển thị tên bài hát đang phát (hỗ trợ cuộn chữ động).
    *   Hiển thị trạng thái phát nhạc thời gian thực: Biểu tượng ` > ` màu xanh lá khi nhạc đang phát, và `||` màu đỏ khi nhạc tạm dừng.
    *   Tự động trễ **2 giây** trước khi phát nhạc khi người dùng chuyển bài để hệ thống cơ học ổn định.
4.  **`STATE_CREDITS` (Thông tin nhóm):**
    *   Hiển thị thông tin đồ án, giảng viên hướng dẫn và danh sách 6 thành viên thực hiện đồ án.
    *   Cho phép người dùng bấm nút Lên/Xuống để cuộn xem danh sách thành viên một cách mượt mà.

---

## ⚡ 4. Hướng dẫn nạp chương trình & Khắc phục lỗi

### 🚀 Cách nạp chương trình bằng VS Code PlatformIO:
1. Kết nối board vi điều khiển STM32F411 với máy tính qua cổng ST-LINK USB.
2. Mở dự án trong VS Code.
3. Mở Terminal trong VS Code và chạy lệnh nạp sau để PlatformIO tự động nạp firmware xuống kit:
   ```powershell
   C:\Users\PC\.platformio\penv\Scripts\pio.exe run --target upload
   ```

### 🔧 Các lưu ý khắc phục lỗi hiển thị (Troubleshooting):
*   **Màn hình bị trắng/không hiển thị:** Kiểm tra xem các chân `PA2` (RESET), `PA3` (DC), `PA4` (CS) đã nối đúng và chắc chắn chưa. Đảm bảo nguồn cấp cho LCD đủ (3.3V hoặc 5V).
*   **Chữ cuộn bị giật lag:** Tốc độ cuộn chữ được cấu hình định thì định kỳ mỗi **350ms** trong vòng lặp `while(1)` ở `main.c` dựa trên hàm `HAL_GetTick()`. Đảm bảo không thêm các hàm delay dài (`HAL_Delay`) bên trong vòng lặp chính làm chặn tiến trình quét giao diện.
*   **Nút bấm bị loạn hoặc nhảy phím:** Hãy kiểm tra xem bạn có nối nhầm đầu nút bấm vào nguồn 3.3V thay vì chân GND hay không. Các phím bấm bắt buộc phải được đấu nối một đầu vào chân vi điều khiển và một đầu vào **GND**.

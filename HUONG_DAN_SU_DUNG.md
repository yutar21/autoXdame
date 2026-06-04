# 🎮 HƯỚNG DẪN SỬ DỤNG HỆ THỐNG PICO MACRO

## 📋 MỤC LỤC
1. [Giới thiệu](#giới-thiệu)
2. [Yêu cầu phần cứng](#yêu-cầu-phần-cứng)
3. [Yêu cầu phần mềm](#yêu-cầu-phần-mềm)
4. [Cài đặt môi trường](#cài-đặt-môi-trường)
5. [Build firmware cho Pico](#build-firmware)
6. [Flash firmware vào Pico](#flash-firmware)
7. [Biên dịch ứng dụng Windows](#biên-dịch-windows)
8. [Sử dụng hệ thống](#sử-dụng)
9. [Tùy chỉnh macro](#tùy-chỉnh-macro)
10. [Xử lý lỗi thường gặp](#xử-lý-lỗi)

---

## 🔍 GIỚI THIỆU

Hệ thống **Pico Macro** cho phép bạn sử dụng RP2040-ZERO (hoặc Raspberry Pi Pico) để:
- ✅ Thực thi combo phím/chuột tự động
- ✅ Kích hoạt bằng 1 phím bấm (mặc định: Q)
- ✅ Hardware-based, khó phát hiện
- ✅ Độ trễ thấp (~1-2ms)
- ✅ Hoạt động như USB HID device thật

**Cơ chế hoạt động:**
```
Windows PC ←──USB──→ RP2040-ZERO (hoặc Raspberry Pi Pico)
     ↓                        ↓
  Nhấn Q              Nhận lệnh 0x51
     ↓                        ↓
Gửi HID Report        Thực thi macro
                    (gửi phím/chuột)
```

---

## 🔧 YÊU CẦU PHẦN CỨNG

### **Bắt buộc:**
1. **RP2040-ZERO** (hoặc Raspberry Pi Pico/Pico W)
   - Giá: ~40,000 - 80,000 VNĐ
   - Mua tại: Shopee, Lazada, các shop linh kiện điện tử
   - Chip: RP2040 dual-core ARM Cortex-M0+

2. **Cáp USB Type-C** (kết nối RP2040-ZERO → PC)
   - **Lưu ý**: RP2040-ZERO dùng cổng USB Type-C (không phải Micro-B như Pico)
   - Đảm bảo cáp có chức năng truyền dữ liệu (không chỉ sạc)

### **Khuyến nghị:**
- Hub USB (nếu PC thiếu cổng USB Type-C)
- Vỏ case cho RP2040-ZERO (tùy chọn, để bảo vệ - RP2040-ZERO rất nhỏ và dễ hỏng)

---

## 💻 YÊU CẦU PHẦN MỀM

### **Hệ điều hành:**
- Windows 10/11 (64-bit)
- *(Có thể chạy trên Linux/Mac nhưng cần sửa code Windows app)*

### **Công cụ cần cài đặt:**

#### **A. Để build firmware Pico:**
1. **Pico SDK** - Bộ công cụ phát triển chính thức
2. **CMake** (≥ 3.13) - Build system
3. **ARM GCC Compiler** - Trình biên dịch cho ARM Cortex-M0+
4. **Git** - Quản lý mã nguồn
5. **Python 3** - Cần thiết cho một số script của SDK

#### **B. Để build ứng dụng Windows:**
1. **Visual Studio 2019/2022** (Community Edition - miễn phí)
   - Hoặc **MinGW-w64** (GCC cho Windows)
2. **Windows SDK** (đã bao gồm trong Visual Studio)

---

## 🛠️ CÀI ĐẶT MÔI TRƯỜNG

### **BƯỚC 1: Cài đặt Pico SDK**

#### **Cách 1: Tự động (Windows - Khuyến nghị)**

1. Tải **Pico Setup for Windows**:
   ```
   https://github.com/raspberrypi/pico-setup-windows/releases/latest
   ```

2. Chạy file `.exe` và làm theo hướng dẫn
   - ✅ Chọn: Install Pico SDK
   - ✅ Chọn: Install ARM GCC
   - ✅ Chọn: Install CMake
   - ✅ Chọn: Install Visual Studio Code (tùy chọn)

3. Khởi động lại máy tính

#### **Cách 2: Thủ công**

**Bước 1.1: Cài đặt Git**
```bash
https://git-scm.com/download/win
```

**Bước 1.2: Cài đặt CMake**
```bash
https://cmake.org/download/
```
- Chọn "Add CMake to system PATH" khi cài

**Bước 1.3: Cài đặt ARM GCC**
```bash
https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
```
- Tải phiên bản: `arm-none-eabi-gcc` (Windows x86_64)
- Giải nén vào `C:\Program Files\arm-gcc`
- Thêm vào PATH: `C:\Program Files\arm-gcc\bin`

**Bước 1.4: Clone Pico SDK**
```bash
cd C:\
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
```

**Bước 1.5: Thiết lập biến môi trường**
- Mở "Edit System Environment Variables"
- Thêm biến mới:
  - Tên: `PICO_SDK_PATH`
  - Giá trị: `C:\pico-sdk`

---

### **BƯỚC 2: Cài đặt Visual Studio**

1. Tải Visual Studio Community:
   ```
   https://visualstudio.microsoft.com/downloads/
   ```

2. Khi cài đặt, chọn workload:
   - ✅ **Desktop development with C++**
   - ✅ **Windows SDK**

3. Khởi động lại máy

---

### **BƯỚC 3: Kiểm tra cài đặt**

Mở **Command Prompt** và chạy:

```bash
# Kiểm tra CMake
cmake --version
# Kết quả mong đợi: cmake version 3.x.x

# Kiểm tra ARM GCC
arm-none-eabi-gcc --version
# Kết quả mong đợi: arm-none-eabi-gcc (GCC) x.x.x

# Kiểm tra PICO_SDK_PATH
echo %PICO_SDK_PATH%
# Kết quả mong đợi: C:\pico-sdk
```

✅ **Nếu tất cả đều trả về kết quả → Cài đặt thành công!**

---

## 🔨 BUILD FIRMWARE CHO PICO

### **BƯỚC 1: Copy file pico_sdk_import.cmake**

Nếu chưa có file này trong thư mục `pico_macro`:

```bash
cd d:\duk\autoXdame\pico_macro
copy %PICO_SDK_PATH%\external\pico_sdk_import.cmake .
```

### **BƯỚC 2: Tạo thư mục build**

```bash
cd d:\duk\autoXdame\pico_macro
mkdir build
cd build
```

### **BƯỚC 3: Chạy CMake**

```bash
cmake -G "NMake Makefiles" ..
```

**Lưu ý:**
- Nếu dùng MinGW: `cmake -G "MinGW Makefiles" ..`
- Nếu dùng Visual Studio: `cmake -G "Visual Studio 17 2022" -A ARM ..`

### **BƯỚC 4: Build firmware**

```bash
nmake
```

Hoặc với MinGW:
```bash
mingw32-make
```

### **BƯỚC 5: Kiểm tra kết quả**

Sau khi build thành công, bạn sẽ có các file:
```
build/
├── pico_macro.elf
├── pico_macro.bin
├── pico_macro.uf2  ← FILE QUAN TRỌNG NHẤT
├── pico_macro.hex
└── pico_macro.map
```

✅ **File `pico_macro.uf2` là firmware cần flash vào Pico**

---

## 📥 FLASH FIRMWARE VÀO PICO

### **BƯỚC 1: Đưa RP2040-ZERO vào chế độ BOOT**

1. **RÚT cáp USB** khỏi RP2040-ZERO (nếu đang cắm)

2. **GIỮ NÚT BOOT** trên RP2040-ZERO:
   ```
   ┌──────────────┐
   │  RP2040-ZERO │
   │    (mặt sau)  │
   │              │
   │   [BOOT]     │ ← Nút nhỏ ở mặt sau board (hoặc có thể ở cạnh)
   │              │
   │  [USB Type-C]│
   └──────────────┘
   ```
   **Lưu ý**: RP2040-ZERO nhỏ hơn Pico, nút BOOT thường ở mặt sau hoặc cạnh board

3. **TRONG KHI GIỮ NÚT**, cắm cáp USB Type-C vào máy tính

4. **GIỮ THÊM 2-3 GIÂY**, sau đó thả tay

5. RP2040-ZERO sẽ xuất hiện như **ổ đĩa USB** có tên `RPI-RP2`

### **BƯỚC 2: Copy file .uf2**

**Cách 1: Kéo thả (Dễ nhất)**
1. Mở File Explorer
2. Tìm ổ đĩa `RPI-RP2`
3. Kéo file `pico_macro.uf2` vào ổ đĩa này

**Cách 2: Command line**
```bash
copy d:\duk\autoXdame\pico_macro\build\pico_macro.uf2 E:\
```
*(Thay `E:` bằng ổ đĩa RPI-RP2 của bạn)*

### **BƯỚC 3: Chờ flash hoàn tất**

- Ổ đĩa `RPI-RP2` sẽ **tự động biến mất**
- RP2040-ZERO sẽ **tự động khởi động lại**
- LED trên RP2040-ZERO có thể nhấp nháy (tùy code)

✅ **Flash thành công! RP2040-ZERO giờ đang chạy firmware macro**

---

## 🖥️ BIÊN DỊCH ỨNG DỤNG WINDOWS

**LƯU Ý:** Có 2 phiên bản:
- **Hook version** (cũ): Latency ~1-2ms
- **Raw Input version** (mới): Latency ~0.2-0.5ms ⚡ **Khuyến nghị**

### **Phương án 1: Dùng batch script (Khuyến nghị)**

1. Mở **Developer Command Prompt for VS 2022**
2. Chạy:
```cmd
cd d:\duk\autoXdame\pico_macro\scripts
build_windows_app.bat
```

3. Kết quả:
   - `send_q_hook.exe` - Phiên bản gốc
   - `send_q_rawinput.exe` - **Phiên bản tối ưu (dùng cái này)**

### **Phương án 2: Dùng Visual Studio (Hook version)**

1. Mở **Visual Studio 2022**
2. File → New → Project from Existing Code
3. Chọn thư mục: `d:\duk\autoXdame\pico_macro\scripts`
4. Chọn file: `send_q_cpp.cpp`
5. Nhấn **Build** (Ctrl+Shift+B)

File `send_q.exe` sẽ được tạo trong thư mục `Debug\` hoặc `Release\`

### **Phương án 3: Dùng Command Line (Raw Input - Tối ưu)**

**Build phiên bản tối ưu (khuyến nghị):**

```bash
cd d:\duk\autoXdame\pico_macro\scripts

cl.exe send_q_rawinput.cpp ^
  /link setupapi.lib hid.lib user32.lib ^
  /out:send_q_rawinput.exe
```

**Build phiên bản gốc:**

```bash
cd d:\duk\autoXdame\pico_macro\scripts

cl.exe send_q_cpp.cpp ^
  /link setupapi.lib hid.lib ^
  /out:send_q_hook.exe
```

### **Phương án 4: Dùng MinGW**

**Raw Input version (khuyến nghị):**
```bash
cd d:\duk\autoXdame\pico_macro\scripts

g++ send_q_rawinput.cpp -o send_q_rawinput.exe ^
  -lsetupapi -lhid -luser32 -static
```

**Hook version:**
```bash
g++ send_q_cpp.cpp -o send_q_hook.exe ^
  -lsetupapi -lhid -static
```

---

## 📊 SO SÁNH 2 PHIÊN BẢN WINDOWS APP

| Tính năng | Hook Version | Raw Input Version |
|-----------|-------------|-------------------|
| **File** | `send_q_hook.exe` | `send_q_rawinput.exe` |
| **Latency** | ~1-2ms | **~0.2-0.5ms** ⚡ |
| **Admin required** | ⚠️ Đôi khi | ❌ Không |
| **Antivirus** | ⚠️ Có thể bị chặn | ✅ Ít bị chặn |
| **CPU usage** | Thấp | Rất thấp |
| **Khuyến nghị** | - | ✅ **Dùng cái này** |

**📖 Chi tiết:** Xem `scripts/README_RAWINPUT.md`

---

## ▶️ SỬ DỤNG HỆ THỐNG

### **BƯỚC 1: Kết nối RP2040-ZERO**

1. **CẮM RP2040-ZERO vào USB** (không cần giữ nút BOOT lần này)
2. Windows sẽ tự động nhận thiết bị HID
3. Không cần cài driver

### **BƯỚC 2: Chạy ứng dụng Windows**

**Raw Input version (khuyến nghị - nhanh nhất):**
```bash
cd d:\duk\autoXdame\pico_macro\scripts
send_q_rawinput.exe
```

**Hook version (phiên bản gốc):**
```bash
cd d:\duk\autoXdame\pico_macro\scripts
send_q_hook.exe
```

**Kết quả mong đợi (Raw Input):**
```
=== Pico Macro Controller (RAW INPUT - OPTIMIZED) ===
Searching for Pico device...
[SCAN] Found HID: VID=0x2E8A PID=0x0005
[OK] Found Pico device!
[CONNECT] Connected to Pico (HID mode)
[OK] Raw Input registered successfully
[INFO] Lower latency mode enabled (~0.5ms faster)
[INFO] Press Q key on keyboard to trigger macro
[INFO] Close this window to exit
[PERF] Optimized mode: Raw Input (lowest latency)
```

**Kết quả mong đợi (Hook):**
```
=== Pico Macro Controller (USB HID) ===
Searching for Pico device...
[SCAN] Found HID: VID=0x2E8A PID=0x0005
[OK] Found Pico device!
[CONNECT] Connected to Pico (HID mode)
[INFO] Press Q key on keyboard to trigger macro
[INFO] Close this window to exit
```

### **BƯỚC 3: Kích hoạt macro**

1. **Click vào bất kỳ ứng dụng nào** (game, notepad, v.v.)
2. **Nhấn phím Q** trên bàn phím
3. Macro sẽ tự động chạy:
   ```
   Nhấn 7 → Click phải → Click trái → Nhấn 8 → Click phải
   ```

**Console sẽ hiển thị:**
```
[SENT] Trigger sent to Pico
```

### **BƯỚC 4: Dừng chương trình**

- Nhấn `Ctrl+C` hoặc đóng cửa sổ console
- Ngắt kết nối USB nếu không dùng nữa

---

## ✏️ TÙY CHỈNH MACRO

### **Thay đổi combo macro**

1. Mở file `d:\duk\autoXdame\pico_macro\src\main.c`

2. Tìm dòng 35-46:
```c
static const combo_step_t combo_full[] = {
    {COMBO_ACTION_KEY_PRESS, HID_KEY_7, 0, 8},
    {COMBO_ACTION_KEY_RELEASE, 0, 0, 5},
    {COMBO_ACTION_MOUSE_DOWN, 0, RIGHT_BTN, 10},
    {COMBO_ACTION_MOUSE_UP, 0, 0, 90},
    {COMBO_ACTION_MOUSE_DOWN, 0, LEFT_BTN, 10},
    {COMBO_ACTION_MOUSE_UP, 0, 0, 5},
    {COMBO_ACTION_KEY_PRESS, HID_KEY_8, 0, 8},
    {COMBO_ACTION_KEY_RELEASE, 0, 0, 5},
    {COMBO_ACTION_MOUSE_DOWN, 0, RIGHT_BTN, 10},
    {COMBO_ACTION_MOUSE_UP, 0, 0, 0},
};
```

### **Các action có sẵn:**

| Action | Mô tả | Tham số |
|--------|-------|---------|
| `COMBO_ACTION_KEY_PRESS` | Nhấn phím | `keycode`, delay_ms |
| `COMBO_ACTION_KEY_RELEASE` | Nhả tất cả phím | delay_ms |
| `COMBO_ACTION_MOUSE_DOWN` | Nhấn chuột | `buttons`, delay_ms |
| `COMBO_ACTION_MOUSE_UP` | Nhả chuột | delay_ms |

### **Mã phím thông dụng:**

```c
// Chữ cái
HID_KEY_A = 0x04
HID_KEY_B = 0x05
...
HID_KEY_Z = 0x1D

// Số
HID_KEY_1 = 0x1E
HID_KEY_2 = 0x1F
...
HID_KEY_9 = 0x26
HID_KEY_0 = 0x27

// Chữ số Numpad
HID_KEY_KEYPAD_1 = 0x59
...

// Phím đặc biệt
HID_KEY_ESCAPE        = 0x29
HID_KEY_SPACE         = 0x2C
HID_KEY_SHIFT_LEFT    = 0xE1
HID_KEY_CONTROL_LEFT  = 0xE0
HID_KEY_ALT_LEFT      = 0xE2

// F-keys
HID_KEY_F1 = 0x3A
HID_KEY_F2 = 0x3B
...
HID_KEY_F12 = 0x45
```

**Danh sách đầy đủ:** Xem file `tusb.h` trong Pico SDK

### **Nút chuột:**
```c
#define LEFT_BTN   0x01   // Chuột trái
#define RIGHT_BTN  0x02   // Chuột phải
#define MIDDLE_BTN 0x04   // Chuột giữa
```

### **Ví dụ: Combo QWER (skill trong game)**

```c
static const combo_step_t combo_full[] = {
    // Q
    {COMBO_ACTION_KEY_PRESS, HID_KEY_Q, 0, 50},
    {COMBO_ACTION_KEY_RELEASE, 0, 0, 100},
    
    // W
    {COMBO_ACTION_KEY_PRESS, HID_KEY_W, 0, 50},
    {COMBO_ACTION_KEY_RELEASE, 0, 0, 100},
    
    // E
    {COMBO_ACTION_KEY_PRESS, HID_KEY_E, 0, 50},
    {COMBO_ACTION_KEY_RELEASE, 0, 0, 100},
    
    // R
    {COMBO_ACTION_KEY_PRESS, HID_KEY_R, 0, 50},
    {COMBO_ACTION_KEY_RELEASE, 0, 0, 0},
};
```

### **Ví dụ: Combo có giữ phím Shift**

```c
static const combo_step_t combo_full[] = {
    // Giữ Shift
    {COMBO_ACTION_KEY_PRESS, HID_KEY_SHIFT_LEFT, 0, 10},
    
    // Nhấn 1 (Shift+1)
    {COMBO_ACTION_KEY_PRESS, HID_KEY_1, 0, 50},
    {COMBO_ACTION_KEY_RELEASE, 0, 0, 50},
    
    // Nhả Shift
    {COMBO_ACTION_KEY_RELEASE, 0, 0, 0},
};
```

### **Sau khi sửa code:**

1. Build lại firmware:
```bash
cd d:\duk\autoXdame\pico_macro\build
nmake
```

2. Flash lại vào Pico (xem [FLASH FIRMWARE](#flash-firmware))

3. Chạy lại `send_q.exe`

---

## ⚙️ TÙY CHỈNH PHÍM KÍCH HOẠT

### **Đổi từ Q sang phím khác (ví dụ: F1)**

**File: `scripts/send_q_cpp.cpp`**

Tìm dòng 19:
```cpp
if (kb->vkCode == 0x51)  // 0x51 = Q
```

Đổi thành:
```cpp
if (kb->vkCode == 0x70)  // 0x70 = F1
```

**Virtual Key Codes phổ biến:**
```cpp
// Chữ cái
0x41 = A
0x42 = B
...
0x51 = Q
...
0x5A = Z

// Số
0x30 = 0
0x31 = 1
...

// F-keys
0x70 = F1
0x71 = F2
...
0x7B = F12

// Phím đặc biệt
0x20 = Space
0x0D = Enter
0x1B = Escape
```

**Danh sách đầy đủ:** https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

Sau khi sửa, build lại `send_q.exe`:
```bash
cd d:\duk\autoXdame\pico_macro\scripts
cl.exe send_q_cpp.cpp /link setupapi.lib hid.lib /out:send_q.exe
```

---

## 🐛 XỬ LÝ LỖI THƯỜNG GẶP

### **❌ Lỗi 1: "CMake Error: Could not find PICO_SDK_PATH"**

**Nguyên nhân:** Biến môi trường chưa được thiết lập

**Giải pháp:**
1. Kiểm tra:
```bash
echo %PICO_SDK_PATH%
```

2. Nếu trống, thiết lập thủ công:
```bash
setx PICO_SDK_PATH "C:\pico-sdk"
```

3. Khởi động lại Command Prompt

---

### **❌ Lỗi 2: "arm-none-eabi-gcc is not recognized"**

**Nguyên nhân:** ARM GCC chưa có trong PATH

**Giải pháp:**
1. Tìm thư mục cài đặt ARM GCC (ví dụ: `C:\Program Files\ARM\bin`)
2. Thêm vào PATH:
   - Windows Settings → System → Advanced → Environment Variables
   - Chọn PATH → Edit → New
   - Thêm: `C:\Program Files\ARM\bin`
3. Khởi động lại Command Prompt

---

### **❌ Lỗi 3: "[ERROR] Pico HID device not found"**

**Nguyên nhân:**
- RP2040-ZERO chưa được flash firmware
- Firmware bị lỗi
- Cáp USB Type-C chỉ có chức năng sạc (không có data)

**Giải pháp:**
1. **Flash lại firmware:**
   - Rút USB, giữ nút BOOT, cắm lại
   - Copy lại file `.uf2`

2. **Kiểm tra Device Manager:**
   - Mở Device Manager (Win+X → Device Manager)
   - Tìm "Human Interface Devices"
   - Phải có thiết bị với VID=2E8A

3. **Thử cổng USB khác** (ưu tiên cổng USB 2.0)

4. **Đổi cáp USB Type-C** (đảm bảo cáp có hỗ trợ data, không chỉ sạc)
   - **Lưu ý**: Nhiều cáp Type-C chỉ có chức năng sạc nhanh

---

### **❌ Lỗi 4: "HID write failed"**

**Nguyên nhân:** Quyền truy cập bị chặn

**Giải pháp:**
1. Chạy `send_q.exe` **với quyền Administrator**:
   - Right-click → Run as administrator

2. Tắt phần mềm chặn (antivirus, firewall)

---

### **❌ Lỗi 5: Macro không chạy đúng timing**

**Nguyên nhân:**
- Delay quá ngắn
- Game có lag
- USB polling rate thấp

**Giải pháp:**
1. **Tăng delay trong macro:**
```c
{COMBO_ACTION_KEY_PRESS, HID_KEY_Q, 0, 100},  // Tăng từ 50 → 100
```

2. **Giảm tải game** (tắt hiệu ứng đồ họa cao)

3. **Kiểm tra USB polling rate:**
   - Device Manager → USB Root Hub → Properties
   - Power Management → Bỏ tick "Allow computer to turn off this device"

---

### **❌ Lỗi 6: Macro chạy nhiều lần khi nhấn Q 1 lần**

**Nguyên nhân:** Code firmware có bug hoặc key repeat

**Giải pháp:**
1. **Kiểm tra flag `combo_in_progress`:**
   - File `main.c` dòng 165 đã có check:
   ```c
   if (buffer[0] == CMD_MACRO_TRIGGER && !combo_in_progress) {
   ```
   
2. **Nhấn Q nhẹ tay hơn** (tránh giữ)

3. **Thêm debounce vào Windows app** (nâng cao)

---

### **❌ Lỗi 7: Windows Defender cảnh báo virus**

**Nguyên nhân:** 
- Keyboard hook bị phát hiện là suspicious
- File chưa có chữ ký số

**Giải pháp:**
1. **Thêm exception vào Windows Defender:**
   - Settings → Privacy & Security → Windows Security
   - Virus & threat protection → Exclusions
   - Add: `d:\duk\autoXdame\pico_macro\scripts\send_q.exe`

2. **Tự build từ source** (đáng tin hơn file .exe từ internet)

---

## 🔧 DEBUGGING & TESTING

### **Kiểm tra Pico có kết nối không:**

**Windows PowerShell:**
```powershell
Get-PnpDevice -Class HIDClass | Where-Object {$_.InstanceId -like "*VID_2E8A*"}
```

**Kết quả mong đợi:**
```
Status  Class  FriendlyName
------  -----  ------------
OK      HIDClass  HID-compliant device
```

### **Test macro thủ công:**

1. Mở **Notepad**
2. Chạy `send_q.exe`
3. Nhấn Q
4. Xem kết quả trong Notepad (phải thấy ký tự 7 và 8)

---

## 📚 TÀI LIỆU THAM KHẢO

- **Pico SDK Documentation:** https://raspberrypi.github.io/pico-sdk-doxygen/
- **USB HID Usage Tables:** https://usb.org/sites/default/files/hut1_21.pdf
- **TinyUSB Documentation:** https://docs.tinyusb.org/
- **Windows Virtual Key Codes:** https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

---

## ⚠️ LƯU Ý QUAN TRỌNG

### **Pháp lý:**
- ⚖️ Sử dụng macro trong game online có thể **vi phạm điều khoản dịch vụ**
- 🚫 Có thể bị **cấm tài khoản** vĩnh viễn
- 👮 Một số quốc gia coi việc sử dụng cheat trong game là **vi phạm pháp luật**

### **Đạo đức:**
- 🎮 Macro tạo **lợi thế không công bằng** cho người chơi khác
- 🏆 Chiến thắng nhờ macro **không phải chiến thắng thật sự**
- 🤝 Hãy tôn trọng người chơi khác

### **Khuyến nghị:**
- ✅ Chỉ dùng để **học tập, nghiên cứu**
- ✅ Test trong **single-player mode** hoặc **private server**
- ✅ **KHÔNG sử dụng trong competitive gaming**

---

## 📞 HỖ TRỢ

Nếu gặp vấn đề không giải quyết được:

1. **Kiểm tra lại từng bước** trong hướng dẫn
2. **Đọc phần xử lý lỗi** ở trên
3. **Google lỗi cụ thể** (thường đã có người gặp)
4. **Hỏi trên diễn đàn:**
   - Raspberry Pi Forum
   - Reddit: r/raspberry_pi
   - Stack Overflow

---

## 📝 CHANGELOG

**v1.0** (2024)
- ✅ Build hệ thống cơ bản
- ✅ Hỗ trợ 1 macro combo
- ✅ Kích hoạt bằng phím Q

**Tính năng sắp tới:**
- 🔜 Nhiều macro presets (F1-F12)
- 🔜 GUI config tool
- 🔜 Lưu config vào file
- 🔜 Random timing (anti-detect)

---

**📅 Cập nhật lần cuối:** 2026-06-04
**✍️ Người viết:** Auto-generated documentation

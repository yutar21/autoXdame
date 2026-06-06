# 🎮 HƯỚNG DẪN SỬ DỤNG - PICO MACRO

## Tổng quan

RP2040-ZERO hoạt động như **bàn phím + chuột USB thật** (HID device).  
Nhấn phím **Q** → trigger gửi đến RP2040 → macro tự động chạy.

```
[Bàn phím] → nhấn Q
      ↓
[send_q_rawinput.exe] → gửi trigger qua USB HID
      ↓
[RP2040-ZERO] → thực thi combo (chuột/phím)
```

---

## Cấu trúc thư mục

```
pico_macro/
├── build.bat                    ← Script build firmware
├── CMakeLists.txt               ← Cấu hình build
├── pico_sdk_import.cmake
├── build/
│   └── pico_macro.uf2           ← Firmware (nạp vào RP2040)
├── scripts/
│   ├── send_q_rawinput.cpp      ← Source Windows app
│   ├── send_q_rawinput.exe      ← Windows app (chạy cái này)
│   ├── build_windows_app.bat
│   └── README_RAWINPUT.md
└── src/
    ├── main.c                   ← Firmware source
    └── tusb_config.h            ← TinyUSB config
```

---

## Sử dụng hàng ngày

### 1. Cắm RP2040-ZERO vào USB
Windows tự nhận HID device (~5 giây), không cần cài driver.

### 2. Chạy Windows app
```cmd
cd d:\duk\autoXdame\pico_macro\scripts
send_q_rawinput.exe
```

Kết quả mong đợi:
```
=== Pico Macro Controller (RAW INPUT - OPTIMIZED) ===
[SCAN] Found HID: VID=0xCAFE PID=0x4006
[OK] Found Pico device!
[CONNECT] Connected to Pico (HID mode)
[OK] Raw Input registered successfully
[INFO] Press Q key on keyboard to trigger macro
```

### 3. Nhấn Q để trigger macro
Macro hiện tại:
```
Right click (10ms) → wait 90ms → Left click (10ms) → wait 5ms
→ Press '8' (8ms) → Right click (10ms)
```

---

## Cắm sang máy khác

**Chỉ cần 3 bước:**

1. Copy file `send_q_rawinput.exe` sang máy mới
2. Cắm RP2040-ZERO vào USB (firmware đã có sẵn trong RP2040)
3. Chạy `send_q_rawinput.exe`

**Nếu báo lỗi `[ERROR] Pico HID device not found`:**
- Rút USB ra, đợi 5 giây, cắm lại
- Chạy lại `send_q_rawinput.exe`

---

## Thay đổi macro combo

Sửa file `src/main.c`, tìm phần `combo_full[]`:

```c
static const combo_step_t combo_full[] = {
    {COMBO_ACTION_MOUSE_DOWN, 0, RIGHT_BTN, 10},  // Right click, giữ 10ms
    {COMBO_ACTION_MOUSE_UP,   0, 0,         90},  // Nhả, đợi 90ms
    {COMBO_ACTION_MOUSE_DOWN, 0, LEFT_BTN,  10},  // Left click, giữ 10ms
    {COMBO_ACTION_MOUSE_UP,   0, 0,          5},  // Nhả, đợi 5ms
    {COMBO_ACTION_KEY_PRESS,  HID_KEY_8, 0,  8},  // Nhấn '8', giữ 8ms
    {COMBO_ACTION_KEY_RELEASE,0, 0,          5},  // Nhả, đợi 5ms
    {COMBO_ACTION_MOUSE_DOWN, 0, RIGHT_BTN, 10},  // Right click, giữ 10ms
    {COMBO_ACTION_MOUSE_UP,   0, 0,          0},  // Nhả, kết thúc
};
```

**Các action:**

| Action | Mô tả |
|---|---|
| `COMBO_ACTION_KEY_PRESS` | Nhấn phím |
| `COMBO_ACTION_KEY_RELEASE` | Nhả tất cả phím |
| `COMBO_ACTION_MOUSE_DOWN` | Nhấn chuột |
| `COMBO_ACTION_MOUSE_UP` | Nhả chuột |

**Nút chuột:**
```c
LEFT_BTN  = 0x01   // Chuột trái
RIGHT_BTN = 0x02   // Chuột phải
```

**Keycode thông dụng:**
```c
HID_KEY_1 đến HID_KEY_9    // Số 1-9
HID_KEY_A đến HID_KEY_Z    // Chữ a-z
HID_KEY_F1 đến HID_KEY_F12 // F1-F12
HID_KEY_SPACE              // Space
```

### Sau khi sửa: Build lại firmware

Mở **Developer Command Prompt for Pico** và chạy:
```cmd
cd /d d:\duk\autoXdame\pico_macro
build.bat
```

Nạp file `build\pico_macro.uf2` vào RP2040-ZERO:
1. Giữ **BOOT** + nhấn **RESET** → ổ đĩa `RPI-RP2` xuất hiện
2. Copy `pico_macro.uf2` vào ổ đĩa
3. Nhấn **RESET**

---

## Build lại Windows app (nếu cần)

Yêu cầu: MinGW (g++) hoặc Visual Studio

```cmd
cd d:\duk\autoXdame\pico_macro\scripts
g++ send_q_rawinput.cpp -o send_q_rawinput.exe -lsetupapi -lhid -luser32 -static
```

---

## Xử lý lỗi

| Lỗi | Nguyên nhân | Giải pháp |
|---|---|---|
| `Pico HID device not found` | Driver cache cũ | Rút cắm lại USB |
| `HID write failed` | Mất kết nối USB | Rút cắm lại, chạy lại app |
| Macro chạy nhưng không có tác dụng | Focus sai cửa sổ | Click vào cửa sổ game trước |
| Chỉ chạy khi focus vào send_q window | RIDEV_INPUTSINK không hoạt động | Chạy app với quyền Admin |

---

## Thông số kỹ thuật

- **VID/PID:** 0xCAFE / 0x4006
- **Trigger latency:** ~0.5ms (Raw Input)
- **Macro timing accuracy:** ±0.01ms (hardware timer)
- **USB HID:** Composite (Keyboard + Mouse + Vendor trong 1 interface)
- **Firmware:** Pico SDK + TinyUSB (bare metal C)

Phương án	Trigger latency	Timing accuracy	Anti-cheat	Độ khó
C firmware (hiện tại)	~1.3ms	±0.01ms	✅ Hardware	⭐⭐⭐⭐
CircuitPython	~3ms	±2ms	✅ Hardware	⭐⭐
AutoHotKey	~10ms	±10ms	❌ Software	⭐
C# SendInput	~5ms	±3ms	❌ Software	⭐⭐
Arduino Leonardo	~1ms	±0.05ms	✅ Hardware	⭐⭐
Kernel Driver	~0.1ms	±0.001ms	✅ Hardware	⭐⭐⭐⭐⭐
# send_q_keyboard.py
# Press Q on the laptop keyboard to send HID report to Pico
# Install dependencies with: pip install hid

import hid
import sys
from pynput import keyboard

# Pico USB IDs
PICO_VID = 0x2E8A   # Raspberry Pi
PICO_PID = 0x0005   # RP2040 Generic HID

def find_pico_device():
    """Find Pico HID device"""
    for device_info in hid.enumerate():
        if device_info['vendor_id'] == PICO_VID:
            return device_info
    return None

def main():
    print("=== Pico Macro Controller (USB HID) ===")
    print("Đang tìm Pico device...")
    
    device_info = find_pico_device()
    if device_info is None:
        print("Lỗi: Không tìm thấy Pico HID device")
        print("Hãy kiểm tra lại kết nối USB")
        return 1
    
    try:
        device = hid.device()
        device.open(device_info['vendor_id'], device_info['product_id'])
        print(f"Đã kết nối Pico: {device_info['product_string']}")
    except Exception as e:
        print(f"Lỗi mở device: {e}")
        return 1
    
    print("Nhấn phím Q trên bàn phím để trigger macro. Ctrl-C để thoát.")
    
    def on_press(key):
        try:
            if key.char == 'q' or key.char == 'Q':
                # Send HID report
                report = [0x00, 0x51, 0, 0, 0, 0, 0, 0, 0]  # Report ID + command
                try:
                    device.write(report)
                    print("Gửi trigger tới Pico (HID)")
                except Exception as e:
                    print(f"Lỗi gửi HID: {e}")
        except AttributeError:
            pass
    
    try:
        with keyboard.Listener(on_press=on_press) as listener:
            listener.join()
    except KeyboardInterrupt:
        print("\nĐã thoát.")
    finally:
        device.close()
    
    return 0

if __name__ == '__main__':
    sys.exit(main())


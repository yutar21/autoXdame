#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <cstdio>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "user32.lib")

// ============================================
// OPTIMIZED VERSION USING RAW INPUT
// Latency: ~0.2-0.5ms (vs 1-2ms with hooks)
// ============================================

HANDLE g_hid_device = INVALID_HANDLE_VALUE;
HWND g_hwnd = NULL;

const USHORT PICO_VID = 0xCAFE;
const USHORT PICO_PID = 0x4006;  // HID Composite (single interface)

// Pre-allocated buffer - Report ID 3 (Vendor) + trigger byte
static UCHAR g_trigger_report[9] = {3, 0x51, 0, 0, 0, 0, 0, 0, 0};  // [ReportID=3, 'Q', ...]

// Inline function for fastest possible send
inline void SendTriggerToPico()
{
    if (g_hid_device != INVALID_HANDLE_VALUE)
    {
        HidD_SetOutputReport(g_hid_device, g_trigger_report, sizeof(g_trigger_report));
    }
}

bool FindAndOpenPicoDevice()
{
    GUID hid_guid;
    ZeroMemory(&hid_guid, sizeof(hid_guid));
    
    HidD_GetHidGuid(&hid_guid);

    HDEVINFO device_info = SetupDiGetClassDevs(&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (device_info == INVALID_HANDLE_VALUE)
    {
        printf("[ERROR] Could not enumerate HID devices\n");
        return false;
    }

    SP_DEVICE_INTERFACE_DATA device_interface;
    ZeroMemory(&device_interface, sizeof(device_interface));
    device_interface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    bool found = false;

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(device_info, NULL, &hid_guid, i, &device_interface); i++)
    {
        DWORD required_size = 0;
        SetupDiGetDeviceInterfaceDetail(device_info, &device_interface, NULL, 0, &required_size, NULL);
        
        if (required_size == 0)
            continue;

        PSP_DEVICE_INTERFACE_DETAIL_DATA device_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(required_size);
        if (!device_detail)
            continue;

        device_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(device_info, &device_interface, device_detail, required_size, NULL, NULL))
        {
            free(device_detail);
            continue;
        }

        HANDLE device = CreateFileA(
            device_detail->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (device != INVALID_HANDLE_VALUE)
        {
            HIDD_ATTRIBUTES attributes;
            ZeroMemory(&attributes, sizeof(attributes));
            attributes.Size = sizeof(HIDD_ATTRIBUTES);
            
            if (HidD_GetAttributes(device, &attributes))
            {
                printf("[SCAN] Found HID: VID=0x%04X PID=0x%04X\n", attributes.VendorID, attributes.ProductID);
                fflush(stdout);
                
                if (attributes.VendorID == PICO_VID)
                {
                    printf("[OK] Found Pico device!\n");
                    fflush(stdout);
                    g_hid_device = device;
                    free(device_detail);
                    found = true;
                    break;
                }
            }
            CloseHandle(device);
        }
        free(device_detail);
    }

    SetupDiDestroyDeviceInfoList(device_info);
    
    if (!found)
    {
        printf("\n[ERROR] Pico HID device not found\n");
        printf("[CHECK] Make sure:\n");
        printf("  1. Pico is connected to USB\n");
        printf("  2. USB HID firmware is flashed to Pico\n");
        printf("  3. Try disconnect USB for 10 seconds then reconnect\n");
        fflush(stdout);
        return false;
    }
    
    return true;
}

// Pre-allocated Raw Input buffer (tránh new/delete mỗi lần nhấn phím)
static BYTE g_raw_input_buf[sizeof(RAWINPUT) + 64];

// Window procedure to handle Raw Input
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INPUT:
        {
            UINT dwSize = sizeof(g_raw_input_buf);
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT,
                g_raw_input_buf, &dwSize, sizeof(RAWINPUTHEADER)) != (UINT)-1)
            {
                RAWINPUT* raw = (RAWINPUT*)g_raw_input_buf;
                if (raw->header.dwType == RIM_TYPEKEYBOARD &&
                    raw->data.keyboard.VKey == 0x51 &&
                    !(raw->data.keyboard.Flags & RI_KEY_BREAK))
                {
                    SendTriggerToPico();
                }
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

bool RegisterRawInputDevices(HWND hwnd)
{
    // Register for keyboard input
    RAWINPUTDEVICE rid[1];
    
    rid[0].usUsagePage = 0x01;  // HID_USAGE_PAGE_GENERIC
    rid[0].usUsage = 0x06;      // HID_USAGE_GENERIC_KEYBOARD
    rid[0].dwFlags = RIDEV_INPUTSINK;  // Receive input even when not in foreground
    rid[0].hwndTarget = g_hwnd;

    if (!RegisterRawInputDevices(rid, 1, sizeof(rid[0])))
    {
        printf("[ERROR] Failed to register Raw Input devices (Error: %d)\n", GetLastError());
        return false;
    }

    printf("[OK] Raw Input registered successfully\n");
    printf("[INFO] Lower latency mode enabled (~0.5ms faster)\n");
    fflush(stdout);
    return true;
}

int main()
{
    printf("=== Pico Macro Controller (RAW INPUT - OPTIMIZED) ===\n");
    printf("Searching for Pico device...\n");
    fflush(stdout);

    if (!FindAndOpenPicoDevice())
    {
        printf("\nPress Enter to exit...\n");
        fflush(stdout);
        getchar();
        return 1;
    }

    printf("[CONNECT] Connected to Pico (HID mode)\n");
    fflush(stdout);

    // Create a message-only window for Raw Input
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "PicoMacroRawInput";

    if (!RegisterClassEx(&wc))
    {
        printf("[ERROR] Window class registration failed\n");
        fflush(stdout);
        CloseHandle(g_hid_device);
        return 1;
    }

    // Create hidden window
    g_hwnd = CreateWindowEx(
        0,
        "PicoMacroRawInput",
        "DukTLT",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!g_hwnd)
    {
        printf("[ERROR] Window creation failed\n");
        fflush(stdout);
        CloseHandle(g_hid_device);
        return 1;
    }

    // Register Raw Input
    if (!RegisterRawInputDevices(g_hwnd))
    {
        DestroyWindow(g_hwnd);
        CloseHandle(g_hid_device);
        return 1;
    }

    // Show window (optional - you can keep it hidden)
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    printf("[INFO] Press Q key on keyboard to trigger macro\n");
    printf("[INFO] Close this window to exit\n");
    printf("[PERF] Optimized mode: Raw Input (lowest latency)\n");
    fflush(stdout);

    // Set high priority for minimal latency
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);  // Bỏ TranslateMessage - không cần WM_CHAR
    }

    // Cleanup
    CloseHandle(g_hid_device);
    printf("[EXIT] Closed.\n");
    return 0;
}

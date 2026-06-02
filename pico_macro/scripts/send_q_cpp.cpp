#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <iostream>
#include <string>
#include <cstdio>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

HANDLE g_hid_device = INVALID_HANDLE_VALUE;
HHOOK g_hook = NULL;

const USHORT PICO_VID = 0x2E8A;
const USHORT PICO_PID = 0x0005;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
    {
        KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (kb->vkCode == 0x51)
        {
            if (g_hid_device != INVALID_HANDLE_VALUE)
            {
                UCHAR report[9] = {0};
                report[0] = 0x00;
                report[1] = 0x51;
                
                if (HidD_SetOutputReport(g_hid_device, report, sizeof(report)))
                {
                    printf("[SENT] Trigger sent to Pico\n");
                    fflush(stdout);
                }
                else
                {
                    printf("[ERROR] HID write failed\n");
                    fflush(stdout);
                }
            }
        }
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
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

int main()
{
    printf("=== Pico Macro Controller (USB HID) ===\n");
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
    printf("[INFO] Press Q key on keyboard to trigger macro\n");
    printf("[INFO] Close this window to exit\n");
    fflush(stdout);

    HMODULE module = GetModuleHandle(NULL);
    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, module, 0);
    if (!g_hook)
    {
        printf("[ERROR] Could not create keyboard hook\n");
        fflush(stdout);
        CloseHandle(g_hid_device);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(g_hook);
    CloseHandle(g_hid_device);
    printf("[EXIT] Closed.\n");
    return 0;
}

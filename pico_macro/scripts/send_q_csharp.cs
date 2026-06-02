using System;
using System.Runtime.InteropServices;

class Program
{
    // Pico USB IDs
    private const int PICO_VID = 0x2E8A;  // Raspberry Pi
    private const int PICO_PID = 0x0005;  // RP2040 Generic HID

    // Windows HID API
    private const int WH_KEYBOARD_LL = 13;
    private const int WM_KEYDOWN = 0x0100;
    private static LowLevelKeyboardProc _proc = HookCallback;
    private static IntPtr _hookID = IntPtr.Zero;
    private static IntPtr _hidDevice = IntPtr.Zero;

    private delegate IntPtr LowLevelKeyboardProc(int nCode, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern IntPtr SetWindowsHookEx(int idHook, LowLevelKeyboardProc lpfn, IntPtr hMod, uint dwThreadId);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool UnhookWindowsHookEx(IntPtr hhk);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern IntPtr CallNextHookEx(IntPtr hhk, int nCode, IntPtr wParam, IntPtr lParam);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr GetModuleHandle(string lpModuleName);

    [DllImport("user32.dll")]
    private static extern bool GetMessage(out MSG lpMsg, IntPtr hWnd, uint wMsgFilterMin, uint wMsgFilterMax);

    // HID API
    [DllImport("hid.dll")]
    private static extern void HidD_GetHidGuid(out Guid hidGuid);

    [DllImport("hid.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool HidD_GetAttributes(IntPtr deviceObject, ref HIDD_ATTRIBUTES attributes);

    [DllImport("hid.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool HidD_SetOutputReport(IntPtr HidDeviceObject, byte[] ReportBuffer, int ReportBufferLength);

    [DllImport("setupapi.dll", SetLastError = true)]
    private static extern IntPtr SetupDiGetClassDevs(ref Guid ClassGuid, IntPtr Enumerator, IntPtr hwndParent, int Flags);

    [DllImport("setupapi.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetupDiDestroyDeviceInfoList(IntPtr DeviceInfoSet);

    [DllImport("setupapi.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetupDiEnumDeviceInterfaces(IntPtr DeviceInfoSet, IntPtr DeviceInfoData, ref Guid InterfaceClassGuid, int MemberIndex, ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData);

    [DllImport("setupapi.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr DeviceInfoSet, ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData, IntPtr DeviceInterfaceDetailData, int DeviceInterfaceDetailDataSize, out int RequiredSize, IntPtr DeviceInfoData);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr CreateFileA(string filename, int access, int share, IntPtr securityAttributes, int creationDisposition, int flagsAndAttributes, IntPtr templateFile);

    [DllImport("kernel32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool CloseHandle(IntPtr hObject);

    [StructLayout(LayoutKind.Sequential)]
    private struct HIDD_ATTRIBUTES
    {
        public int Size;
        public ushort VendorID;
        public ushort ProductID;
        public ushort VersionNumber;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct SP_DEVICE_INTERFACE_DATA
    {
        public int cbSize;
        public Guid InterfaceClassGuid;
        public int Flags;
        public IntPtr Reserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct POINT { public int x; public int y; }

    [StructLayout(LayoutKind.Sequential)]
    private struct MSG { public IntPtr hwnd; public uint message; public UIntPtr wParam; public IntPtr lParam; public uint time; public POINT pt; }

    private const int DIGCF_PRESENT = 0x00000002;
    private const int DIGCF_DEVICEINTERFACE = 0x00000010;
    private const int GENERIC_READ = 0x80000000;
    private const int GENERIC_WRITE = 0x40000000;
    private const int FILE_SHARE_READ = 0x00000001;
    private const int FILE_SHARE_WRITE = 0x00000002;
    private const int OPEN_EXISTING = 0x00000003;
    private const int FILE_ATTRIBUTE_NORMAL = 0x00000080;

    static void Main()
    {
        Console.WriteLine("=== Pico Macro Controller (USB HID) ===");
        Console.WriteLine("Đang tìm Pico device...");

        if (!FindAndOpenPicoDevice())
        {
            Console.WriteLine("Lỗi: Không tìm được Pico HID device");
            return;
        }

        Console.WriteLine("Đã kết nối Pico thành công (HID)");
        Console.WriteLine("Nhấn phím Q trên bàn phím để trigger macro. Đóng cửa sổ để thoát.");

        _hookID = SetHook(_proc);
        MSG msg;
        while (GetMessage(out msg, IntPtr.Zero, 0, 0)) { }

        UnhookWindowsHookEx(_hookID);
        CloseHandle(_hidDevice);
        Console.WriteLine("Đã thoát.");
    }

    private static bool FindAndOpenPicoDevice()
    {
        Guid hid_guid;
        HidD_GetHidGuid(out hid_guid);

        IntPtr deviceInfo = SetupDiGetClassDevs(ref hid_guid, IntPtr.Zero, IntPtr.Zero, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (deviceInfo == IntPtr.Zero)
            return false;

        SP_DEVICE_INTERFACE_DATA deviceInterface = new SP_DEVICE_INTERFACE_DATA();
        deviceInterface.cbSize = Marshal.SizeOf(deviceInterface);

        for (int i = 0; SetupDiEnumDeviceInterfaces(deviceInfo, IntPtr.Zero, ref hid_guid, i, ref deviceInterface); i++)
        {
            int requiredSize = 0;
            SetupDiGetDeviceInterfaceDetail(deviceInfo, ref deviceInterface, IntPtr.Zero, 0, out requiredSize, IntPtr.Zero);

            IntPtr deviceDetailPtr = Marshal.AllocHGlobal(requiredSize);
            Marshal.WriteInt32(deviceDetailPtr, 5);  // cbSize

            if (SetupDiGetDeviceInterfaceDetail(deviceInfo, ref deviceInterface, deviceDetailPtr, requiredSize, out requiredSize, IntPtr.Zero))
            {
                string devicePath = Marshal.PtrToStringAnsi(new IntPtr(deviceDetailPtr.ToInt64() + 4));

                IntPtr device = CreateFileA(devicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, IntPtr.Zero, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, IntPtr.Zero);

                if (device != IntPtr.Zero)
                {
                    HIDD_ATTRIBUTES attributes = new HIDD_ATTRIBUTES();
                    attributes.Size = Marshal.SizeOf(attributes);
                    if (HidD_GetAttributes(device, ref attributes))
                    {
                        Console.WriteLine($"Found HID Device - VID: 0x{attributes.VendorID:X04} PID: 0x{attributes.ProductID:X04}");

                        if (attributes.VendorID == PICO_VID)
                        {
                            Console.WriteLine("Tìm thấy Pico device!");
                            _hidDevice = device;
                            Marshal.FreeHGlobal(deviceDetailPtr);
                            SetupDiDestroyDeviceInfoList(deviceInfo);
                            return true;
                        }
                    }
                    CloseHandle(device);
                }
            }
            Marshal.FreeHGlobal(deviceDetailPtr);
        }

        SetupDiDestroyDeviceInfoList(deviceInfo);
        return false;
    }

    private static IntPtr SetHook(LowLevelKeyboardProc proc)
    {
        using (var curProcess = System.Diagnostics.Process.GetCurrentProcess())
        using (var curModule = curProcess.MainModule)
        {
            return SetWindowsHookEx(WH_KEYBOARD_LL, proc, GetModuleHandle(curModule.ModuleName), 0);
        }
    }

    private static IntPtr HookCallback(int nCode, IntPtr wParam, IntPtr lParam)
    {
        if (nCode >= 0 && wParam == (IntPtr)WM_KEYDOWN)
        {
            int vkCode = Marshal.ReadInt32(lParam);
            if (vkCode == 0x51) // 'Q'
            {
                try
                {
                    byte[] report = new byte[9];
                    report[0] = 0x00;  // Report ID
                    report[1] = 0x51;  // Command: trigger macro

                    if (_hidDevice != IntPtr.Zero && HidD_SetOutputReport(_hidDevice, report, report.Length))
                    {
                        Console.WriteLine("Gửi trigger tới Pico (HID)");
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Lỗi HID: {ex.Message}");
                }
            }
        }
        return CallNextHookEx(_hookID, nCode, wParam, lParam);
    }
}


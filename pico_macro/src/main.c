#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"

#define LEFT_BTN  0x01
#define RIGHT_BTN 0x02
#define CMD_MACRO_TRIGGER 0x51  // 'Q'

//--------------------------------------------------------------------+
// USB HID Descriptors
//--------------------------------------------------------------------+

// HID Report Descriptor for Keyboard
uint8_t const desc_hid_report_keyboard[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// HID Report Descriptor for Mouse
uint8_t const desc_hid_report_mouse[] = {
    TUD_HID_REPORT_DESC_MOUSE()
};

// HID Report Descriptor for Vendor (custom commands)
uint8_t const desc_hid_report_vendor[] = {
    0x06, 0x00, 0xFF,        // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,              // Usage (Vendor Usage 1)
    0xA1, 0x01,              // Collection (Application)
    0x09, 0x02,              //   Usage (Vendor Usage 2)
    0x15, 0x00,              //   Logical Minimum (0)
    0x26, 0xFF, 0x00,        //   Logical Maximum (255)
    0x75, 0x08,              //   Report Size (8 bits)
    0x95, 0x08,              //   Report Count (8 bytes)
    0x81, 0x02,              //   Input (Data, Var, Abs)
    0x09, 0x03,              //   Usage (Vendor Usage 3)
    0x91, 0x02,              //   Output (Data, Var, Abs)
    0xC0                     // End Collection
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance)
{
    if (instance == 0) {
        return desc_hid_report_keyboard;
    } else if (instance == 1) {
        return desc_hid_report_mouse;
    } else {
        return desc_hid_report_vendor;
    }
}

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x2E8A,  // Raspberry Pi
    .idProduct          = 0x0005,  // Pico SDK
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN * 3)

#define EPNUM_HID_KEYBOARD  0x81
#define EPNUM_HID_MOUSE     0x82
#define EPNUM_HID_VENDOR    0x83

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 3, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface 0: Keyboard
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report_keyboard), EPNUM_HID_KEYBOARD, CFG_TUD_HID_EP_BUFSIZE, 1),
    
    // Interface 1: Mouse
    TUD_HID_DESCRIPTOR(1, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(desc_hid_report_mouse), EPNUM_HID_MOUSE, CFG_TUD_HID_EP_BUFSIZE, 1),
    
    // Interface 2: Vendor (for receiving triggers)
    TUD_HID_DESCRIPTOR(2, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_vendor), EPNUM_HID_VENDOR, CFG_TUD_HID_EP_BUFSIZE, 1)
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
    "Raspberry Pi",                 // 1: Manufacturer
    "Pico Macro Device",           // 2: Product
    "123456",                       // 3: Serials, should use chip ID
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;

        const char* str = string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        // Convert ASCII string into UTF-16
        for(uint8_t i=0; i<chr_count; i++) {
            _desc_str[1+i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

    return _desc_str;
}

//--------------------------------------------------------------------+
// HID Callbacks
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // Interface 2 is vendor HID for receiving triggers
    if (instance == 2 && bufsize > 0) {
        if (buffer[0] == CMD_MACRO_TRIGGER && !combo_in_progress) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            combo_start(combo_full, sizeof(combo_full) / sizeof(combo_full[0]), now);
        }
    }
}

//--------------------------------------------------------------------+
// Macro Combo Logic
//--------------------------------------------------------------------+

typedef enum {
    COMBO_ACTION_NONE = 0,
    COMBO_ACTION_KEY_PRESS,
    COMBO_ACTION_KEY_RELEASE,
    COMBO_ACTION_MOUSE_DOWN,
    COMBO_ACTION_MOUSE_UP,
} combo_action_t;

typedef struct {
    combo_action_t action;
    uint8_t keycode;
    uint8_t buttons;
    uint32_t delay_ms;
} combo_step_t;

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

static const combo_step_t *active_combo = NULL;
static size_t active_combo_length = 0;
static size_t active_combo_index = 0;
static uint32_t combo_deadline_ms = 0;
static bool combo_in_progress = false;

static inline void send_key_press(uint8_t keycode) {
    uint8_t keys[6] = {0};
    keys[0] = keycode;
    tud_hid_keyboard_report(1, 0, keys);  // Report ID = 1 for keyboard
}

static inline void send_key_release(void) {
    uint8_t keys[6] = {0};
    tud_hid_keyboard_report(1, 0, keys);  // Report ID = 1 for keyboard
}

static inline void send_mouse(uint8_t buttons) {
    tud_hid_mouse_report(2, buttons, 0, 0, 0, 0);  // Report ID = 2 for mouse
}

static inline void combo_reset(void) {
    active_combo = NULL;
    active_combo_length = 0;
    active_combo_index = 0;
    combo_deadline_ms = 0;
    combo_in_progress = false;
}

static void combo_start(const combo_step_t *combo, size_t length, uint32_t now) {
    active_combo = combo;
    active_combo_length = length;
    active_combo_index = 0;
    combo_in_progress = true;
    combo_deadline_ms = 0;

    // execute first step immediately
    // combo_advance() will set the next deadline
    
    while (combo_in_progress && now >= combo_deadline_ms) {
        const combo_step_t *step = &active_combo[active_combo_index];
        switch (step->action) {
            case COMBO_ACTION_KEY_PRESS:
                send_key_press(step->keycode);
                break;
            case COMBO_ACTION_KEY_RELEASE:
                send_key_release();
                break;
            case COMBO_ACTION_MOUSE_DOWN:
                send_mouse(step->buttons);
                break;
            case COMBO_ACTION_MOUSE_UP:
                send_mouse(0);
                break;
        }

        uint32_t delay = step->delay_ms;
        active_combo_index++;
        if (active_combo_index >= active_combo_length) {
            combo_reset();
            return;
        }

        combo_deadline_ms = now + delay;
        if (delay == 0) {
            continue;
        }
        break;
    }
}

static void combo_advance(uint32_t now) {
    if (!combo_in_progress) {
        return;
    }

    while (combo_in_progress && now >= combo_deadline_ms) {
        const combo_step_t *step = &active_combo[active_combo_index];
        switch (step->action) {
            case COMBO_ACTION_KEY_PRESS:
                send_key_press(step->keycode);
                break;
            case COMBO_ACTION_KEY_RELEASE:
                send_key_release();
                break;
            case COMBO_ACTION_MOUSE_DOWN:
                send_mouse(step->buttons);
                break;
            case COMBO_ACTION_MOUSE_UP:
                send_mouse(0);
                break;
        }

        uint32_t delay = step->delay_ms;
        active_combo_index++;
        if (active_combo_index >= active_combo_length) {
            combo_reset();
            return;
        }

        combo_deadline_ms = now + delay;
        if (delay == 0) {
            continue;
        }
        break;
    }
}

// USB HID callback - triggered when host sends data
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize)
{
    // This is now defined above with the other callbacks
}

// USB HID get report callback
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen)
{
    // This is now defined above with the other callbacks
    return 0;
}

int main() {
    tusb_init();

    while (1) {
        tud_task();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        if (tud_mounted()) {
            combo_advance(now);
        }
    }

    return 0;
}

#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"

#define LEFT_BTN  0x01
#define RIGHT_BTN 0x02
#define CMD_MACRO_TRIGGER 0x51  // 'Q'

// Report IDs (1 interface, multiple report IDs - giống example chính thức)
enum {
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_MOUSE    = 2,
    REPORT_ID_VENDOR   = 3,
};

//--------------------------------------------------------------------+
// USB HID Descriptors (1 interface, composite report)
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] = {
    // Keyboard
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    // Mouse
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
    // Vendor (for receiving trigger commands)
    HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),
    HID_USAGE(0x01),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
        HID_REPORT_ID(REPORT_ID_VENDOR)
        HID_USAGE(0x02),
        HID_LOGICAL_MIN(0x00),
        HID_LOGICAL_MAX_N(0xFF, 2),
        HID_REPORT_SIZE(8),
        HID_REPORT_COUNT(8),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
        HID_USAGE(0x03),
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_COLLECTION_END
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return desc_hid_report;
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
    .idVendor           = 0xCAFE,
    .idProduct          = 0x4006,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor (1 HID interface)
//--------------------------------------------------------------------+

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID  0x81

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    "TinyUSB",
    "Pico Macro Device",
    "123456",
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;
    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for(uint8_t i=0; i<chr_count; i++) {
            _desc_str[1+i] = str[i];
        }
    }
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*chr_count + 2);
    return _desc_str;
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

static const combo_step_t *active_combo = NULL;
static size_t active_combo_length = 0;
static size_t active_combo_index = 0;
static uint32_t combo_deadline_ms = 0;
static bool combo_in_progress = false;

static const combo_step_t combo_full[] = {
    {COMBO_ACTION_MOUSE_DOWN, 0, RIGHT_BTN, 10},
    {COMBO_ACTION_MOUSE_UP,   0, 0,         90},
    {COMBO_ACTION_MOUSE_DOWN, 0, LEFT_BTN,  10},
    {COMBO_ACTION_MOUSE_UP,   0, 0,          5},
    {COMBO_ACTION_KEY_PRESS,  HID_KEY_8, 0,  8},
    {COMBO_ACTION_KEY_RELEASE,0, 0,          5},
    {COMBO_ACTION_MOUSE_DOWN, 0, RIGHT_BTN, 10},
    {COMBO_ACTION_MOUSE_UP,   0, 0,          0},
};

static inline void send_key_press(uint8_t keycode) {
    if (!tud_hid_ready()) return;
    uint8_t keys[6] = {0};
    keys[0] = keycode;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keys);
}

static inline void send_key_release(void) {
    if (!tud_hid_ready()) return;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
}

static inline void send_mouse(uint8_t buttons) {
    if (!tud_hid_ready()) return;
    tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, 0, 0, 0, 0);
}

static inline void combo_reset(void) {
    active_combo = NULL;
    active_combo_length = 0;
    active_combo_index = 0;
    combo_deadline_ms = 0;
    combo_in_progress = false;
}

static void combo_execute_step(const combo_step_t *step) {
    switch (step->action) {
        case COMBO_ACTION_KEY_PRESS:   send_key_press(step->keycode); break;
        case COMBO_ACTION_KEY_RELEASE: send_key_release(); break;
        case COMBO_ACTION_MOUSE_DOWN:  send_mouse(step->buttons); break;
        case COMBO_ACTION_MOUSE_UP:    send_mouse(0); break;
        default: break;
    }
}

static void combo_start(const combo_step_t *combo, size_t length, uint32_t now) {
    active_combo = combo;
    active_combo_length = length;
    active_combo_index = 0;
    combo_in_progress = true;
    combo_deadline_ms = 0;

    while (combo_in_progress && now >= combo_deadline_ms) {
        const combo_step_t *step = &active_combo[active_combo_index];
        combo_execute_step(step);

        uint32_t delay = step->delay_ms;
        active_combo_index++;
        if (active_combo_index >= active_combo_length) {
            combo_reset();
            return;
        }
        combo_deadline_ms = now + delay;
        if (delay == 0) continue;
        break;
    }
}

static void combo_advance(uint32_t now) {
    if (!combo_in_progress) return;

    while (combo_in_progress && now >= combo_deadline_ms) {
        const combo_step_t *step = &active_combo[active_combo_index];
        combo_execute_step(step);

        uint32_t delay = step->delay_ms;
        active_combo_index++;
        if (active_combo_index >= active_combo_length) {
            combo_reset();
            return;
        }
        combo_deadline_ms = now + delay;
        if (delay == 0) continue;
        break;
    }
}

//--------------------------------------------------------------------+
// HID Callbacks
//--------------------------------------------------------------------+

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
    hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type;
    (void) buffer; (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
    hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance; (void) report_type;
    // Nhận trigger qua Report ID 3 (Vendor)
    if (report_id == REPORT_ID_VENDOR && bufsize > 0) {
        if (buffer[0] == CMD_MACRO_TRIGGER && !combo_in_progress) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            combo_start(combo_full, sizeof(combo_full) / sizeof(combo_full[0]), now);
        }
    }
}

//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

int main() {
    tusb_init();

    while (1) {
        tud_task();
        if (tud_mounted()) {
            combo_advance(to_ms_since_boot(get_absolute_time()));
        }
    }

    return 0;
}

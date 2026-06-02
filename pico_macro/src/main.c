#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"

#define LEFT_BTN  0x01
#define RIGHT_BTN 0x02
#define CMD_MACRO_TRIGGER 0x51  // 'Q'

// HID Report Descriptor for custom commands
const uint8_t desc_hid_report[] = {
    0x06, 0x00, 0xFF,        // Usage Page (Vendor Defined)
    0x09, 0x01,              // Usage (Vendor Usage 1)
    0xA1, 0x01,              // Collection (Application)
    0x09, 0x02,              // Usage (Vendor Usage 2)
    0x15, 0x00,              // Logical Minimum (0)
    0x26, 0xFF, 0x00,        // Logical Maximum (255)
    0x75, 0x08,              // Report Size (8)
    0x95, 0x08,              // Report Count (8)
    0x81, 0x00,              // Input (Data, Array, Absolute)
    0x09, 0x03,              // Usage (Vendor Usage 3)
    0x91, 0x00,              // Output (Data, Array, Absolute)
    0xC0                     // End Collection
};

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
    tud_hid_keyboard_report(0, 0, keys);
}

static inline void send_key_release(void) {
    uint8_t keys[6] = {0};
    tud_hid_keyboard_report(0, 0, keys);
}

static inline void send_mouse(uint8_t buttons) {
    tud_hid_mouse_report(0, 0, 0, 0, buttons);
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
    if (report_type == HID_REPORT_TYPE_OUTPUT && bufsize > 0) {
        if (buffer[0] == CMD_MACRO_TRIGGER && !combo_in_progress) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            combo_start(combo_full, sizeof(combo_full) / sizeof(combo_full[0]), now);
        }
    }
}

// USB HID get report callback
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen)
{
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

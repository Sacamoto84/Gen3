#include "scope_input.h"
#include "scope_config.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

void buttons_init() {
    gpio_init(PIN_BTN_TIME_DOWN);
    gpio_set_dir(PIN_BTN_TIME_DOWN, GPIO_IN);
    gpio_pull_up(PIN_BTN_TIME_DOWN);

    gpio_init(PIN_BTN_TIME_UP);
    gpio_set_dir(PIN_BTN_TIME_UP, GPIO_IN);
    gpio_pull_up(PIN_BTN_TIME_UP);
}

bool check_button(uint pin, bool &prev, uint32_t &press_time, uint32_t &last_repeat) {
    bool raw = gpio_get(pin); // 0 = pressed (active LOW)
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool triggered = false;

    // Contact debounce lockout (50 ms) on falling edge
    if (!raw && prev && (now - last_repeat > 50)) {
        triggered = true;
        press_time = now;
        last_repeat = now;
    } else if (!raw && !prev) {
        // Held down: repeat after 350ms every 100ms
        if ((now - press_time) > 350 && (now - last_repeat) > 100) {
            triggered = true;
            last_repeat = now;
        }
    }
    prev = raw;
    return triggered;
}

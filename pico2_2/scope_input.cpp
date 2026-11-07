#include "scope_input.h"
#include "scope_config.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

static volatile int g_encoder_delta = 0;
static volatile bool g_encoder_activity = false;
static uint8_t g_last_enc_state = 0x03;
static int8_t g_enc_accum = 0;

// Quadrature Gray-code transition lookup table
static const int8_t enc_table[16] = {
     0,  // 00 -> 00: no change
    -1,  // 00 -> 01: CCW
     1,  // 00 -> 10: CW
     0,  // 00 -> 11: invalid
     1,  // 01 -> 00: CW
     0,  // 01 -> 01: no change
     0,  // 01 -> 10: invalid
    -1,  // 01 -> 11: CCW
    -1,  // 10 -> 00: CCW
     0,  // 10 -> 01: invalid
     0,  // 10 -> 10: no change
     1,  // 10 -> 11: CW
     0,  // 11 -> 00: invalid
     1,  // 11 -> 01: CW
    -1,  // 11 -> 10: CCW
     0   // 11 -> 11: no change
};

static void encoder_gpio_irq_callback(uint gpio, uint32_t events) {
    uint8_t a = gpio_get(PIN_ENC_A) ? 1 : 0;
    uint8_t b = gpio_get(PIN_ENC_B) ? 1 : 0;
    uint8_t curr = (a << 1) | b;

    int8_t diff = enc_table[(g_last_enc_state << 2) | curr];
    g_last_enc_state = curr;
    g_enc_accum += diff;

    if (g_enc_accum >= 4) {
        g_encoder_delta++;
        g_enc_accum -= 4;
        g_encoder_activity = true;
    } else if (g_enc_accum <= -4) {
        g_encoder_delta--;
        g_enc_accum += 4;
        g_encoder_activity = true;
    } else if (curr == 0x03 && g_enc_accum != 0) {
        if (g_enc_accum >= 2) {
            g_encoder_delta++;
            g_encoder_activity = true;
        } else if (g_enc_accum <= -2) {
            g_encoder_delta--;
            g_encoder_activity = true;
        }
        g_enc_accum = 0;
    }
}

void encoder_init() {
    // 1. Encoder Channel A
    gpio_init(PIN_ENC_A);
    gpio_set_dir(PIN_ENC_A, GPIO_IN);
    gpio_pull_up(PIN_ENC_A);

    // 2. Encoder Channel B
    gpio_init(PIN_ENC_B);
    gpio_set_dir(PIN_ENC_B, GPIO_IN);
    gpio_pull_up(PIN_ENC_B);

    // 3. Encoder Button (SW)
    gpio_init(PIN_ENC_SW);
    gpio_set_dir(PIN_ENC_SW, GPIO_IN);
    gpio_pull_up(PIN_ENC_SW);

    // Read initial state
    uint8_t a = gpio_get(PIN_ENC_A) ? 1 : 0;
    uint8_t b = gpio_get(PIN_ENC_B) ? 1 : 0;
    g_last_enc_state = (a << 1) | b;
    g_enc_accum = 0;
    g_encoder_delta = 0;
    g_encoder_activity = false;

    // Attach GPIO edge interrupts for both A and B channels
    gpio_set_irq_enabled_with_callback(PIN_ENC_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &encoder_gpio_irq_callback);
    gpio_set_irq_enabled(PIN_ENC_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}

int encoder_get_delta() {
    uint32_t status = save_and_disable_interrupts();
    int delta = g_encoder_delta;
    g_encoder_delta = 0;
    restore_interrupts(status);
    return delta;
}

bool encoder_button_pressed() {
    static bool btn_prev = true;
    static uint32_t btn_last_press = 0;

    bool raw = gpio_get(PIN_ENC_SW); // 0 = pressed (active LOW)
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool pressed = false;

    // 50ms debounce lockout
    if (!raw && btn_prev && (now - btn_last_press > 50)) {
        pressed = true;
        btn_last_press = now;
        g_encoder_activity = true;
    }
    btn_prev = raw;
    return pressed;
}

bool encoder_has_activity() {
    bool act = g_encoder_activity || !gpio_get(PIN_ENC_SW);
    g_encoder_activity = false;
    return act;
}

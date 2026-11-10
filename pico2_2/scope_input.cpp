#include "scope_input.h"
#include "scope_config.h"
#include "hardware/pio.h"
#include "quadrature_encoder.pio.h"
#include <stdlib.h>

// GyverLibs uButton instance
uButton enc_btn;

// Hardware PIO instance for Quadrature Encoder
static PIO encoder_pio = pio0;
static uint encoder_sm = 0;
static int32_t last_pio_count = 0;
static bool pio_primed = false;
static bool last_btn_level = true; // High when unpressed (pullup)

void encoder_init() {
    // 1. Initialize AlexGyver uButton on PIN_ENC_SW (GP20)
    enc_btn.init(PIN_ENC_SW);

    // 2. Load and initialize PIO Quadrature Encoder program on pio0, SM 0
    // PIN_ENC_B is GP21, PIN_ENC_A is GP22 (consecutive pins 21 and 22!)
    // max_step_rate = 5000 Hz provides hardware digital low-pass filtering
    // rejecting contact bounce pulses shorter than 200 us (identical to STM32 digital filter)
    pio_add_program_at_offset(encoder_pio, &quadrature_encoder_program, 0);
    quadrature_encoder_program_init(encoder_pio, encoder_sm, PIN_ENC_B, 5000);

    // Read initial hardware count and button level
    last_pio_count = quadrature_encoder_get_count(encoder_pio, encoder_sm);
    pio_primed = true;
    last_btn_level = gpio_get(PIN_ENC_SW);
}

void encoder_btn_tick() {
    last_btn_level = gpio_get(PIN_ENC_SW);
    enc_btn.tick();
}

int encoder_get_delta() {
    int32_t current_count = quadrature_encoder_get_count(encoder_pio, encoder_sm);
    if (!pio_primed) {
        last_pio_count = current_count;
        pio_primed = true;
        return 0;
    }

    int32_t diff = current_count - last_pio_count;

    // EC11 mechanical encoder produces 4 quadrature edges per physical detent click
    int steps = diff / 4;
    if (steps != 0) {
        last_pio_count += steps * 4;
    }

    return steps;
}

bool encoder_has_activity() {
    // 1. Check for NEW button press edge (falling edge: HIGH -> LOW)
    // When the button is continuously held down, it is NOT new activity,
    // so ongoing DMA captures are allowed to complete and render normally!
    bool cur_btn = gpio_get(PIN_ENC_SW);
    if (last_btn_level && !cur_btn) {
        last_btn_level = cur_btn;
        return true; // Abort DMA on initial press edge so main() can process press instantly
    }
    last_btn_level = cur_btn;

    // 2. Check if hardware PIO counter has moved by at least one full detent click
    // EC11 mechanical encoder produces 4 quadrature edges per physical detent click.
    int32_t current_count = quadrature_encoder_get_count(encoder_pio, encoder_sm);
    int32_t diff = current_count - last_pio_count;
    if ((diff / 4) != 0) {
        return true;
    }

    return false;
}

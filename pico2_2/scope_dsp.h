#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "scope_config.h"

struct TriggerResult {
    int trigger_index;
    bool triggered;
    float frequency_hz;
    float vpp_volts;
    float vmid_volts;
    bool is_envelope; // true if locked to modulation envelope!
};

// Convert ADC 8-bit raw reading (0..255, centered at 128 = 1.65V) to screen Y
static inline float adc_to_y(float adc_val) {
    // 128 is AC midpoint (1.65V). 55.0 pixels for 128 counts (+10% amplitude gain relative to midpoint)
    float y = (float)SCOPE_Y_MID - (adc_val - 128.0f) * (55.0f / 128.0f);
    if (y < (float)SCOPE_Y_MIN + 1.0f) y = (float)SCOPE_Y_MIN + 1.0f;
    if (y > (float)SCOPE_Y_MAX - 1.0f) y = (float)SCOPE_Y_MAX - 1.0f;
    return y;
}

// Convert ADC raw reading to vertical bin coordinate (0.0f .. 117.0f)
static inline float adc_to_bin(float adc_val) {
    float y = adc_to_y(adc_val);
    float b = y - (float)SCOPE_Y_MIN;
    if (b < 0.0f) b = 0.0f;
    if (b > (float)(SCOPE_Y_BINS - 1)) b = (float)(SCOPE_Y_BINS - 1);
    return b;
}

TriggerResult find_trigger_centered(const uint8_t* samples, int count, int pre_trigger_samples, float sample_rate_hz, int manual_trig_level = -1);

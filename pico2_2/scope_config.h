#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware/spi.h"
#include "TFT.h"
#include "TFT_color.h"

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================
// Display SPI Pins (ST7789 135x240 / 240x135)
#define PIN_SCK   2   // GP2: SPI0 SCK
#define PIN_MOSI  3   // GP3: SPI0 TX / MOSI
#define PIN_RES   4   // GP4: Display Reset
#define PIN_DC    5   // GP5: Display Data/Command
#define PIN_CS    6   // GP6: Display Chip Select
#define PIN_BL    7   // GP7: Display Backlight PWM

// Oscilloscope ADC Input
#define PIN_ADC   26  // GP26 (ADC0, Pin 31 on Pico): 0..3.3V, DC bias ~1.65V
#define ADC_CHANNEL 0

// Control Buttons (Active LOW with internal pull-up, connect button to GND)
#define PIN_BTN_TIME_DOWN 14 // GP14 (Pin 19): Timebase - (Zoom IN, faster sweep)
#define PIN_BTN_TIME_UP   15 // GP15 (Pin 20): Timebase + (Zoom OUT, slower sweep)

// Display orientation: 90 or 270 for landscape (240 width x 135 height)
#define DISPLAY_ROTATION  90

// Display Dimensions in Landscape
#define LCD_W 135
#define LCD_H 240
#define SCOPE_W 240
#define SCOPE_H 135

// Viewport boundaries
#define HUD_H        17   // Top HUD status bar height (Y: 0..16)
#define SCOPE_Y_MIN  17   // First row of oscilloscope area
#define SCOPE_Y_MAX  134  // Last row of oscilloscope area
#define SCOPE_X_MID  120  // Horizontal Center (Trigger synchronization point!)
#define SCOPE_Y_MID  76   // Vertical Center horizontal line
#define SCOPE_Y_BINS (SCOPE_Y_MAX - SCOPE_Y_MIN + 1) // 118 vertical bins

// Visual trace styling (High contrast DSO style: crisp yellow on dark grid)
#define COLOR_TRACE      COLOR_YELLOW // Channel 1 trace color (Bright Yellow 0xFFE0)
#define COLOR_DARKGRID   0x10A2       // Subtle dark slate grid dots
#define COLOR_AXIS       0x2965       // Medium slate center crosshairs & border
#define TRACE_BOLD       1            // 1 = 2-pixel bold for high visibility, 0 = 1-pixel
#define DENSITY_GAIN     22.0f        // Gain for phosphor tone mapping

// Continuous ADC DMA Buffer (131,072 samples = 128 KB in 8-bit mode)
#define ADC_MAX_SAMPLES 131072

// ============================================================================
// TIMEBASE TYPES & TABLES
// ============================================================================
enum ScopeMode {
    MODE_SWEEP, // Fast sweeps (10us..25ms): Center Triggered Sync + Peak-Detect (4..60 FPS)
    MODE_ROLL   // Slow sweeps (>= 50ms/div): Continuous 60 FPS Gapless Strip-Chart Live Roll
};

struct TimebaseConfig {
    const char* label;      // Timebase label
    float us_per_div;       // Microseconds per division (10 divisions across screen)
    float sample_rate_hz;   // ADC sample rate (Hz)
    float clkdiv;           // ADC clock divisor register value
    ScopeMode mode;         // Sweep vs Roll mode
};

#define TIMEBASE_COUNT 15
extern const TimebaseConfig TIMEBASES[TIMEBASE_COUNT];

// Global Shared Buffers
extern uint16_t LCD_Buffer16[LCD_W * LCD_H];
extern uint8_t adc_raw_buffer[ADC_MAX_SAMPLES];
extern float density_grid[SCOPE_W][SCOPE_Y_BINS];
extern uint16_t grid_template[SCOPE_Y_BINS][SCOPE_W];

// Display driver instances
extern TFT_LCD_t LCD_0;
extern TFT tft;

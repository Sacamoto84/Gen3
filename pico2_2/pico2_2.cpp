#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "timber.h"
#include "TFT.h"
#include "TFT_color.h"
#include "FontClassic/FontClassic.h"

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

// Visual trace styling (High contrast DSO style: crisp yellow on dark grid)
#define COLOR_TRACE      COLOR_YELLOW // Channel 1 trace color (Bright Yellow 0xFFE0)
#define COLOR_DARKGRID   0x10A2       // Subtle dark slate grid dots (0x10A2)
#define COLOR_AXIS       0x2965       // Medium slate center crosshairs & border (0x2965)
#define TRACE_BOLD       1            // 1 = 2-pixel bold for high visibility, 0 = 1-pixel

// Frame buffer (240 * 135 * 2 bytes = 64,800 bytes)
uint16_t LCD_Buffer16[LCD_W * LCD_H];

// Continuous ADC DMA Buffer (65,536 samples = 128 KB)
#define ADC_MAX_SAMPLES 65536
uint16_t adc_raw_buffer[ADC_MAX_SAMPLES];
int adc_dma_chan = -1;
dma_channel_config adc_dma_cfg;

// ST7789 configuration structure
TFT_LCD_t LCD_0 = {
    .TFT_WIDTH = LCD_W,
    .TFT_HEIGHT = LCD_H,
    .LCD_Driver = ST7789,
    .Bit = 16,
    .spi = spi0,
    .baudrate = 75000000,
    .notUse2 = NULL,
    .I2C_Adress = 0,
    .buffer16 = &LCD_Buffer16[0],
    .buffer8 = NULL,
    .palete = NULL,
    .dx = 52,
    .dy = 40,
    .pin_cs = PIN_CS,
    .pin_dc = PIN_DC,
    .pin_reset = PIN_RES,
    .pin_sck = PIN_SCK,
    .pin_mosi = PIN_MOSI,
    .pin_bl = PIN_BL,
    .MADCTL = { 0, 0, 0, 0, 0, 0 },
    .ROTATE_DELTA = {
        LCD_W, LCD_H, 52, 40,   // 0 deg
        LCD_H, LCD_W, 40, 52,   // 90 deg
        LCD_W, LCD_H, 52, 40,   // 180 deg
        LCD_H, LCD_W, 40, 52    // 270 deg
    }
};

TFT tft;

// ============================================================================
// TIMEBASE PRESETS TABLE
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

static const TimebaseConfig TIMEBASES[] = {
    // 1. Fast Sweeps: Triggered Center Sync (Cycle Detail, 4 to 60 FPS)
    { "10us",     10.0f,     500000.0f,    0.0f, MODE_SWEEP }, // 0:  100us window (60 FPS)
    { "25us",     25.0f,     500000.0f,    0.0f, MODE_SWEEP }, // 1:  250us window (60 FPS)
    { "50us",     50.0f,     500000.0f,    0.0f, MODE_SWEEP }, // 2:  500us window (60 FPS)
    { "100us",   100.0f,     500000.0f,    0.0f, MODE_SWEEP }, // 3:  1.0ms window (60 FPS)
    { "250us",   250.0f,     500000.0f,    0.0f, MODE_SWEEP }, // 4:  2.5ms window (60 FPS)
    { "500us",   500.0f,     500000.0f,    0.0f, MODE_SWEEP }, // 5:  5.0ms window (60 FPS)
    { "1ms",    1000.0f,     250000.0f,  191.0f, MODE_SWEEP }, // 6:   10ms window (60 FPS)
    { "2.5ms",  2500.0f,     250000.0f,  191.0f, MODE_SWEEP }, // 7:   25ms window (35 FPS)
    { "5ms",    5000.0f,     250000.0f,  191.0f, MODE_SWEEP }, // 8:   50ms window (20 FPS)
    { "10ms",  10000.0f,     250000.0f,  191.0f, MODE_SWEEP }, // 9:  100ms window (10 FPS)
    { "25ms",  25000.0f,     250000.0f,  191.0f, MODE_SWEEP }, // 10: 250ms window (4 FPS)

    // 2. Slow Sweeps: 60 FPS Continuous Gapless Peak-Detect Live Roll (AM/FM Modulation & Envelope)
    // Roll mode activates starting from 50ms/div (50ms, 100ms, 200ms, 500ms)
    { "50ms",  50000.0f,     250000.0f,  191.0f, MODE_ROLL  }, // 11: 500ms window (60 FPS live roll)
    { "100ms",100000.0f,     250000.0f,  191.0f, MODE_ROLL  }, // 12: 1.0s window  (60 FPS live roll)
    { "200ms",200000.0f,     250000.0f,  191.0f, MODE_ROLL  }, // 13: 2.0s window  (60 FPS live roll)
    { "500ms",500000.0f,     250000.0f,  191.0f, MODE_ROLL  }  // 14: 5.0s window  (48 FPS live roll)
};
#define TIMEBASE_COUNT (sizeof(TIMEBASES) / sizeof(TIMEBASES[0]))
static int current_tb_idx = 2; // Default to 50us/div (5 cycles of 10kHz)

// ============================================================================
// DRAWING & RETICLE FUNCTIONS (CRISP DIGITAL SCOPE LOOK)
// ============================================================================
static inline void draw_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < SCOPE_W && y >= SCOPE_Y_MIN && y <= SCOPE_Y_MAX) {
        LCD_Buffer16[y * SCOPE_W + x] = color;
#if TRACE_BOLD
        if (y + 1 <= SCOPE_Y_MAX) {
            LCD_Buffer16[(y + 1) * SCOPE_W + x] = color;
        }
#endif
    }
}

// Crisp line drawing (Bresenham)
void draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

// Convert ADC raw reading (0..4095, centered at 2048 = 1.65V) to screen Y
static inline float adc_to_y(float adc_val) {
    // 2048 is midpoint (1.65V). 50 pixels for 2048 counts (100 px peak-to-peak for 3.3V)
    float y = (float)SCOPE_Y_MID - (adc_val - 2048.0f) * (50.0f / 2048.0f);
    if (y < (float)SCOPE_Y_MIN + 1.0f) y = (float)SCOPE_Y_MIN + 1.0f;
    if (y > (float)SCOPE_Y_MAX - 1.0f) y = (float)SCOPE_Y_MAX - 1.0f;
    return y;
}

// Draw the top center trigger marker (indicates X=120 trigger point)
void draw_trigger_marker() {
    // Small inverted yellow triangle at X=120, Y=17..19
    LCD_Buffer16[SCOPE_Y_MIN * SCOPE_W + 118] = COLOR_YELLOW;
    LCD_Buffer16[SCOPE_Y_MIN * SCOPE_W + 119] = COLOR_YELLOW;
    LCD_Buffer16[SCOPE_Y_MIN * SCOPE_W + 120] = COLOR_YELLOW;
    LCD_Buffer16[SCOPE_Y_MIN * SCOPE_W + 121] = COLOR_YELLOW;
    LCD_Buffer16[SCOPE_Y_MIN * SCOPE_W + 122] = COLOR_YELLOW;

    LCD_Buffer16[(SCOPE_Y_MIN + 1) * SCOPE_W + 119] = COLOR_YELLOW;
    LCD_Buffer16[(SCOPE_Y_MIN + 1) * SCOPE_W + 120] = COLOR_YELLOW;
    LCD_Buffer16[(SCOPE_Y_MIN + 1) * SCOPE_W + 121] = COLOR_YELLOW;

    LCD_Buffer16[(SCOPE_Y_MIN + 2) * SCOPE_W + 120] = COLOR_YELLOW;
}

// Draw classic DSO reticle grid
void draw_grid() {
    // 1. Clear scope viewport to pure black
    for (int y = SCOPE_Y_MIN; y <= SCOPE_Y_MAX; y++) {
        memset(&LCD_Buffer16[y * SCOPE_W], 0, SCOPE_W * sizeof(uint16_t));
    }

    // 2. Horizontal divider below HUD
    for (int x = 0; x < SCOPE_W; x++) {
        LCD_Buffer16[(SCOPE_Y_MIN - 1) * SCOPE_W + x] = COLOR_AXIS;
    }

    // 3. Grid dots:
    // Vertical divisions: every 24 pixels (10 div across 240px)
    // Horizontal divisions: every 25 pixels around center Y=76 (at 26, 51, 76, 101, 126)
    for (int gx = 0; gx < SCOPE_W; gx += 24) {
        for (int y = SCOPE_Y_MIN; y <= SCOPE_Y_MAX; y += 4) {
            LCD_Buffer16[y * SCOPE_W + gx] = COLOR_DARKGRID;
        }
    }
    for (int gy = 26; gy <= 126; gy += 25) {
        for (int x = 0; x < SCOPE_W; x += 4) {
            LCD_Buffer16[gy * SCOPE_W + x] = COLOR_DARKGRID;
        }
    }

    // 4. Center crosshairs:
    // Horizontal center line with tick marks
    for (int x = 0; x < SCOPE_W; x++) {
        if ((x % 4) == 0 || (x % 24) == 0) {
            LCD_Buffer16[SCOPE_Y_MID * SCOPE_W + x] = COLOR_AXIS;
        }
    }
    // Vertical center line (X = 120) with tick marks
    for (int y = SCOPE_Y_MIN; y <= SCOPE_Y_MAX; y++) {
        if (((y - SCOPE_Y_MIN) % 4) == 0 || ((y - SCOPE_Y_MID) % 25) == 0) {
            LCD_Buffer16[y * SCOPE_W + SCOPE_X_MID] = COLOR_AXIS;
        }
    }

    // 5. Trigger marker at top center (X=120)
    draw_trigger_marker();
}

// Single-pixel background grid lookup (used in roll mode to restore background behind envelope)
static inline uint16_t get_grid_pixel(int x, int y) {
    if (y == SCOPE_Y_MID && ((x % 4) == 0 || (x % 24) == 0)) return COLOR_AXIS;
    if (x == SCOPE_X_MID && (((y - SCOPE_Y_MIN) % 4) == 0 || ((y - SCOPE_Y_MID) % 25) == 0)) return COLOR_AXIS;
    if ((x % 24 == 0) && (y % 4 == 0)) return COLOR_DARKGRID;
    if (((y - SCOPE_Y_MID) % 25 == 0) && (x % 4 == 0)) return COLOR_DARKGRID;
    return COLOR_BLACK;
}

// ============================================================================
// DENSITY ACCUMULATION GRID FOR ROLL MODE (ANDROID PHOSPHOR MODEL)
// ============================================================================
#define SCOPE_Y_BINS (SCOPE_Y_MAX - SCOPE_Y_MIN + 1) // 118 vertical bins
#define DENSITY_GAIN 22.0f

static float density_grid[SCOPE_W][SCOPE_Y_BINS];

// Precomputed background reticle grid for fast 1-cycle lookup during scrolling
static uint16_t grid_template[SCOPE_Y_BINS][SCOPE_W];
static int screen_to_grid[SCOPE_W];

void init_grid_template() {
    for (int b = 0; b < SCOPE_Y_BINS; b++) {
        int y = SCOPE_Y_MIN + b;
        for (int x = 0; x < SCOPE_W; x++) {
            grid_template[b][x] = get_grid_pixel(x, y);
        }
    }
}

// Convert ADC raw reading to vertical bin coordinate (0.0f .. 117.0f)
static inline float adc_to_bin(float adc_val) {
    float y = adc_to_y(adc_val);
    float b = y - (float)SCOPE_Y_MIN;
    if (b < 0.0f) b = 0.0f;
    if (b > (float)(SCOPE_Y_BINS - 1)) b = (float)(SCOPE_Y_BINS - 1);
    return b;
}

// Android-style exponential tone mapping with alpha blending over DSO reticle grid
static inline uint16_t tone_map_density(int col, int y, float density) {
    uint16_t bg = get_grid_pixel(col, y);
    if (density <= 1e-5f) {
        return bg;
    }

    // Exponential tone mapping: matches Android PhosphorGrid: L = 1.0 - exp(-energy * gain)
    float L = 1.0f - expf(-density * DENSITY_GAIN);
    if (L > 1.0f) L = 1.0f;

    // Background color components (RGB565)
    uint32_t bg_r = (bg >> 11) & 0x1F;
    uint32_t bg_g = (bg >> 5) & 0x3F;
    uint32_t bg_b = bg & 0x1F;

    // Channel 1 Yellow trace: R=31, G=63, B=0
    // Alpha-blend tone-mapped trace over background reticle grid
    uint32_t r = (uint32_t)((float)bg_r * (1.0f - L) + 31.0f * L + 0.5f);
    uint32_t g = (uint32_t)((float)bg_g * (1.0f - L) + 63.0f * L + 0.5f);
    uint32_t b = (uint32_t)((float)bg_b * (1.0f - L) + 0.5f);

    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;

    return (uint16_t)((r << 11) | (g << 5) | b);
}

// ============================================================================
// ADC & DMA CAPTURE (WITH NON-BLOCKING ABORT ON BUTTON PRESS)
// ============================================================================
void adc_capture_init() {
    adc_init();
    adc_gpio_init(PIN_ADC);
    adc_select_input(ADC_CHANNEL);

    adc_fifo_setup(
        true,    // Write conversions to FIFO
        true,    // Enable DMA request
        1,       // DREQ asserted when >= 1 sample in FIFO
        false,   // No error bit
        false    // Full 16-bit word
    );

    adc_set_clkdiv(0);

    adc_dma_chan = dma_claim_unused_channel(true);
    adc_dma_cfg = dma_channel_get_default_config(adc_dma_chan);
    channel_config_set_transfer_data_size(&adc_dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&adc_dma_cfg, false);
    channel_config_set_write_increment(&adc_dma_cfg, true);
    channel_config_set_dreq(&adc_dma_cfg, DREQ_ADC);
}

// Captures `count` samples via DMA. Polls buttons so captures abort instantly on button press!
bool adc_capture_dma_poll(uint16_t* dest, uint32_t count, float clkdiv) {
    if (count > ADC_MAX_SAMPLES) count = ADC_MAX_SAMPLES;
    adc_run(false);
    adc_fifo_drain();
    adc_set_clkdiv(clkdiv);

    dma_channel_configure(
        adc_dma_chan,
        &adc_dma_cfg,
        dest,           // Destination buffer
        &adc_hw->fifo,  // Source: ADC FIFO
        count,          // Number of samples
        true            // Start immediately
    );

    adc_run(true);

    while (dma_channel_is_busy(adc_dma_chan)) {
        // If button pressed during capture, abort immediately!
        if (!gpio_get(PIN_BTN_TIME_DOWN) || !gpio_get(PIN_BTN_TIME_UP)) {
            dma_channel_abort(adc_dma_chan);
            adc_run(false);
            adc_fifo_drain();
            return false; // Aborted
        }
        tight_loop_contents();
    }

    adc_run(false);
    adc_fifo_drain();
    return true; // Finished
}

// Asynchronous non-blocking DMA start for double-buffered continuous streaming
void adc_capture_dma_start(uint16_t* dest, uint32_t count, float clkdiv) {
    if (count > (ADC_MAX_SAMPLES / 2)) count = ADC_MAX_SAMPLES / 2;
    adc_fifo_drain();
    adc_set_clkdiv(clkdiv);

    dma_channel_configure(
        adc_dma_chan,
        &adc_dma_cfg,
        dest,           // Destination buffer
        &adc_hw->fifo,  // Source: ADC FIFO
        count,          // Number of samples
        true            // Start immediately
    );

    adc_run(true);
}

// Wait for active DMA capture to complete, polling buttons for instant abort
bool adc_capture_dma_wait() {
    while (dma_channel_is_busy(adc_dma_chan)) {
        if (!gpio_get(PIN_BTN_TIME_DOWN) || !gpio_get(PIN_BTN_TIME_UP)) {
            dma_channel_abort(adc_dma_chan);
            adc_run(false);
            adc_fifo_drain();
            return false; // Aborted by button press
        }
        tight_loop_contents();
    }
    adc_run(false);
    adc_fifo_drain();
    return true;
}

// ============================================================================
// TRIGGER SYNCHRONIZATION ALIGNED TO SCREEN CENTER (X = 120)
// ============================================================================
struct TriggerResult {
    int trigger_index;
    bool triggered;
    float frequency_hz;
    float vpp_volts;
    float vmid_volts;
};

TriggerResult find_trigger_centered(const uint16_t* samples, int count, int pre_trigger_samples, float sample_rate_hz) {
    TriggerResult res = { 0, false, 0.0f, 0.0f, 0.0f };
    if (count < pre_trigger_samples + 100) return res;

    // 1. Calculate min and max across buffer
    uint16_t v_min = samples[0];
    uint16_t v_max = samples[0];
    for (int i = 1; i < count; i++) {
        if (samples[i] < v_min) v_min = samples[i];
        if (samples[i] > v_max) v_max = samples[i];
    }

    uint16_t swing = (v_max - v_min) / 2;
    res.vpp_volts = (float)(v_max - v_min) * (3.30f / 4096.0f);
    res.vmid_volts = (float)((v_max + v_min) / 2) * (3.30f / 4096.0f);

    // If signal swing is too small (< 20 counts ~ 16 mV), flat/noise -> Free-run auto
    if (swing < 20) {
        res.trigger_index = pre_trigger_samples;
        res.triggered = false;
        res.frequency_hz = 0.0f;
        return res;
    }

    // Midpoint and arm level with 25% hysteresis
    int level = (v_min + v_max) / 2;
    int arm_level = level - (int)(swing * 0.25f);

    // Pre-trigger margin: search starts after pre_trigger_samples so X=0..119 has valid history!
    int start_search = pre_trigger_samples;
    int end_search = count - pre_trigger_samples - 10;
    if (end_search <= start_search) {
        end_search = count - 10;
    }

    bool armed = false;
    int first_edge = -1;
    int second_edge = -1;
    float frac1 = 0.0f;
    float frac2 = 0.0f;

    // Scan for hysteresis arming starting a bit before start_search
    int arm_start = (start_search > 40) ? (start_search - 40) : 0;
    for (int i = arm_start; i < end_search; i++) {
        int val = samples[i];
        if (val < arm_level) {
            armed = true;
        } else if (armed && val >= level) {
            if (i >= start_search) {
                // Rising edge detected with valid pre-trigger!
                int prev = samples[i - 1];
                float frac = (float)(level - prev) / (float)(val - prev + 1e-5f);

                if (first_edge < 0) {
                    first_edge = i;
                    frac1 = frac;
                    armed = false; // Re-arm to find second edge for frequency measurement
                } else {
                    second_edge = i;
                    frac2 = frac;
                    break; // Period found!
                }
            }
        }
    }

    if (first_edge >= 0) {
        res.trigger_index = first_edge;
        res.triggered = true;

        if (second_edge > first_edge) {
            float period_samples = (float)(second_edge - first_edge) + (frac2 - frac1);
            if (period_samples > 1.0f) {
                res.frequency_hz = sample_rate_hz / period_samples;
            }
        }
    } else {
        // Fallback: Auto trigger centered
        res.trigger_index = pre_trigger_samples;
        res.triggered = false;
        res.frequency_hz = 0.0f;
    }

    return res;
}

// ============================================================================
// BUTTONS WITH DEBOUNCE & AUTO-REPEAT
// ============================================================================
uint32_t btn_down_press_time = 0;
uint32_t btn_up_press_time = 0;
uint32_t btn_down_last_repeat = 0;
uint32_t btn_up_last_repeat = 0;
bool btn_down_prev = true;
bool btn_up_prev = true;

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

    if (!raw && prev) {
        // Falling edge: button just pressed
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

// ============================================================================
// ON-SCREEN HUD STATUS BAR
// ============================================================================
void render_hud(const char* tb_label, bool trig, bool is_roll, float freq_hz, float vpp) {
    // Clear top HUD background area
    tft.RectangleFilled(0, 0, SCOPE_W - 1, HUD_H - 2, COLOR_BLACK);

    // 1. Timebase label
    char buf[32];
    snprintf(buf, sizeof(buf), "%s/d", tb_label);
    tft.SetFontColor(COLOR_YELLOW, COLOR_BLACK);
    tft.uTFT.CurrentX = 4;
    tft.uTFT.CurrentY = 3;
    FontClassicPuts(&tft, buf, &Font_7x10, 0);

    // 2. Mode badge: [TRIG], [AUTO], [ROLL]
    if (is_roll) {
        tft.SetFontColor(COLOR_CYAN, COLOR_BLACK);
        snprintf(buf, sizeof(buf), "[ROLL]");
    } else if (trig) {
        tft.SetFontColor(COLOR_GREEN, COLOR_BLACK);
        snprintf(buf, sizeof(buf), "[TRIG]");
    } else {
        tft.SetFontColor(COLOR_RED, COLOR_BLACK);
        snprintf(buf, sizeof(buf), "[AUTO]");
    }
    tft.uTFT.CurrentX = 58;
    tft.uTFT.CurrentY = 3;
    FontClassicPuts(&tft, buf, &Font_7x10, 0);

    // 3. Measured Frequency
    if (freq_hz >= 1000.0f) {
        snprintf(buf, sizeof(buf), "%5.2fkHz", freq_hz / 1000.0f);
    } else if (freq_hz > 0.0f) {
        snprintf(buf, sizeof(buf), "%4.0fHz", freq_hz);
    } else {
        snprintf(buf, sizeof(buf), "----Hz");
    }
    tft.SetFontColor(COLOR_CYAN, COLOR_BLACK);
    tft.uTFT.CurrentX = 114;
    tft.uTFT.CurrentY = 3;
    FontClassicPuts(&tft, buf, &Font_7x10, 0);

    // 4. Measured Peak-to-Peak Voltage
    snprintf(buf, sizeof(buf), "%4.2fVpp", vpp);
    tft.SetFontColor(COLOR_WHITE, COLOR_BLACK);
    tft.uTFT.CurrentX = 176;
    tft.uTFT.CurrentY = 3;
    FontClassicPuts(&tft, buf, &Font_7x10, 0);
}

// ============================================================================
// CORE 1: DEDICATED DISPLAY & GPU ENGINE (RUNS AT 300 MHz IN PARALLEL)
// ============================================================================
enum CoreCommand {
    CMD_IDLE = 0,
    CMD_RENDER_ROLL,
    CMD_RENDER_SWEEP,
    CMD_DRAW_GRID
};

static volatile int g_render_roll_head = SCOPE_W - 1;
static volatile float g_render_vpp = 0.0f;
static volatile float g_render_freq = 0.0f;
static volatile bool g_render_trig = false;
static volatile int g_render_tb_idx = 2;

void core1_entry() {
    timber.info("Core 1 online: Dedicated Hardware Display Engine active at 300 MHz!");

    while (true) {
        // Wait for render dispatch from Core 0 via hardware SIO FIFO (1-cycle latency)
        uint32_t cmd = multicore_fifo_pop_blocking();

        if (cmd == CMD_RENDER_ROLL) {
            int head = g_render_roll_head;
            float vpp = g_render_vpp;
            int tb_idx = g_render_tb_idx;

            // 1. Precompute column mapping for strip-chart
            int cur_col = (head + 1) % SCOPE_W;
            for (int X = 0; X < SCOPE_W; X++) {
                screen_to_grid[X] = cur_col;
                cur_col++;
                if (cur_col >= SCOPE_W) cur_col = 0;
            }

            // 2. Render scrolling strip-chart over stationary DSO reticle grid (sequential burst write)
            for (int b = 0; b < SCOPE_Y_BINS; b++) {
                int Y = SCOPE_Y_MIN + b;
                uint16_t* row_ptr = &LCD_Buffer16[Y * SCOPE_W];
                const uint16_t* bg_row = &grid_template[b][0];

                for (int X = 0; X < SCOPE_W; X++) {
                    int g_col = screen_to_grid[X];
                    float d = density_grid[g_col][b];

                    if (d <= 1e-5f) {
                        row_ptr[X] = bg_row[X];
                    } else {
                        // Exponential tone mapping: L = 1.0 - exp(-energy * gain)
                        float L = 1.0f - expf(-d * DENSITY_GAIN);
                        if (L > 1.0f) L = 1.0f;

                        uint16_t bg = bg_row[X];
                        uint32_t bg_r = (bg >> 11) & 0x1F;
                        uint32_t bg_g = (bg >> 5) & 0x3F;
                        uint32_t bg_b = bg & 0x1F;

                        // Alpha-blend trace over background reticle grid
                        uint32_t r = (uint32_t)((float)bg_r * (1.0f - L) + 31.0f * L + 0.5f);
                        uint32_t g = (uint32_t)((float)bg_g * (1.0f - L) + 63.0f * L + 0.5f);
                        uint32_t b_col = (uint32_t)((float)bg_b * (1.0f - L) + 0.5f);

                        if (r > 31) r = 31;
                        if (g > 63) g = 63;
                        if (b_col > 31) b_col = 31;

                        row_ptr[X] = (uint16_t)((r << 11) | (g << 5) | b_col);
                    }
                }
            }

            // 3. Render HUD & Trigger Marker
            render_hud(TIMEBASES[tb_idx].label, false, true, 0.0f, vpp);
            draw_trigger_marker();

            // 4. Refresh display via hardware DMA over 75 MHz SPI (6.9 ms)
            tft.driver.ST7789_UpdateDMA16bitV3();

            // 5. Notify Core 0 that frame is displayed
            multicore_fifo_push_blocking(1);
        }
        else if (cmd == CMD_DRAW_GRID) {
            draw_grid();
            tft.driver.ST7789_UpdateDMA16bitV3();
            multicore_fifo_push_blocking(1);
        }
        else if (cmd == CMD_RENDER_SWEEP) {
            int tb_idx = g_render_tb_idx;
            render_hud(TIMEBASES[tb_idx].label, g_render_trig, false, g_render_freq, g_render_vpp);
            draw_trigger_marker();
            tft.driver.ST7789_UpdateDMA16bitV3();
            multicore_fifo_push_blocking(1);
        }
    }
}

// ============================================================================
// MAIN OSCILLOSCOPE LOGIC
// ============================================================================
int main() {
    // 1. Elevate core voltage for overclocking
    vreg_set_voltage(VREG_VOLTAGE_1_25);
    sleep_ms(10);

    // 2. Set CPU clock to 300 MHz
    set_sys_clock_khz(300000, true);

    // 3. Connect peripheral clock (clk_peri: SPI, UART) to clk_sys (300 MHz)
    clock_configure_undivided(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, clock_get_hz(clk_sys));

    // 4. Initialize STDIO & USB terminal
    stdio_init_all();
    sleep_ms(1000);

    uint32_t freq_mhz = clock_get_hz(clk_sys) / 1000000;
    uint32_t peri_mhz = clock_get_hz(clk_peri) / 1000000;

    timber.init();
    timber.clear();
    timber.info("==========================================");
    timber.info(" RP2350 DDS Oscilloscope Initializing...  ");
    timber.info(" CPU Clock: %lu MHz | clk_peri: %lu MHz   ", freq_mhz, peri_mhz);
    timber.info("==========================================");

    // Turn on backlight immediately
    gpio_init(PIN_BL);
    gpio_set_dir(PIN_BL, GPIO_OUT);
    gpio_put(PIN_BL, 1);

    timber.print("Initializing ST7789 display (240x135)... ");
    tft.init(&LCD_0);
    tft.driver.ST77XX_Rotate(DISPLAY_ROTATION);
    tft.driver.ST77XX_Update_MADCTL();
    tft.initBacklight(PIN_BL, 90);
    draw_grid();
    init_grid_template();
    timber.successful("OK");

    timber.print("Initializing ADC on GP%d & DMA (64K Buffer)... ", PIN_ADC);
    adc_capture_init();
    buttons_init();
    timber.successful("OK");

    timber.print("Launching Core 1 (Dedicated Display/GPU Engine)... ");
    multicore_launch_core1(core1_entry);
    timber.successful("OK");

    timber.info("Oscilloscope ready! Buttons on GP%d (Down) and GP%d (Up)", PIN_BTN_TIME_DOWN, PIN_BTN_TIME_UP);

    float last_measured_freq = 0.0f;
    float last_measured_vpp = 0.0f;

    int roll_head = SCOPE_W - 1;
    uint16_t last_roll_adc = 2048;
    memset(density_grid, 0, sizeof(density_grid));
    uint16_t* roll_buf_curr = &adc_raw_buffer[0];
    uint16_t* roll_buf_next = &adc_raw_buffer[ADC_MAX_SAMPLES / 2];
    bool roll_primed = false;

    while (true) {
        // 1. Check Button Inputs
        if (check_button(PIN_BTN_TIME_DOWN, btn_down_prev, btn_down_press_time, btn_down_last_repeat)) {
            if (current_tb_idx > 0) {
                current_tb_idx--;
                multicore_fifo_push_blocking(CMD_DRAW_GRID);
                multicore_fifo_pop_blocking();
                roll_head = SCOPE_W - 1;
                memset(density_grid, 0, sizeof(density_grid));
                last_roll_adc = 2048;
                roll_primed = false;
                timber.info("Timebase changed to: %s/div", TIMEBASES[current_tb_idx].label);
            }
        }
        if (check_button(PIN_BTN_TIME_UP, btn_up_prev, btn_up_press_time, btn_up_last_repeat)) {
            if (current_tb_idx + 1 < (int)TIMEBASE_COUNT) {
                current_tb_idx++;
                multicore_fifo_push_blocking(CMD_DRAW_GRID);
                multicore_fifo_pop_blocking();
                roll_head = SCOPE_W - 1;
                memset(density_grid, 0, sizeof(density_grid));
                last_roll_adc = 2048;
                roll_primed = false;
                timber.info("Timebase changed to: %s/div", TIMEBASES[current_tb_idx].label);
            }
        }

        const TimebaseConfig &tb = TIMEBASES[current_tb_idx];

        if (tb.mode == MODE_SWEEP) {
            // ================================================================
            // SWEEP MODE: Triggered Center Sync + Fast Peak-Detect (10..60 FPS)
            // ================================================================
            float total_window_us = tb.us_per_div * 10.0f;
            int window_samples = (int)(total_window_us * (tb.sample_rate_hz / 1000000.0f));
            if (window_samples < 24) window_samples = 24;

            float step_samples = (float)window_samples / (float)SCOPE_W;

            // Half-window for 120 pixels of pre-trigger:
            int pre_trigger_samples = (int)ceilf((float)SCOPE_X_MID * step_samples);

            // Search reserve for trigger edge (up to 2500 samples, fitted to available DMA buffer):
            int search_samples = ADC_MAX_SAMPLES - (2 * pre_trigger_samples) - 36;
            if (search_samples > 2500) search_samples = 2500;
            if (search_samples < 500) search_samples = 500;
            int capture_count = pre_trigger_samples + search_samples + pre_trigger_samples;
            if (capture_count > ADC_MAX_SAMPLES) capture_count = ADC_MAX_SAMPLES;
            if (capture_count < 800) capture_count = 800;

            // Capture contiguous samples via high-speed ADC DMA (aborts instantly if button pressed)
            bool finished = adc_capture_dma_poll(adc_raw_buffer, capture_count, tb.clkdiv);
            if (!finished) {
                continue; // Button pressed, switch immediately!
            }

            // Find rising edge with hysteresis, centered at X=120
            TriggerResult trig = find_trigger_centered(adc_raw_buffer, capture_count, pre_trigger_samples, tb.sample_rate_hz);
            if (trig.triggered) {
                last_measured_freq = trig.frequency_hz;
                last_measured_vpp = trig.vpp_volts;
            } else {
                last_measured_vpp = trig.vpp_volts;
            }

            // Draw clean DSO reticle grid
            draw_grid();

            // Render trace: Trigger is centered at col = 120!
            float start_sample_pos = (float)trig.trigger_index - (float)SCOPE_X_MID * step_samples;
            int prev_x = 0;
            int prev_y_top = (int)roundf(adc_to_y(adc_raw_buffer[(int)fmaxf(0.0f, start_sample_pos)]));
            int prev_y_bot = prev_y_top;

            for (int col = 0; col < SCOPE_W; col++) {
                float pos = start_sample_pos + (float)col * step_samples;

                if (step_samples <= 1.0f) {
                    // Zoomed in: Sub-sample linear interpolation
                    int s_idx = (int)floorf(pos);
                    int cur_y = prev_y_top;
                    if (s_idx >= 0 && s_idx + 1 < capture_count) {
                        float frac = pos - (float)s_idx;
                        float val = (float)adc_raw_buffer[s_idx] * (1.0f - frac) + (float)adc_raw_buffer[s_idx + 1] * frac;
                        cur_y = (int)roundf(adc_to_y(val));
                    }
                    if (col > 0) {
                        draw_line(prev_x, prev_y_top, col, cur_y, COLOR_TRACE);
                    } else {
                        draw_pixel(col, cur_y, COLOR_TRACE);
                    }
                    prev_y_top = cur_y;
                    prev_y_bot = cur_y;
                } else {
                    // Zoomed out (Peak-Detect envelope): Find exact min and max across all samples in this column
                    int s_start = (int)pos;
                    int s_end = (int)(pos + step_samples);
                    if (s_start < 0) s_start = 0;
                    if (s_end >= capture_count) s_end = capture_count - 1;

                    uint16_t c_min = adc_raw_buffer[s_start];
                    uint16_t c_max = adc_raw_buffer[s_start];
                    for (int s = s_start + 1; s <= s_end; s++) {
                        if (adc_raw_buffer[s] < c_min) c_min = adc_raw_buffer[s];
                        if (adc_raw_buffer[s] > c_max) c_max = adc_raw_buffer[s];
                    }

                    int y_top = (int)roundf(adc_to_y((float)c_max));
                    int y_bot = (int)roundf(adc_to_y((float)c_min));
                    if (y_top > y_bot) { int t = y_top; y_top = y_bot; y_bot = t; }

                    // Draw solid column envelope
                    for (int y = y_top; y <= y_bot; y++) {
                        draw_pixel(col, y, COLOR_TRACE);
                    }

                    // Seamless connection with previous column to eliminate gaps
                    if (col > 0) {
                        int connect_top_min = (y_top < prev_y_top) ? y_top : prev_y_top;
                        int connect_top_max = (y_top > prev_y_top) ? y_top : prev_y_top;
                        for (int y = connect_top_min; y <= connect_top_max; y++) {
                            draw_pixel(col, y, COLOR_TRACE);
                        }

                        int connect_bot_min = (y_bot < prev_y_bot) ? y_bot : prev_y_bot;
                        int connect_bot_max = (y_bot > prev_y_bot) ? y_bot : prev_y_bot;
                        for (int y = connect_bot_min; y <= connect_bot_max; y++) {
                            draw_pixel(col, y, COLOR_TRACE);
                        }
                    }

                    prev_y_top = y_top;
                    prev_y_bot = y_bot;
                }

                prev_x = col;
            }

            // Delegate HUD rendering and SPI display transmission to Core 1!
            g_render_tb_idx = current_tb_idx;
            g_render_trig = trig.triggered;
            g_render_freq = last_measured_freq;
            g_render_vpp = last_measured_vpp;
            multicore_fifo_push_blocking(CMD_RENDER_SWEEP);
            multicore_fifo_pop_blocking();

            // Pacing delay on ultra-fast sweeps
            if (tb.us_per_div <= 500.0f) {
                sleep_ms(16);
            }

        } else {
            // ================================================================
            // ROLL MODE: Continuous Strip-Chart Slider (Scrolls Right-to-Left)
            // ================================================================
            float total_screen_us = tb.us_per_div * 10.0f;
            float col_duration_us = total_screen_us / (float)SCOPE_W;

            // Target frame interval ~16.6 ms (60 FPS)
            // Number of columns advancing in ~16.6 ms:
            int num_cols = (int)roundf(16666.0f / col_duration_us);
            if (num_cols < 1) num_cols = 1;
            if (num_cols > 24) num_cols = 24;

            // Exact duration for these columns:
            float capture_time_us = (float)num_cols * col_duration_us;

            // Total samples at 250 kSPS (0.25 samples per microsecond):
            int total_samples = (int)(capture_time_us * 0.25f);
            if (total_samples < 64) total_samples = 64;
            if (total_samples > (ADC_MAX_SAMPLES / 2)) total_samples = (ADC_MAX_SAMPLES / 2);

            // Double-buffering: prime current buffer on first frame
            if (!roll_primed) {
                adc_capture_dma_start(roll_buf_curr, total_samples, tb.clkdiv);
                bool ok = adc_capture_dma_wait();
                if (!ok) {
                    continue; // Aborted by button press
                }
                roll_primed = true;
            }

            // Immediately launch next capture into roll_buf_next in background hardware DMA!
            // This runs in parallel while CPU processes and SPI DMA transmits the current frame!
            adc_capture_dma_start(roll_buf_next, total_samples, tb.clkdiv);

            // Track min and max across all samples in this frame for instant Vpp measurement
            uint16_t f_min = roll_buf_curr[0];
            uint16_t f_max = roll_buf_curr[0];
            for (int s = 1; s < total_samples; s++) {
                if (roll_buf_curr[s] < f_min) f_min = roll_buf_curr[s];
                if (roll_buf_curr[s] > f_max) f_max = roll_buf_curr[s];
            }
            last_measured_vpp = (float)(f_max - f_min) * (3.30f / 4096.0f);

            // Distribute samples evenly across the num_cols columns:
            float samples_per_c = (float)total_samples / (float)num_cols;

            // Energy weight per sample step: normalized so column integral equals 1.0f
            // Invariant to sweep timebase setting!
            const float weight = 1.0f / samples_per_c;

            // Append new columns to ring buffer head
            for (int c = 0; c < num_cols; c++) {
                int s_start = (int)((float)c * samples_per_c);
                int s_end   = (int)((float)(c + 1) * samples_per_c);
                if (s_end > total_samples) s_end = total_samples;
                if (s_start >= s_end) s_start = s_end - 1;

                // Advance head (newest column)
                roll_head = (roll_head + 1) % SCOPE_W;

                // Clear newest column in density grid
                memset(density_grid[roll_head], 0, sizeof(density_grid[roll_head]));

                // Rasterize continuous dwell-time path into density_grid[roll_head]
                for (int s = s_start; s < s_end; s++) {
                    uint16_t prev_raw = (s == 0) ? last_roll_adc : roll_buf_curr[s - 1];
                    uint16_t cur_raw  = roll_buf_curr[s];

                    float b1 = adc_to_bin((float)prev_raw);
                    float b2 = adc_to_bin((float)cur_raw);

                    float delta_b = b2 - b1;
                    int steps = (int)ceilf(fabsf(delta_b));
                    if (steps < 1) steps = 1;

                    float share = weight / (float)steps;
                    float step_b = delta_b / (float)steps;

                    float cur_b = b1;
                    for (int st = 0; st < steps; st++) {
                        int low = (int)cur_b;
                        if (low < 0) low = 0;
                        if (low >= SCOPE_Y_BINS - 1) low = SCOPE_Y_BINS - 2;
                        float frac = cur_b - (float)low;
                        if (frac < 0.0f) frac = 0.0f;
                        if (frac > 1.0f) frac = 1.0f;

                        density_grid[roll_head][low]     += share * (1.0f - frac);
                        density_grid[roll_head][low + 1] += share * frac;

                        cur_b += step_b;
                    }
                }
            }

            last_roll_adc = roll_buf_curr[total_samples - 1];

            // Delegate tone-mapping, HUD rendering, and SPI display transmission to Core 1!
            g_render_roll_head = roll_head;
            g_render_vpp = last_measured_vpp;
            g_render_tb_idx = current_tb_idx;
            multicore_fifo_push_blocking(CMD_RENDER_ROLL);

            // Wait for background ADC DMA capture of next frame (16.6 ms)
            // (Meanwhile Core 1 renders and pushes SPI in parallel!)
            bool ok = adc_capture_dma_wait();
            if (!ok) {
                roll_primed = false;
                multicore_fifo_pop_blocking(); // Drain Core 1 completion before aborting
                continue; // Button pressed, switch immediately!
            }

            // Ensure Core 1 has finished frame transmission (already done ~9 ms ago)
            multicore_fifo_pop_blocking();

            // Ping-pong swap buffer pointers
            uint16_t* tmp = roll_buf_curr;
            roll_buf_curr = roll_buf_next;
            roll_buf_next = tmp;
        }
    }

    return 0;
}

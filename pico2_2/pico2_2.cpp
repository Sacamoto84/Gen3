#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "timber.h"
#include "scope_config.h"
#include "scope_input.h"
#include "scope_adc.h"
#include "scope_dsp.h"
#include "scope_render.h"

// ============================================================================
// GLOBAL SHARED BUFFERS & HARDWARE STRUCTURES
// ============================================================================
// Frame buffer (240 * 135 * 2 bytes = 64,800 bytes)
uint16_t LCD_Buffer16[LCD_W * LCD_H];

// Continuous ADC DMA Buffer (131,072 samples = 128 KB in 8-bit mode)
uint8_t adc_raw_buffer[ADC_MAX_SAMPLES];

// Roll Mode Phosphor Energy Grid (240 x 118 x 4 bytes = 113,280 bytes)
float density_grid[SCOPE_W][SCOPE_Y_BINS];

// Precomputed background reticle grid for fast 1-cycle lookup during scrolling
uint16_t grid_template[SCOPE_Y_BINS][SCOPE_W];

// ST7789 display configuration
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
const TimebaseConfig TIMEBASES[TIMEBASE_COUNT] = {
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
    { "10ms",  10000.0f,     250000.0f,  191.0f, MODE_SWEEP }, // 9:  100ms window (Envelope/Carrier Trigger)
    { "25ms",  25000.0f,     100000.0f,  479.0f, MODE_SWEEP }, // 10: 250ms window (Envelope/Carrier Trigger)

    // 2. Slow Sweeps: 60 FPS Continuous Gapless Peak-Detect Live Roll (AM/FM Modulation & Envelope)
    { "50ms",  50000.0f,     250000.0f,  191.0f, MODE_ROLL  }, // 11: 500ms window (60 FPS live roll)
    { "100ms",100000.0f,     250000.0f,  191.0f, MODE_ROLL  }, // 12: 1.0s window  (60 FPS live roll)
    { "200ms",200000.0f,     250000.0f,  191.0f, MODE_ROLL  }, // 13: 2.0s window  (60 FPS live roll)
    { "500ms",500000.0f,     250000.0f,  191.0f, MODE_ROLL  }  // 14: 5.0s window  (48 FPS live roll)
};

static int current_tb_idx = 2; // Default to 50us/div
static bool scope_stopped = false;

// ============================================================================
// MAIN APPLICATION ENTRY POINT
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

    timber.print("Initializing ADC on GP%d & DMA (128K Buffer)... ", PIN_ADC);
    adc_capture_init();
    timber.successful("OK");

    timber.print("Initializing Rotary Encoder (GP%d/A, GP%d/B, GP%d/SW)... ", PIN_ENC_A, PIN_ENC_B, PIN_ENC_SW);
    encoder_init();
    timber.successful("OK");

    timber.print("Launching Core 1 (Dedicated Display/GPU Engine)... ");
    multicore_launch_core1(core1_entry);
    timber.successful("OK");

    timber.info("Oscilloscope ready! Rotary Encoder on GP%d/GP%d, Button on GP%d (Run/Stop)", PIN_ENC_A, PIN_ENC_B, PIN_ENC_SW);

    float last_measured_freq = 0.0f;
    float last_measured_vpp = 0.0f;

    int roll_head = SCOPE_W - 1;
    uint8_t last_roll_adc = 128;
    memset(density_grid, 0, sizeof(density_grid));
    uint8_t* roll_buf_curr = &adc_raw_buffer[0];
    uint8_t* roll_buf_next = &adc_raw_buffer[ADC_MAX_SAMPLES / 2];
    bool roll_primed = false;

    uint32_t roll_abs_sample = 0;
    uint32_t roll_last_edge = 0;
    float roll_last_frac = 0.0f;
    bool roll_armed = false;
    uint32_t roll_last_trig_ms = 0;

    while (true) {
        // 1. Check Encoder Button (Toggle Run / Stop)
        if (encoder_button_pressed()) {
            scope_stopped = !scope_stopped;
            g_render_stopped = scope_stopped;
            timber.info("Oscilloscope: %s", scope_stopped ? "STOPPED [HOLD]" : "RUNNING [LIVE]");

            if (scope_stopped) {
                // Immediate refresh of HUD with [STOP] indicator
                g_render_tb_idx = current_tb_idx;
                g_render_freq = last_measured_freq;
                g_render_vpp = last_measured_vpp;
                multicore_fifo_push_blocking(CMD_RENDER_SWEEP);
                multicore_fifo_pop_blocking();
            }
        }

        // 2. Check Rotary Encoder Step (Change Timebase)
        int delta = encoder_get_delta();
        if (delta != 0) {
            int new_idx = current_tb_idx + delta;
            if (new_idx < 0) new_idx = 0;
            if (new_idx >= TIMEBASE_COUNT) new_idx = TIMEBASE_COUNT - 1;

            if (new_idx != current_tb_idx) {
                current_tb_idx = new_idx;
                multicore_fifo_push_blocking(CMD_DRAW_GRID);
                multicore_fifo_pop_blocking();
                roll_head = SCOPE_W - 1;
                memset(density_grid, 0, sizeof(density_grid));
                last_roll_adc = 128;
                roll_primed = false;
                roll_abs_sample = 0;
                roll_last_edge = 0;
                roll_last_frac = 0.0f;
                roll_armed = false;
                roll_last_trig_ms = 0;
                timber.info("Timebase changed to: %s/div", TIMEBASES[current_tb_idx].label);
            }
        }

        // If stopped, pause acquisition and loop gently
        if (scope_stopped) {
            sleep_ms(15);
            continue;
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

            // Search reserve for trigger edge
            int max_avail_search = ADC_MAX_SAMPLES - (2 * pre_trigger_samples) - 64;
            int search_samples = (int)(window_samples * 1.5f);
            if (search_samples < 1500) search_samples = 1500;
            if (search_samples > max_avail_search) search_samples = max_avail_search;
            if (search_samples < 500) search_samples = 500;
            int capture_count = pre_trigger_samples + search_samples + pre_trigger_samples;
            if (capture_count > ADC_MAX_SAMPLES) capture_count = ADC_MAX_SAMPLES;
            if (capture_count < 800) capture_count = 800;

            // Capture contiguous samples via high-speed ADC DMA (aborts instantly if encoder turned)
            bool finished = adc_capture_dma_poll(adc_raw_buffer, capture_count, tb.clkdiv);
            if (!finished) {
                continue; // Encoder turned, switch immediately!
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

                    uint8_t c_min = adc_raw_buffer[s_start];
                    uint8_t c_max = adc_raw_buffer[s_start];
                    for (int s = s_start + 1; s <= s_end; s++) {
                        if (adc_raw_buffer[s] < c_min) c_min = adc_raw_buffer[s];
                        if (adc_raw_buffer[s] > c_max) c_max = adc_raw_buffer[s];
                    }

                    int y_top = (int)roundf(adc_to_y((float)c_max));
                    int y_bot = (int)roundf(adc_to_y((float)c_min));
                    if (y_top > y_bot) { int t = y_top; y_top = y_bot; y_bot = t; }

                    // Draw shaded column envelope:
                    if (y_bot - y_top <= 1) {
                        draw_pixel(col, y_top, COLOR_TRACE);
                        if (y_bot > y_top) draw_pixel(col, y_bot, COLOR_TRACE);
                    } else {
                        draw_pixel(col, y_top, COLOR_TRACE);
                        draw_pixel(col, y_bot, COLOR_TRACE);
                        for (int y = y_top + 1; y < y_bot; y++) {
                            LCD_Buffer16[y * SCOPE_W + col] = 0x8400;
                        }
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

            // Delegate HUD rendering and SPI display transmission to Core 1
            g_render_tb_idx = current_tb_idx;
            g_render_trig = trig.triggered;
            g_render_is_env = trig.is_envelope;
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

            int num_cols = (int)roundf(16666.0f / col_duration_us);
            if (num_cols < 1) num_cols = 1;
            if (num_cols > 24) num_cols = 24;

            float capture_time_us = (float)num_cols * col_duration_us;

            // Calculate total_samples from tb.sample_rate_hz dynamically
            int total_samples = (int)(capture_time_us * (tb.sample_rate_hz / 1000000.0f));
            if (total_samples < 64) total_samples = 64;
            if (total_samples > (ADC_MAX_SAMPLES / 2)) total_samples = (ADC_MAX_SAMPLES / 2);

            // Double-buffering: prime current buffer on first frame
            if (!roll_primed) {
                adc_capture_dma_start(roll_buf_curr, total_samples, tb.clkdiv);
                bool ok = adc_capture_dma_wait();
                if (!ok) {
                    continue; // Aborted by encoder or button
                }
                roll_primed = true;
            }

            // Launch next capture into roll_buf_next in background hardware DMA
            adc_capture_dma_start(roll_buf_next, total_samples, tb.clkdiv);

            // Track min and max across all samples in this frame for instant Vpp measurement
            uint8_t f_min = roll_buf_curr[0];
            uint8_t f_max = roll_buf_curr[0];
            for (int s = 1; s < total_samples; s++) {
                if (roll_buf_curr[s] < f_min) f_min = roll_buf_curr[s];
                if (roll_buf_curr[s] > f_max) f_max = roll_buf_curr[s];
            }
            last_measured_vpp = (float)(f_max - f_min) * (3.30f / 256.0f);

            // Frequency and trigger detection across continuous roll stream
            uint32_t now_ms = to_ms_since_boot(get_absolute_time());
            uint16_t swing = f_max - f_min;

            if (swing >= 2) {
                int mid_level = (int)f_min + (int)(swing / 2);
                int arm_level = mid_level - (int)(swing * 0.20f);
                if (arm_level >= mid_level) arm_level = mid_level - 1;

                for (int s = 0; s < total_samples; s++) {
                    int val = roll_buf_curr[s];
                    if (val < arm_level) {
                        roll_armed = true;
                    } else if (roll_armed && val >= mid_level) {
                        int prev = (s > 0) ? roll_buf_curr[s - 1] : last_roll_adc;
                        float frac = (float)(mid_level - prev) / (float)(val - prev + 1e-5f);
                        uint32_t current_edge = roll_abs_sample + (uint32_t)s;

                        if (roll_last_edge > 0 && current_edge > roll_last_edge) {
                            float period_samples = (float)(current_edge - roll_last_edge) + (frac - roll_last_frac);
                            if (period_samples > 2.0f) {
                                float f = tb.sample_rate_hz / period_samples;
                                if (f >= 1.0f && f <= 100000.0f) {
                                    last_measured_freq = f;
                                    roll_last_trig_ms = now_ms;
                                }
                            }
                        }

                        roll_last_edge = current_edge;
                        roll_last_frac = frac;
                        roll_armed = false;
                    }
                }
            }

            roll_abs_sample += total_samples;

            bool trig_found = (now_ms - roll_last_trig_ms < 600) && (swing >= 2);
            float display_freq = trig_found ? last_measured_freq : 0.0f;

            // Distribute samples evenly across num_cols
            float samples_per_c = (float)total_samples / (float)num_cols;
            const float weight = 1.0f / samples_per_c;

            // Append new columns to ring buffer head
            for (int c = 0; c < num_cols; c++) {
                int s_start = (int)((float)c * samples_per_c);
                int s_end   = (int)((float)(c + 1) * samples_per_c);
                if (s_end > total_samples) s_end = total_samples;
                if (s_start >= s_end) s_start = s_end - 1;

                roll_head = (roll_head + 1) % SCOPE_W;

                memset(density_grid[roll_head], 0, sizeof(density_grid[roll_head]));

                for (int s = s_start; s < s_end; s++) {
                    uint8_t prev_raw = (s == 0) ? last_roll_adc : roll_buf_curr[s - 1];
                    uint8_t cur_raw  = roll_buf_curr[s];

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

            // Delegate tone-mapping, HUD rendering, and SPI display transmission to Core 1
            g_render_roll_head = roll_head;
            g_render_vpp = last_measured_vpp;
            g_render_freq = display_freq;
            g_render_trig = trig_found;
            g_render_tb_idx = current_tb_idx;
            multicore_fifo_push_blocking(CMD_RENDER_ROLL);

            // Wait for background ADC DMA capture of next frame (16.6 ms)
            bool ok = adc_capture_dma_wait();
            if (!ok) {
                roll_primed = false;
                multicore_fifo_pop_blocking(); // Drain Core 1 completion before aborting
                continue;
            }

            // Ensure Core 1 has finished frame transmission
            multicore_fifo_pop_blocking();

            // Ping-pong swap buffer pointers
            uint8_t* tmp = roll_buf_curr;
            roll_buf_curr = roll_buf_next;
            roll_buf_next = tmp;
        }
    }

    return 0;
}

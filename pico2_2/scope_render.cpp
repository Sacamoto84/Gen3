#include "scope_render.h"
#include "scope_config.h"
#include "pico/multicore.h"
#include "FontClassic/FontClassic.h"
#include "timber.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Precomputed mapping for strip-chart
static int screen_to_grid[SCOPE_W];

// Shared variables between Core 0 and Core 1
volatile int g_render_roll_head = SCOPE_W - 1;
volatile float g_render_vpp = 0.0f;
volatile float g_render_freq = 0.0f;
volatile bool g_render_trig = false;
volatile bool g_render_is_env = false;
volatile int g_render_tb_idx = 2;
volatile bool g_render_stopped = false;

// ============================================================================
// DRAWING & RETICLE FUNCTIONS
// ============================================================================
void draw_pixel(int x, int y, uint16_t color) {
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

// Draw the top center trigger marker (indicates X=120 trigger point)
void draw_trigger_marker() {
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
    for (int x = 0; x < SCOPE_W; x++) {
        if ((x % 4) == 0 || (x % 24) == 0) {
            LCD_Buffer16[SCOPE_Y_MID * SCOPE_W + x] = COLOR_AXIS;
        }
    }
    for (int y = SCOPE_Y_MIN; y <= SCOPE_Y_MAX; y++) {
        if (((y - SCOPE_Y_MIN) % 4) == 0 || ((y - SCOPE_Y_MID) % 25) == 0) {
            LCD_Buffer16[y * SCOPE_W + SCOPE_X_MID] = COLOR_AXIS;
        }
    }

    // 5. Trigger marker at top center (X=120)
    draw_trigger_marker();
}

// Single-pixel background grid lookup (used to restore background behind envelope)
uint16_t get_grid_pixel(int x, int y) {
    if (y == SCOPE_Y_MID && ((x % 4) == 0 || (x % 24) == 0)) return COLOR_AXIS;
    if (x == SCOPE_X_MID && (((y - SCOPE_Y_MIN) % 4) == 0 || ((y - SCOPE_Y_MID) % 25) == 0)) return COLOR_AXIS;
    if ((x % 24 == 0) && (y % 4 == 0)) return COLOR_DARKGRID;
    if (((y - SCOPE_Y_MID) % 25 == 0) && (x % 4 == 0)) return COLOR_DARKGRID;
    return COLOR_BLACK;
}

void init_grid_template() {
    for (int b = 0; b < SCOPE_Y_BINS; b++) {
        int y = SCOPE_Y_MIN + b;
        for (int x = 0; x < SCOPE_W; x++) {
            grid_template[b][x] = get_grid_pixel(x, y);
        }
    }
}

// Android-style exponential tone mapping with alpha blending over DSO reticle grid
uint16_t tone_map_density(int col, int y, float density) {
    uint16_t bg = get_grid_pixel(col, y);
    if (density <= 1e-5f) {
        return bg;
    }

    float L = 1.0f - expf(-density * DENSITY_GAIN);
    if (L > 1.0f) L = 1.0f;

    uint32_t bg_r = (bg >> 11) & 0x1F;
    uint32_t bg_g = (bg >> 5) & 0x3F;
    uint32_t bg_b = bg & 0x1F;

    uint32_t r = (uint32_t)((float)bg_r * (1.0f - L) + 31.0f * L + 0.5f);
    uint32_t g = (uint32_t)((float)bg_g * (1.0f - L) + 63.0f * L + 0.5f);
    uint32_t b = (uint32_t)((float)bg_b * (1.0f - L) + 0.5f);

    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;

    return (uint16_t)((r << 11) | (g << 5) | b);
}

// ============================================================================
// ON-SCREEN HUD STATUS BAR
// ============================================================================
void render_hud(const char* tb_label, bool trig, bool is_roll, float freq_hz, float vpp, bool is_env, bool is_stopped) {
    tft.RectangleFilled(0, 0, SCOPE_W - 1, HUD_H - 2, COLOR_BLACK);

    // 1. Timebase label
    char buf[32];
    snprintf(buf, sizeof(buf), "%s/d", tb_label);
    tft.SetFontColor(COLOR_YELLOW, COLOR_BLACK);
    tft.uTFT.CurrentX = 4;
    tft.uTFT.CurrentY = 3;
    FontClassicPuts(&tft, buf, &Font_7x10, 0);

    // 2. Mode badge: [STOP], [ENV], [TRIG], [ROLL], [AUTO]
    if (is_stopped) {
        tft.SetFontColor(COLOR_RED, COLOR_BLACK);
        snprintf(buf, sizeof(buf), "[STOP]");
    } else if (is_env && trig) {
        tft.SetFontColor(COLOR_GREEN, COLOR_BLACK);
        snprintf(buf, sizeof(buf), "[ENV]");
    } else if (trig) {
        tft.SetFontColor(COLOR_GREEN, COLOR_BLACK);
        snprintf(buf, sizeof(buf), "[TRIG]");
    } else if (is_roll) {
        tft.SetFontColor(COLOR_CYAN, COLOR_BLACK);
        snprintf(buf, sizeof(buf), "[ROLL]");
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
    } else if (freq_hz >= 100.0f) {
        snprintf(buf, sizeof(buf), "%4.0fHz", freq_hz);
    } else if (freq_hz > 0.0f) {
        snprintf(buf, sizeof(buf), "%4.1fHz", freq_hz);
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
// CORE 1: DEDICATED DISPLAY & GPU ENGINE (300 MHz)
// ============================================================================
void core1_entry() {
    timber.info("Core 1 online: Dedicated Hardware Display Engine active at 300 MHz!");

    while (true) {
        uint32_t cmd = multicore_fifo_pop_blocking();

        if (cmd == CMD_RENDER_ROLL) {
            int head = g_render_roll_head;
            float vpp = g_render_vpp;
            int tb_idx = g_render_tb_idx;
            bool trig = g_render_trig;
            float freq = g_render_freq;
            bool stopped = g_render_stopped;

            int cur_col = (head + 1) % SCOPE_W;
            for (int X = 0; X < SCOPE_W; X++) {
                screen_to_grid[X] = cur_col;
                cur_col++;
                if (cur_col >= SCOPE_W) cur_col = 0;
            }

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
                        float L = 1.0f - expf(-d * DENSITY_GAIN);
                        if (L > 1.0f) L = 1.0f;

                        uint16_t bg = bg_row[X];
                        uint32_t bg_r = (bg >> 11) & 0x1F;
                        uint32_t bg_g = (bg >> 5) & 0x3F;
                        uint32_t bg_b = bg & 0x1F;

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

            render_hud(TIMEBASES[tb_idx].label, trig, true, freq, vpp, false, stopped);
            draw_trigger_marker();
            tft.driver.ST7789_UpdateDMA16bitV3();
            multicore_fifo_push_blocking(1);
        }
        else if (cmd == CMD_DRAW_GRID) {
            draw_grid();
            tft.driver.ST7789_UpdateDMA16bitV3();
            multicore_fifo_push_blocking(1);
        }
        else if (cmd == CMD_RENDER_SWEEP) {
            int tb_idx = g_render_tb_idx;
            render_hud(TIMEBASES[tb_idx].label, g_render_trig, false, g_render_freq, g_render_vpp, g_render_is_env, g_render_stopped);
            draw_trigger_marker();
            tft.driver.ST7789_UpdateDMA16bitV3();
            multicore_fifo_push_blocking(1);
        }
    }
}

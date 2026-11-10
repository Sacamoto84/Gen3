#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "scope_config.h"

enum CoreCommand {
    CMD_IDLE = 0,
    CMD_RENDER_ROLL,
    CMD_RENDER_SWEEP,
    CMD_DRAW_GRID
};

extern volatile int g_render_roll_head;
extern volatile float g_render_vpp;
extern volatile float g_render_freq;
extern volatile bool g_render_trig;
extern volatile bool g_render_is_env;
extern volatile int g_render_tb_idx;
extern volatile bool g_render_stopped;

// Trigger level rendering globals
extern volatile int g_render_trig_level_y;
extern volatile bool g_render_is_adjusting_trig;
extern volatile float g_render_trig_volts;
extern volatile bool g_render_manual_trig;

void draw_pixel(int x, int y, uint16_t color);
void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void draw_trigger_marker();
void draw_trigger_level_indicator(int y_trig, bool is_adjusting);
void draw_grid();
uint16_t get_grid_pixel(int x, int y);
void init_grid_template();
uint16_t tone_map_density(int col, int y, float density);
void render_hud(const char* tb_label, bool trig, bool is_roll, float freq_hz, float vpp,
                bool is_env = false, bool is_stopped = false,
                bool is_adjusting_trig = false, float trig_volts = 1.65f);

void core1_entry();

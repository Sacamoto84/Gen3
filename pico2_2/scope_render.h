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

void draw_pixel(int x, int y, uint16_t color);
void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void draw_trigger_marker();
void draw_grid();
uint16_t get_grid_pixel(int x, int y);
void init_grid_template();
uint16_t tone_map_density(int col, int y, float density);
void render_hud(const char* tb_label, bool trig, bool is_roll, float freq_hz, float vpp, bool is_env = false);

void core1_entry();

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "pico/types.h"

void buttons_init();
bool check_button(uint pin, bool &prev, uint32_t &press_time, uint32_t &last_repeat);

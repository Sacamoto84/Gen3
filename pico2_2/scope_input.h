#pragma once

#include <stdint.h>
#include <stdbool.h>

void encoder_init();
int encoder_get_delta();
bool encoder_button_pressed();
bool encoder_has_activity();

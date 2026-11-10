#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "uButton.h"

// GyverLibs uButton instance for the encoder button
extern uButton enc_btn;

void encoder_init();
int encoder_get_delta();
bool encoder_has_activity();
void encoder_btn_tick();

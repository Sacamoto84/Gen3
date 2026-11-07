#pragma once

#include <stdint.h>
#include <stdbool.h>

void adc_capture_init();
bool adc_capture_dma_poll(uint8_t* dest, uint32_t count, float clkdiv);
void adc_capture_dma_start(uint8_t* dest, uint32_t count, float clkdiv);
bool adc_capture_dma_wait();

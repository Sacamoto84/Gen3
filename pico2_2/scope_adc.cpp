#include "scope_adc.h"
#include "scope_config.h"
#include "scope_input.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

static int adc_dma_chan = -1;
static dma_channel_config adc_dma_cfg;

void adc_capture_init() {
    adc_init();
    adc_gpio_init(PIN_ADC);
    adc_select_input(ADC_CHANNEL);

    adc_fifo_setup(
        true,    // Write conversions to FIFO
        true,    // Enable DMA request
        1,       // DREQ asserted when >= 1 sample in FIFO
        false,   // No error bit
        true     // byte_shift: Shift bits [11:4] to [7:0] for 8-bit DMA transfers!
    );

    adc_set_clkdiv(0);

    adc_dma_chan = dma_claim_unused_channel(true);
    adc_dma_cfg = dma_channel_get_default_config(adc_dma_chan);
    channel_config_set_transfer_data_size(&adc_dma_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&adc_dma_cfg, false);
    channel_config_set_write_increment(&adc_dma_cfg, true);
    channel_config_set_dreq(&adc_dma_cfg, DREQ_ADC);
}

// Captures `count` samples via DMA. Aborts instantly on encoder turn or button press!
bool adc_capture_dma_poll(uint8_t* dest, uint32_t count, float clkdiv) {
    if (count > ADC_MAX_SAMPLES) count = ADC_MAX_SAMPLES;
    adc_run(false);
    adc_fifo_drain();
    adc_set_clkdiv(clkdiv);

    dma_channel_configure(
        adc_dma_chan,
        &adc_dma_cfg,
        dest,           // Destination buffer
        &adc_hw->fifo,  // Source: ADC FIFO (low byte)
        count,          // Number of samples
        true            // Start immediately
    );

    adc_run(true);

    while (dma_channel_is_busy(adc_dma_chan)) {
        if (encoder_has_activity()) {
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
void adc_capture_dma_start(uint8_t* dest, uint32_t count, float clkdiv) {
    if (count > (ADC_MAX_SAMPLES / 2)) count = ADC_MAX_SAMPLES / 2;
    adc_run(false);     // Safe stop before draining FIFO
    adc_fifo_drain();
    adc_set_clkdiv(clkdiv);

    dma_channel_configure(
        adc_dma_chan,
        &adc_dma_cfg,
        dest,           // Destination buffer
        &adc_hw->fifo,  // Source: ADC FIFO (low byte)
        count,          // Number of samples
        true            // Start immediately
    );

    adc_run(true);
}

// Wait for active DMA capture to complete, polling encoder/button for instant abort
bool adc_capture_dma_wait() {
    while (dma_channel_is_busy(adc_dma_chan)) {
        if (encoder_has_activity()) {
            dma_channel_abort(adc_dma_chan);
            adc_run(false);
            adc_fifo_drain();
            return false; // Aborted by encoder or button
        }
        tight_loop_contents();
    }
    adc_run(false);
    adc_fifo_drain();
    return true;
}

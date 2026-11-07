#include "scope_dsp.h"
#include <stdlib.h>
#include <math.h>

TriggerResult find_trigger_centered(const uint8_t* samples, int count, int pre_trigger_samples, float sample_rate_hz) {
    TriggerResult res = { 0, false, 0.0f, 0.0f, 0.0f, false };
    if (count < pre_trigger_samples + 100) return res;

    // 1. Calculate min, max, swing, and DC midpoint across buffer
    uint8_t v_min = samples[0];
    uint8_t v_max = samples[0];
    for (int i = 1; i < count; i++) {
        if (samples[i] < v_min) v_min = samples[i];
        if (samples[i] > v_max) v_max = samples[i];
    }

    uint16_t total_swing = v_max - v_min;
    res.vpp_volts = (float)total_swing * (3.30f / 256.0f);
    res.vmid_volts = (float)((v_max + v_min) / 2) * (3.30f / 256.0f);

    // If signal swing is too small (< 2 counts ~ 26 mV in 8-bit), flat/noise -> Free-run auto
    if (total_swing < 2) {
        res.trigger_index = pre_trigger_samples;
        res.triggered = false;
        res.frequency_hz = 0.0f;
        res.is_envelope = false;
        return res;
    }

    int mid = (v_min + v_max) / 2;

    // 2. ENVELOPE TRIGGER DETECTOR (for AM / modulated signals on slow sweeps)
    // Active when pre_trigger_samples >= 1000 (>= 1ms/div sweeps)
    if (pre_trigger_samples >= 1000 && count >= 3000) {
        // Window W ~ 1.0 ms to filter out carrier frequencies >= 500 Hz
        int W = (int)(sample_rate_hz * 0.0010f);
        if (W < 48) W = 48;
        if (W > 512) W = 512;

        // Downsample step S ~ 125..160 us per envelope point
        int S = (int)(sample_rate_hz * 0.000130f);
        if (S < 8) S = 8;
        if (count / S >= 4000) {
            S = (count / 3800) + 1;
        }

        uint32_t run_sum = 0;
        for (int i = 0; i < W; i++) {
            run_sum += abs((int)samples[i] - mid);
        }

        static uint8_t env_buf[4096];
        int env_count = 0;
        uint8_t env_min = 255;
        uint8_t env_max = 0;

        for (int i = W; i < count; i++) {
            run_sum += abs((int)samples[i] - mid);
            run_sum -= abs((int)samples[i - W] - mid);

            if ((i % S) == 0 && env_count < 4096) {
                uint8_t val = (uint8_t)(run_sum / W);
                env_buf[env_count++] = val;
                if (val < env_min) env_min = val;
                if (val > env_max) env_max = val;
            }
        }

        uint16_t env_swing = (env_max > env_min) ? (env_max - env_min) : 0;

        // Check if significant AM modulation exists:
        // Modulation depth >= 3 counts and >= 12% of total signal swing
        if (env_swing >= 3 && (env_swing * 8 >= total_swing)) {
            int env_level = (int)env_min + (int)(env_swing / 2);
            int env_arm = env_level - (int)(env_swing * 0.20f); // 20% hysteresis
            if (env_arm >= env_level) env_arm = env_level - 1;

            int start_k = pre_trigger_samples / S;
            int end_k = env_count - (pre_trigger_samples / S) - 2;
            if (end_k <= start_k) end_k = env_count - 2;

            int arm_start_k = (start_k > 20) ? (start_k - 20) : 0;
            bool armed = false;
            int first_edge_k = -1;
            int second_edge_k = -1;
            float frac1 = 0.0f;
            float frac2 = 0.0f;

            for (int k = arm_start_k; k < env_count; k++) {
                int val = env_buf[k];
                if (val < env_arm) {
                    armed = true;
                } else if (armed && val >= env_level) {
                    int prev = env_buf[k - 1];
                    float frac = (float)(env_level - prev) / (float)(val - prev + 1e-5f);
                    if (first_edge_k < 0) {
                        if (k >= start_k && k < end_k) {
                            first_edge_k = k;
                            frac1 = frac;
                            armed = false; // re-arm to find second edge
                        }
                    } else {
                        second_edge_k = k;
                        frac2 = frac;
                        break; // period found!
                    }
                }
            }

            if (first_edge_k >= 0) {
                // Found envelope trigger!
                int trig_samp = W + (int)(((float)first_edge_k + frac1) * (float)S) - (W / 2);
                if (trig_samp < pre_trigger_samples) trig_samp = pre_trigger_samples;
                if (trig_samp > count - pre_trigger_samples) trig_samp = count - pre_trigger_samples;

                res.trigger_index = trig_samp;
                res.triggered = true;
                res.is_envelope = true;

                if (second_edge_k > first_edge_k) {
                    float period_k = ((float)(second_edge_k - first_edge_k) + (frac2 - frac1));
                    float period_samples = period_k * (float)S;
                    if (period_samples > 1.0f) {
                        res.frequency_hz = sample_rate_hz / period_samples;
                    }
                }

                return res; // Locked to AM modulation envelope!
            }
        }
    }

    // 3. CLASSIC CARRIER / EDGE TRIGGER (Fallback for unmodulated signals or fast sweeps)
    int swing = total_swing / 2;
    int level = mid;
    int arm_level = level - (int)(swing * 0.25f);
    if (arm_level >= level) arm_level = level - 1;

    int start_search = pre_trigger_samples;
    int end_search = count - pre_trigger_samples - 10;
    if (end_search <= start_search) end_search = count - 10;

    bool armed = false;
    int first_edge = -1;
    int second_edge = -1;
    float frac1 = 0.0f;
    float frac2 = 0.0f;

    int arm_start = (start_search > 40) ? (start_search - 40) : 0;
    for (int i = arm_start; i < count - 1; i++) {
        int val = samples[i];
        if (val < arm_level) {
            armed = true;
        } else if (armed && val >= level) {
            int prev = samples[i - 1];
            float frac = (float)(level - prev) / (float)(val - prev + 1e-5f);
            if (first_edge < 0) {
                if (i >= start_search && i < end_search) {
                    first_edge = i;
                    frac1 = frac;
                    armed = false;
                }
            } else {
                second_edge = i;
                frac2 = frac;
                break;
            }
        }
    }

    if (first_edge >= 0) {
        res.trigger_index = first_edge;
        res.triggered = true;
        res.is_envelope = false;

        if (second_edge > first_edge) {
            float period_samples = (float)(second_edge - first_edge) + (frac2 - frac1);
            if (period_samples > 1.0f) {
                res.frequency_hz = sample_rate_hz / period_samples;
            }
        }
    } else {
        res.trigger_index = pre_trigger_samples;
        res.triggered = false;
        res.frequency_hz = 0.0f;
        res.is_envelope = false;
    }

    return res;
}

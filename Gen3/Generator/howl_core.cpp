#include "howl_core.h"
#include "generator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HowlEngine Howl CCMRAM;

void HowlEngine::Init() {
	is_active = false;
	source = HOWL_SOURCE_GENERATOR;

	settings.pulse_mode = HOWL_PULSE_WAVELET;
	settings.carrier_hz = 1000;
	settings.wavelet_cycles = 5;
	settings.wavelet_fade = 50;
	settings.multipulse_width_us = 150;
	settings.multipulse_intra_us = 50;

	settings.power_a = 40;
	settings.power_b = 40;
	settings.power_limit = 140; // 70% от 200
	settings.power_step = 5;
	settings.is_muted = false;

	settings.min_freq_hz = 5.0f;
	settings.max_freq_hz = 100.0f;

	settings.auto_inc_en = false;
	settings.auto_inc_step = 1;
	settings.auto_inc_interval_s = 15;

	dma_chunk = 0;
	last_auto_inc_tick = 0;

	current_target_amp = 0.0f;
	current_target_freq_hz = 20.0f;

	phase_carrier = 0;
	wavelet_counter = 0;
	wavelet_stage = 0;
	wavelet_rand_seed = 0x55AA55AA;

	mp_counter = 0;
	mp_stage = 0;
	mp_pulses_left = 1;

	wave_gen.Init();
	activities.Init();
	player.Init();

	Config_Load();
	UpdateTelemetry();
}

void HowlEngine::Start() {
	is_active = true;
	dma_chunk = 0;
	last_auto_inc_tick = HAL_GetTick();
	wavelet_counter = 0;
	wavelet_stage = 0;
	mp_counter = 0;
	mp_stage = 0;
	UpdateTelemetry();
}

void HowlEngine::Stop() {
	is_active = false;
	current_target_amp = 0.0f;
	UpdateTelemetry();
}

void HowlEngine::Update() {
	if (source == HOWL_SOURCE_PLAYER) {
		player.Update();
	}
}

void HowlEngine::SetPower(int16_t p) {
	if (p < 0) p = 0;
	if (p > settings.power_limit) p = settings.power_limit;
	settings.power_a = (uint8_t)p;
	settings.power_b = (uint8_t)p;
	UpdateTelemetry();
}

void HowlEngine::SetPowerLimit(uint8_t limit) {
	if (limit < 20) limit = 20;
	if (limit > 200) limit = 200;
	settings.power_limit = limit;
	if (settings.power_a > limit) settings.power_a = limit;
	if (settings.power_b > limit) settings.power_b = limit;
	UpdateTelemetry();
}

void HowlEngine::ToggleMute() {
	settings.is_muted = !settings.is_muted;
	UpdateTelemetry();
}

void HowlEngine::SetSource(HowlSource src) {
	source = src;
	if (source == HOWL_SOURCE_GENERATOR) {
		wave_gen.Reset();
	} else if (source == HOWL_SOURCE_ACTIVITY) {
		activities.Reset();
	} else if (source == HOWL_SOURCE_PLAYER) {
		player.Play();
	}
	UpdateTelemetry();
}

void HowlEngine::SetPulseMode(HowlPulseMode mode) {
	settings.pulse_mode = mode;
	wavelet_counter = 0;
	wavelet_stage = 0;
	mp_counter = 0;
	mp_stage = 0;
	UpdateTelemetry();
}

void HowlEngine::UpdateTelemetry() {
	telemetry.current_amp = current_target_amp;
	telemetry.current_freq_hz = current_target_freq_hz;
	telemetry.source = source;

	if (source == HOWL_SOURCE_GENERATOR) {
		telemetry.source_name = "Генератор";
		telemetry.activity_name = HowlWaveGenerator::GetShapeName(wave_gen.amp_wave.shape_id);
	} else if (source == HOWL_SOURCE_ACTIVITY) {
		telemetry.source_name = "Активность";
		telemetry.activity_name = HowlActivities::GetActivityName(activities.active_id);
	} else {
		telemetry.source_name = "Плеер";
		telemetry.activity_name = player.current_file[0] ? player.current_file : "Нет файла";
	}

	switch (settings.pulse_mode) {
		case HOWL_PULSE_MULTIPULSE: telemetry.mode_name = "MultiPulse"; break;
		case HOWL_PULSE_CONTINUOUS: telemetry.mode_name = "Continuous"; break;
		case HOWL_PULSE_WAVELET:
		default:                    telemetry.mode_name = "Wavelet"; break;
	}
}

void HowlEngine::Tick_40Hz() {
	HowlPulse pulse = { 0.0f, 0.0f, 0.0f, 0.0f };

	switch (source) {
		case HOWL_SOURCE_GENERATOR:
			pulse = wave_gen.Step();
			break;
		case HOWL_SOURCE_ACTIVITY:
			pulse = activities.Step();
			break;
		case HOWL_SOURCE_PLAYER:
			pulse = player.Step();
			break;
	}

	// Авто-повышение мощности (Auto-Increase Power)
	if (settings.auto_inc_en && !settings.is_muted && settings.power_a < settings.power_limit) {
		uint32_t now = HAL_GetTick();
		if (now - last_auto_inc_tick >= (uint32_t)(settings.auto_inc_interval_s * 1000)) {
			last_auto_inc_tick = now;
			uint16_t next_p = (uint16_t)settings.power_a + settings.auto_inc_step;
			if (next_p > settings.power_limit) next_p = settings.power_limit;
			settings.power_a = (uint8_t)next_p;
			settings.power_b = (uint8_t)next_p;
		}
	}

	// Масштабирование мощности
	if (settings.is_muted || settings.power_a == 0) {
		current_target_amp = 0.0f;
	} else {
		float power_ratio = (float)settings.power_a / 200.0f;
		current_target_amp = pulse.ampA * power_ratio;
		if (current_target_amp > 1.0f) current_target_amp = 1.0f;
	}

	// Масштабирование частоты
	float span = settings.max_freq_hz - settings.min_freq_hz;
	current_target_freq_hz = settings.min_freq_hz + pulse.freqA * span;
	if (current_target_freq_hz < settings.min_freq_hz) current_target_freq_hz = settings.min_freq_hz;
	if (current_target_freq_hz > settings.max_freq_hz) current_target_freq_hz = settings.max_freq_hz;

	telemetry.current_amp = current_target_amp;
	telemetry.current_freq_hz = current_target_freq_hz;
	telemetry.meter_bar = (uint8_t)(pulse.ampA * 100.0f);
}

void HowlEngine::DMA_Render(uint16_t* out, uint16_t* const out_end) {
	// Базовый такт 40 Гц: ровно каждые 10 чанков DMA (25.000 мс)
	if (dma_chunk == 0) {
		Tick_40Hz();
	}
	dma_chunk = (dma_chunk + 1) % 10;

	// Если звук заглушен или амплитуда нулевая — выдаем нейтральный ЦАП (2048 = 0 В перем.)
	if (settings.is_muted || settings.power_a == 0 || current_target_amp <= 0.001f) {
		while (out < out_end) {
			*out++ = 2048;
		}
		return;
	}

	const float volume = current_target_amp;

	if (settings.pulse_mode == HOWL_PULSE_WAVELET) {
		// ════════════════════════════════════════════════════════════════
		// ВЕЙВЛЕТ (Wavelet: пачки несущей со сглаживанием Ханна)
		// ════════════════════════════════════════════════════════════════
		const float carrier_fr = (settings.carrier_hz < 100) ? 100.0f : (float)settings.carrier_hz;
		const float pulse_fr = (current_target_freq_hz < 0.1f) ? 0.1f : current_target_freq_hz;
		const uint8_t cycles = (settings.wavelet_cycles < 1) ? 1 : settings.wavelet_cycles;
		const uint32_t rC = convert_hz_to_dds(carrier_fr);

		const uint32_t burst_len = (uint32_t)(cycles * (SAMPLE_RATE_HZ / carrier_fr));
		const uint32_t period_len = (uint32_t)(SAMPLE_RATE_HZ / pulse_fr);
		const uint32_t rest_len = (period_len > burst_len) ? (period_len - burst_len) : 50;

		float fade_ratio = (float)settings.wavelet_fade * 0.005f;
		if (fade_ratio > 0.45f) fade_ratio = 0.45f;
		uint32_t fade_len = (uint32_t)(burst_len * fade_ratio);
		if (fade_len < 1) fade_len = 1;
		const float inv_fade = 1.0f / (float)fade_len;

		while (out < out_end) {
			float sample = 0.0f;

			if (wavelet_stage == 1) { // BURST (активная вспышка)
				phase_carrier += rC;
				// Синусоидальная несущая из таблицы Gen.CH1.buffer_carrier
				const float raw = (float)Gen.CH1.buffer_carrier[phase_carrier >> 22] - 2048.0f;

				// Гладкая огибающая С1
				float env = 1.0f;
				if (wavelet_counter < fade_len) {
					const float t = (float)wavelet_counter * inv_fade;
					env = t * t * (3.0f - 2.0f * t);
				} else if (wavelet_counter > (burst_len - fade_len)) {
					float t = (float)(burst_len - wavelet_counter) * inv_fade;
					if (t < 0.0f) t = 0.0f;
					env = t * t * (3.0f - 2.0f * t);
				}

				sample = volume * raw * env;

				wavelet_counter++;
				if (wavelet_counter >= burst_len) {
					wavelet_stage = 0; // REST
					wavelet_counter = 0;
				}
			} else { // REST (пауза)
				sample = 0.0f;

				wavelet_counter++;
				if (wavelet_counter >= rest_len) {
					wavelet_stage = 1; // BURST
					wavelet_counter = 0;
					// Zero DC bias: случайная начальная фаза
					wavelet_rand_seed = wavelet_rand_seed * 1103515245U + 12345U;
					phase_carrier = (wavelet_rand_seed & 0x3FF) << 22;
				}
			}

			*out++ = (uint16_t)__USAT((int32_t)(2048.0f + sample), 12);
		}

	} else if (settings.pulse_mode == HOWL_PULSE_MULTIPULSE) {
		// ════════════════════════════════════════════════════════════════
		// MULTIPULSE (двухфазные симметричные прямоугольные импульсы TENS)
		// ════════════════════════════════════════════════════════════════
		const float pulse_fr = (current_target_freq_hz < 0.1f) ? 0.1f : current_target_freq_hz;
		const float us_to_samples = SAMPLE_RATE_HZ * 1e-6f; // 0.5 сэмпла на 1 мкс

		const uint32_t pos_len = (uint32_t)((float)settings.multipulse_width_us * us_to_samples);
		const uint32_t intra_len = (uint32_t)((float)settings.multipulse_intra_us * us_to_samples);
		const uint32_t period_len = (uint32_t)(SAMPLE_RATE_HZ / pulse_fr);
		const uint32_t burst_total = 2 * pos_len + intra_len;
		const uint32_t rest_len = (period_len > burst_total) ? (period_len - burst_total) : 50;

		while (out < out_end) {
			float sample = 0.0f;

			switch (mp_stage) {
				case 1: // POS (+V)
					sample = 2047.0f * volume;
					mp_counter++;
					if (mp_counter >= pos_len) {
						mp_stage = (intra_len > 0) ? 2 : 3;
						mp_counter = 0;
					}
					break;

				case 2: // INTRA (пауза)
					sample = 0.0f;
					mp_counter++;
					if (mp_counter >= intra_len) {
						mp_stage = 3;
						mp_counter = 0;
					}
					break;

				case 3: // NEG (-V)
					sample = -2047.0f * volume;
					mp_counter++;
					if (mp_counter >= pos_len) {
						mp_stage = 0; // REST
						mp_counter = 0;
					}
					break;

				case 0: // REST (пауза между периодами)
				default:
					sample = 0.0f;
					mp_counter++;
					if (mp_counter >= rest_len) {
						mp_stage = 1; // Новый период
						mp_counter = 0;
					}
					break;
			}

			*out++ = (uint16_t)__USAT((int32_t)(2048.0f + sample), 12);
		}

	} else {
		// ════════════════════════════════════════════════════════════════
		// CONTINUOUS (непрерывный гармонический тон с динамической частотой)
		// ════════════════════════════════════════════════════════════════
		const float continuous_fr = (current_target_freq_hz < 0.1f) ? 0.1f : current_target_freq_hz;
		const uint32_t rC = convert_hz_to_dds(continuous_fr);
		while (out < out_end) {
			phase_carrier += rC;
			float raw = (float)Gen.CH1.buffer_carrier[phase_carrier >> 22] - 2048.0f;
			float sample = volume * raw;
			*out++ = (uint16_t)__USAT((int32_t)(2048.0f + sample), 12);
		}
	}
}

bool HowlEngine::Config_Save() {
	FIL f;
	if (f_open(&f, "/Config/howl.ini", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		return false;
	}

	char buf[64];
	snprintf(buf, sizeof(buf), "MODE=%u\r\n", settings.pulse_mode);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "CARRIER=%u\r\n", settings.carrier_hz);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "CYCLES=%u\r\n", settings.wavelet_cycles);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "FADE=%u\r\n", settings.wavelet_fade);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "MP_WIDTH=%u\r\n", settings.multipulse_width_us);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "MP_INTRA=%u\r\n", settings.multipulse_intra_us);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "POWER=%u\r\n", settings.power_a);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "LIMIT=%u\r\n", settings.power_limit);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "AUTO_INC=%u\r\n", settings.auto_inc_en ? 1 : 0);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "AUTO_STEP=%u\r\n", settings.auto_inc_step);
	f_puts(buf, &f);
	snprintf(buf, sizeof(buf), "AUTO_INT=%u\r\n", settings.auto_inc_interval_s);
	f_puts(buf, &f);

	f_close(&f);
	return true;
}

bool HowlEngine::Config_Load() {
	FIL f;
	if (f_open(&f, "/Config/howl.ini", FA_READ | FA_OPEN_EXISTING) != FR_OK) {
		return false;
	}

	char line[64];
	while (f_gets(line, sizeof(line), &f)) {
		if (strncmp(line, "MODE=", 5) == 0) settings.pulse_mode = (uint8_t)atoi(&line[5]);
		else if (strncmp(line, "CARRIER=", 8) == 0) settings.carrier_hz = (uint16_t)atoi(&line[8]);
		else if (strncmp(line, "CYCLES=", 7) == 0) settings.wavelet_cycles = (uint8_t)atoi(&line[7]);
		else if (strncmp(line, "FADE=", 5) == 0) settings.wavelet_fade = (uint8_t)atoi(&line[5]);
		else if (strncmp(line, "MP_WIDTH=", 9) == 0) settings.multipulse_width_us = (uint16_t)atoi(&line[9]);
		else if (strncmp(line, "MP_INTRA=", 9) == 0) settings.multipulse_intra_us = (uint16_t)atoi(&line[9]);
		else if (strncmp(line, "POWER=", 6) == 0) {
			settings.power_a = (uint8_t)atoi(&line[6]);
			settings.power_b = settings.power_a;
		}
		else if (strncmp(line, "LIMIT=", 6) == 0) settings.power_limit = (uint8_t)atoi(&line[6]);
		else if (strncmp(line, "AUTO_INC=", 9) == 0) settings.auto_inc_en = (atoi(&line[9]) != 0);
		else if (strncmp(line, "AUTO_STEP=", 10) == 0) settings.auto_inc_step = (uint8_t)atoi(&line[10]);
		else if (strncmp(line, "AUTO_INT=", 9) == 0) settings.auto_inc_interval_s = (uint16_t)atoi(&line[9]);
	}

	f_close(&f);
	return true;
}

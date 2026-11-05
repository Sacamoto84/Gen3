#include "howl_activities.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static const char* activity_names[HOWL_ACT_COUNT] = {
	"Lick",
	"Penetration",
	"Vibro",
	"Milker",
	"Chaos",
	"Luxury HJ",
	"Opposites",
	"Relentless",
	"Sine Time",
	"Million Seeds"
};

const char* HowlActivities::GetActivityName(ActivityId id) {
	if (id >= HOWL_ACT_COUNT) return "Unknown";
	return activity_names[id];
}

void HowlActivities::Init() {
	active_id = HOWL_ACT_LICK;
	speed_scale = 1.0f;
	intensity_scale = 1.0f;
	million_seed = 424242;
	Reset();
}

void HowlActivities::Reset() {
	tick_count = 0;
	sub_state = 0;
	phase = 0.0f;
	rng_state = 0x12345678 ^ million_seed;
}

void HowlActivities::SelectActivity(ActivityId id) {
	if (id < HOWL_ACT_COUNT) {
		active_id = id;
		Reset();
	}
}

float HowlActivities::RandFloat() {
	rng_state = rng_state * 1664525U + 1013904223U;
	return (float)(rng_state & 0x00FFFFFF) * (1.0f / 16777216.0f);
}

HowlPulse HowlActivities::StepLick() {
	// Цикл 60 тактов (1.5 с при 40 Гц)
	uint32_t cycle = (uint32_t)(60.0f / (speed_scale > 0.1f ? speed_scale : 1.0f));
	if (cycle < 10) cycle = 10;
	uint32_t c_tick = tick_count % cycle;
	float u = (float)c_tick / (float)cycle;

	float amp = 0.0f;
	float freq = 0.2f;

	if (u < 0.35f) {
		// Восходящий мазок (Lick stroke)
		float t = u / 0.35f;
		amp = 0.2f + 0.55f * (t * t);
		freq = 0.25f + 0.35f * t;
	} else if (u < 0.60f) {
		// Быстрый кончик (Tip flick)
		float t = (u - 0.35f) / 0.25f;
		amp = 0.45f * (1.0f - t);
		freq = 0.60f + 0.25f * sinf(t * M_PI * 3.0f);
	} else {
		// Пауза отдыха перед следующим мазком
		amp = 0.05f;
		freq = 0.15f;
	}

	return { amp * intensity_scale, freq, amp * intensity_scale, freq };
}

HowlPulse HowlActivities::StepPenetration() {
	// Цикл 120 тактов (3.0 с)
	uint32_t cycle = (uint32_t)(120.0f / (speed_scale > 0.1f ? speed_scale : 1.0f));
	if (cycle < 20) cycle = 20;
	uint32_t c_tick = tick_count % cycle;
	float u = (float)c_tick / (float)cycle;

	float amp = 0.0f;
	float freq = 0.2f;

	if (u < 0.50f) {
		// Погружение: нарастание мощности, падение частоты (глубокий низкий тон)
		float t = u / 0.50f;
		float smooth = t * t * (3.0f - 2.0f * t);
		amp = 0.20f + 0.70f * smooth;
		freq = 0.45f - 0.30f * smooth; // 40 Гц -> 15 Гц
	} else if (u < 0.65f) {
		// Удержание в глубине
		amp = 0.90f;
		freq = 0.15f;
	} else {
		// Быстрое извлечение: частота растет, мощность спадает
		float t = (u - 0.65f) / 0.35f;
		amp = 0.90f * (1.0f - t) + 0.15f * t;
		freq = 0.15f + 0.45f * t; // 15 Гц -> 60 Гц
	}

	return { amp * intensity_scale, freq, amp * intensity_scale, freq };
}

HowlPulse HowlActivities::StepVibro() {
	phase += 0.025f * speed_scale;
	if (phase >= 1000.0f) phase -= 1000.0f;

	// Быстрое микро-вибрато
	float tremolo = 0.75f + 0.18f * sinf(phase * 2.0f * M_PI * 3.0f);
	float vibrato = 0.50f + 0.35f * sinf(phase * 2.0f * M_PI * 6.5f);

	float amp = tremolo * intensity_scale;
	if (amp > 1.0f) amp = 1.0f;

	return { amp, vibrato, amp, vibrato };
}

HowlPulse HowlActivities::StepMilker() {
	// Цикл 160 тактов (4.0 с)
	uint32_t cycle = (uint32_t)(160.0f / (speed_scale > 0.1f ? speed_scale : 1.0f));
	if (cycle < 20) cycle = 20;
	uint32_t c_tick = tick_count % cycle;
	float u = (float)c_tick / (float)cycle;

	float amp = 0.0f;
	float freq = 0.2f;

	if (u < 0.15f) {
		// Качок 1
		float t = u / 0.15f;
		amp = 0.50f * sinf(t * M_PI);
		freq = 0.35f;
	} else if (u < 0.30f) {
		// Качок 2
		float t = (u - 0.15f) / 0.15f;
		amp = 0.75f * sinf(t * M_PI);
		freq = 0.30f;
	} else if (u < 0.75f) {
		// Мощное сжатие (sustained clamping contraction)
		float t = (u - 0.30f) / 0.45f;
		amp = 0.95f;
		freq = 0.12f + 0.05f * sinf(t * M_PI * 4.0f); // 12-17 Гц для мышечного захвата
	} else {
		// Расслабление
		float t = (u - 0.75f) / 0.25f;
		amp = 0.95f * (1.0f - t);
		freq = 0.25f;
	}

	return { amp * intensity_scale, freq, amp * intensity_scale, freq };
}

HowlPulse HowlActivities::StepChaos() {
	static float cur_amp = 0.3f;
	static float cur_freq = 0.3f;
	static float target_amp = 0.5f;
	static float target_freq = 0.5f;

	if (tick_count % 15 == 0) {
		target_amp = 0.1f + RandFloat() * 0.85f;
		target_freq = 0.05f + RandFloat() * 0.90f;
	}

	cur_amp += (target_amp - cur_amp) * 0.15f * speed_scale;
	cur_freq += (target_freq - cur_freq) * 0.15f * speed_scale;

	return { cur_amp * intensity_scale, cur_freq, cur_amp * intensity_scale, cur_freq };
}

HowlPulse HowlActivities::StepLuxuryHJ() {
	// Длинная 4-стадийная сессия на 1600 тактов (40 с)
	uint32_t session = 1600;
	uint32_t s_tick = tick_count % session;

	float amp = 0.0f;
	float freq = 0.3f;

	if (s_tick < 600) {
		// Фаза 1: Нежные медленные поглаживания (0..15 с)
		float stroke_p = (float)(s_tick % 80) / 80.0f;
		amp = 0.25f + 0.25f * sinf(stroke_p * M_PI);
		freq = 0.25f;
	} else if (s_tick < 1200) {
		// Фаза 2: Нарастающий ритм (15..30 с)
		float stroke_p = (float)((s_tick - 600) % 40) / 40.0f;
		float progress = (float)(s_tick - 600) / 600.0f;
		amp = 0.40f + (0.35f * progress) + 0.20f * sinf(stroke_p * M_PI);
		freq = 0.35f + 0.30f * progress;
	} else if (s_tick < 1450) {
		// Фаза 3: Кульминация (30..36 с) - высокая частота и полная мощность
		float stroke_p = (float)((s_tick - 1200) % 20) / 20.0f;
		amp = 0.85f + 0.15f * sinf(stroke_p * M_PI);
		freq = 0.75f + 0.20f * stroke_p;
	} else {
		// Фаза 4: Послесвечение / отдых (36..40 с)
		float progress = (float)(s_tick - 1450) / 150.0f;
		amp = 0.40f * (1.0f - progress) + 0.10f;
		freq = 0.15f;
	}

	return { amp * intensity_scale, freq, amp * intensity_scale, freq };
}

HowlPulse HowlActivities::StepOpposites() {
	// Чередование каждые 160 тактов (4.0 с):
	// Фаза 0: Быстрая поверхностная щекотка (80 Гц, малая мощность)
	// Фаза 1: Медленные глубокие удары (10 Гц, высокая мощность)
	uint32_t phase_sel = (tick_count / 160) % 2;
	float amp, freq;

	if (phase_sel == 0) {
		amp = 0.30f * intensity_scale;
		freq = 0.85f;
	} else {
		// Пульсирующий удар
		uint32_t beat = tick_count % 40;
		float u = (float)beat / 40.0f;
		amp = (0.40f + 0.55f * expf(-u * 3.0f)) * intensity_scale;
		freq = 0.10f;
	}

	return { amp, freq, amp, freq };
}

HowlPulse HowlActivities::StepRelentless() {
	// Волна нарастания каждые 320 тактов (8 с)
	uint32_t cycle = 320;
	uint32_t c_tick = tick_count % cycle;
	float u = (float)c_tick / (float)cycle;

	// Пилообразное нарастание с микро-ударами
	float base = u * u;
	float pulse = sinf(u * M_PI * 16.0f);
	if (pulse < 0.0f) pulse = 0.0f;

	float amp = (0.20f + 0.65f * base + 0.15f * pulse) * intensity_scale;
	float freq = 0.20f + 0.60f * u;

	return { amp, freq, amp, freq };
}

HowlPulse HowlActivities::StepSineTime() {
	phase += 0.025f * speed_scale;
	if (phase >= 1000.0f) phase -= 1000.0f;

	float amp = (0.50f + 0.45f * sinf(phase * 2.0f * M_PI * 0.15f)) * intensity_scale;
	float freq = 0.50f + 0.45f * sinf(phase * 2.0f * M_PI * 0.2121f); // Несоизмеримая частота

	if (amp > 1.0f) amp = 1.0f;
	return { amp, freq, amp, freq };
}

HowlPulse HowlActivities::StepMillionSeeds() {
	// Детерминированный процедурный генератор от seed
	uint32_t s = million_seed ^ (tick_count / 80);
	s = s * 1664525U + 1013904223U;

	float seed_speed = 0.1f + (float)(s & 0xFF) * (0.8f / 255.0f);
	float seed_amp_base = 0.3f + (float)((s >> 8) & 0xFF) * (0.6f / 255.0f);

	float t = (float)(tick_count % 80) / 80.0f;
	float amp = seed_amp_base * sinf(t * M_PI) * intensity_scale;
	float freq = 0.15f + seed_speed * 0.7f;

	return { amp, freq, amp, freq };
}

HowlPulse HowlActivities::Step() {
	tick_count++;

	switch (active_id) {
		case HOWL_ACT_PENETRATION:   return StepPenetration();
		case HOWL_ACT_VIBRO:         return StepVibro();
		case HOWL_ACT_MILKER:        return StepMilker();
		case HOWL_ACT_CHAOS:         return StepChaos();
		case HOWL_ACT_LUXURY_HJ:     return StepLuxuryHJ();
		case HOWL_ACT_OPPOSITES:     return StepOpposites();
		case HOWL_ACT_RELENTLESS:    return StepRelentless();
		case HOWL_ACT_SINE_TIME:     return StepSineTime();
		case HOWL_ACT_MILLION_SEEDS: return StepMillionSeeds();
		case HOWL_ACT_LICK:
		default:                     return StepLick();
	}
}

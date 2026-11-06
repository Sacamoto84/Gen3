#include "howl_wave_gen.h"
#include <math.h>

#define TMAX 0.99999f

// Определение 18 форм волн Howl
static const WavePoint pts_sawtooth[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ TMAX, 1.0f, 0.0f }
};

static const WavePoint pts_triangle[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.5f, 1.0f, 0.0f }
};

static const WavePoint pts_square[] = {
	{ 0.0f, 1.0f, 0.0f },
	{ 0.4999f, 1.0f, 0.0f },
	{ 0.5f, 0.0f, 0.0f },
	{ TMAX, 0.0f, 0.0f }
};

static const WavePoint pts_constant[] = {
	{ 0.0f, 1.0f, 0.0f },
	{ TMAX, 1.0f, 0.0f }
};

static const WavePoint pts_fangs[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.35f, 1.0f, 0.0f },
	{ 0.5f, 0.5f, 0.0f },
	{ 0.65f, 1.0f, 0.0f }
};

static const WavePoint pts_curvy_triangle[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.5f, 1.0f, 0.0f }
};

static const WavePoint pts_curvy_fangs[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.35f, 1.0f, 0.0f },
	{ 0.5f, 0.5f, 0.0f },
	{ 0.65f, 1.0f, 0.0f }
};

static const WavePoint pts_curvy_trapezium[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.4f, 0.95f, 0.1f },
	{ 0.6f, 0.95f, -0.1f }
};

static const WavePoint pts_gentle_attack[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.75f, 1.0f, 0.0f }
};

static const WavePoint pts_fast_attack[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.25f, 1.0f, 0.0f }
};

static const WavePoint pts_faster_attack[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.15f, 1.0f, 0.0f }
};

static const WavePoint pts_rising_tide[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.1f, 0.4f, 0.0f },
	{ 0.2f, 0.2f, 0.0f },
	{ 0.3f, 0.6f, 0.0f },
	{ 0.4f, 0.4f, 0.0f },
	{ 0.5f, 0.8f, 0.0f },
	{ 0.6f, 0.6f, 0.0f },
	{ 0.7f, 1.0f, 0.0f },
	{ 0.8f, 0.8f, 0.0f }
};

static const WavePoint pts_flourish[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.5f, 0.8f, -0.6f },
	{ 0.66f, 0.6f, 0.3f },
	{ 0.86f, 1.0f, 0.0f },
	{ 0.9f, 1.0f, 0.0f }
};

static const WavePoint pts_jelly[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.2f, 1.0f, 0.0f },
	{ 0.3f, 0.7f, 0.0f },
	{ 0.4f, 1.0f, 0.0f },
	{ 0.5f, 0.7f, 0.0f },
	{ 0.6f, 1.0f, 0.0f },
	{ 0.7f, 0.7f, 0.0f },
	{ 0.8f, 1.0f, 0.0f }
};

static const WavePoint pts_tap_slide[] = {
	{ 0.0f, 1.0f, 0.0f },
	{ 0.0999f, 1.0f, 0.0f },
	{ 0.1f, 0.0f, 0.0f },
	{ 0.1999f, 0.0f, 0.0f },
	{ 0.2f, 1.0f, 0.0f },
	{ 0.2999f, 1.0f, 0.0f },
	{ 0.3f, 0.0f, 0.0f },
	{ 0.3999f, 0.0f, 0.0f },
	{ 0.4f, 1.0f, 0.0f },
	{ 0.5f, 1.0f, 0.0f },
	{ TMAX, 0.0f, 0.0f }
};

static const WavePoint pts_double_time[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.25f, 1.0f, 0.0f },
	{ 0.5f, 0.0f, 0.0f },
	{ 0.625f, 1.0f, 0.0f },
	{ 0.75f, 0.0f, 0.0f },
	{ 0.875f, 1.0f, 0.0f }
};

static const WavePoint pts_triple_trouble[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.10f, 0.98f, 0.05f },
	{ 0.14f, 1.0f, 0.0f },
	{ 0.28f, 0.0f, 0.0f },
	{ 0.38f, 0.98f, 0.05f },
	{ 0.42f, 1.0f, 0.0f },
	{ 0.56f, 0.0f, 0.0f },
	{ 0.66f, 0.98f, 0.05f },
	{ 0.7f, 1.0f, 0.0f },
	{ 0.84f, 0.0f, 0.0f }
};

static const WavePoint pts_steps[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 0.1999f, 0.0f, 0.0f },
	{ 0.2f, 0.25f, 0.0f },
	{ 0.3999f, 0.25f, 0.0f },
	{ 0.4f, 0.5f, 0.0f },
	{ 0.5999f, 0.5f, 0.0f },
	{ 0.6f, 0.75f, 0.0f },
	{ 0.7999f, 0.75f, 0.0f },
	{ 0.8f, 1.0f, 0.0f },
	{ TMAX, 1.0f, 0.0f }
};

static const WaveShapeDef all_shapes[WAVE_SHAPE_COUNT] = {
	{ "Sawtooth",       2,  false, pts_sawtooth },
	{ "Triangle",       2,  false, pts_triangle },
	{ "Square",         4,  false, pts_square },
	{ "Constant",       2,  false, pts_constant },
	{ "Fangs",          4,  false, pts_fangs },
	{ "Curvy Triangle", 2,  true,  pts_curvy_triangle },
	{ "Curvy Fangs",    4,  true,  pts_curvy_fangs },
	{ "Curvy Trapezium",3,  true,  pts_curvy_trapezium },
	{ "Gentle Attack",  2,  true,  pts_gentle_attack },
	{ "Fast Attack",    2,  true,  pts_fast_attack },
	{ "Faster Attack",  2,  true,  pts_faster_attack },
	{ "Rising Tide",    9,  true,  pts_rising_tide },
	{ "Flourish",       5,  true,  pts_flourish },
	{ "Jelly",          8,  true,  pts_jelly },
	{ "Tap+Slide",     11,  false, pts_tap_slide },
	{ "Double Time",    6,  true,  pts_double_time },
	{ "Triple Trouble",10,  true,  pts_triple_trouble },
	{ "Steps",         10,  false, pts_steps }
};

static uint32_t wave_rng = 0x98765432;
static float rand_01() {
	wave_rng = wave_rng * 1664525U + 1013904223U;
	return (float)(wave_rng & 0x00FFFFFF) * (1.0f / 16777216.0f);
}

const char* HowlWaveGenerator::GetShapeName(WaveShapeId id) {
	if (id >= WAVE_SHAPE_COUNT) return "Unknown";
	return all_shapes[id].name;
}

const WaveShapeDef& HowlWaveGenerator::GetShapeDef(WaveShapeId id) {
	if (id >= WAVE_SHAPE_COUNT) return all_shapes[0];
	return all_shapes[id];
}

void WaveManager::Init(WaveShapeId shape, float speed, float min_v, float max_v, bool is_freq) {
	shape_id = shape;
	speed_hz = speed;
	min_val = min_v;
	max_val = max_v;
	phase = 0.0f;
	auto_change_en = false;
	is_frequency = is_freq;
}

void WaveManager::Reset() {
	phase = 0.0f;
}

float WaveManager::EvaluateShape(float u) {
	const WaveShapeDef& def = all_shapes[shape_id];
	const int count = def.num_points;
	const WavePoint* pts = def.points;

	int index = -1;
	for (int i = 0; i < count; i++) {
		if (pts[i].time <= u) {
			index = i;
		} else {
			break;
		}
	}

	float t0, p0, m0;
	float t1, p1, m1;

	if (index == -1) {
		t0 = pts[count - 1].time - 1.0f;
		p0 = pts[count - 1].position;
		m0 = pts[count - 1].slope;
		t1 = pts[0].time;
		p1 = pts[0].position;
		m1 = pts[0].slope;
	} else if (index == count - 1) {
		t0 = pts[count - 1].time;
		p0 = pts[count - 1].position;
		m0 = pts[count - 1].slope;
		t1 = pts[0].time + 1.0f;
		p1 = pts[0].position;
		m1 = pts[0].slope;
	} else {
		t0 = pts[index].time;
		p0 = pts[index].position;
		m0 = pts[index].slope;
		t1 = pts[index + 1].time;
		p1 = pts[index + 1].position;
		m1 = pts[index + 1].slope;
	}

	const float dt = t1 - t0;
	if (dt <= 1e-6f) return p0;

	const float h = (u - t0) / dt;

	if (!def.is_hermite) {
		float pos = p0 + h * (p1 - p0);
		if (pos < 0.0f) pos = 0.0f;
		if (pos > 1.0f) pos = 1.0f;
		return pos;
	} else {
		const float h2 = h * h;
		const float h3 = h2 * h;
		const float h00 = 2.0f * h3 - 3.0f * h2 + 1.0f;
		const float h10 = h3 - 2.0f * h2 + h;
		const float h01 = -2.0f * h3 + 3.0f * h2;
		const float h11 = h3 - h2;
		float pos = h00 * p0 + h10 * dt * m0 + h01 * p1 + h11 * dt * m1;
		if (pos < 0.0f) pos = 0.0f;
		if (pos > 1.0f) pos = 1.0f;
		return pos;
	}
}

void WaveManager::AutoChangeStep(bool is_freq) {
	// Вероятность изменения параметров в конце цикла (как в Howl)
	float r = rand_01();
	if (r < 0.25f) {
		// Смена формы волны
		uint8_t next_shape = (uint8_t)(rand_01() * (float)WAVE_SHAPE_COUNT);
		if (next_shape >= WAVE_SHAPE_COUNT) next_shape = 0;
		shape_id = (WaveShapeId)next_shape;
	} else if (r < 0.50f) {
		// Изменение скорости
		float delta_s = (rand_01() - 0.5f) * 0.1f;
		speed_hz += delta_s;
		if (speed_hz < 0.05f) speed_hz = 0.05f;
		if (speed_hz > 1.5f) speed_hz = 1.5f;
	} else if (r < 0.70f) {
		// Изменение границ
		if (is_freq) {
			min_val = 0.05f + rand_01() * 0.2f;
			max_val = min_val + 0.3f + rand_01() * 0.45f;
			if (max_val > 1.0f) max_val = 1.0f;
		} else {
			min_val = rand_01() * 0.3f;
			max_val = 0.5f + rand_01() * 0.5f;
		}
	}
}

float WaveManager::Step() {
	phase += (speed_hz * (1.0f / 40.0f));
	if (phase >= 1.0f) {
		phase -= 1.0f;
		if (auto_change_en) {
			AutoChangeStep(is_frequency);
		}
	}

	float norm = EvaluateShape(phase);
	return min_val + norm * (max_val - min_val);
}

void WaveManager::Randomize(bool is_freq) {
	shape_id = (WaveShapeId)((uint8_t)(rand_01() * (float)WAVE_SHAPE_COUNT));
	if (shape_id >= WAVE_SHAPE_COUNT) shape_id = WAVE_SHAPE_TRIANGLE;

	speed_hz = 0.08f + rand_01() * 0.5f;

	if (is_freq) {
		min_val = 0.05f + rand_01() * 0.25f;
		max_val = min_val + 0.3f + rand_01() * 0.4f;
		if (max_val > 1.0f) max_val = 1.0f;
	} else {
		min_val = rand_01() * 0.25f;
		max_val = 0.55f + rand_01() * 0.45f;
	}
}

void HowlWaveGenerator::Init() {
	amp_wave.Init(WAVE_SHAPE_CURVY_TRIANGLE, 0.20f, 0.10f, 0.90f, false);
	freq_wave.Init(WAVE_SHAPE_CURVY_TRIANGLE, 0.12f, 0.10f, 0.75f, true);
}

void HowlWaveGenerator::Reset() {
	amp_wave.Reset();
	freq_wave.Reset();
}

HowlPulse HowlWaveGenerator::Step() {
	float amp = amp_wave.Step();
	float freq = freq_wave.Step();
	return { amp, freq, amp, freq };
}

void HowlWaveGenerator::Randomize() {
	amp_wave.Randomize(false);
	freq_wave.Randomize(true);
}

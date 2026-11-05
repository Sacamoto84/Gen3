#ifndef HOWL_TYPES_H_
#define HOWL_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

enum HowlSource : uint8_t {
	HOWL_SOURCE_GENERATOR = 0, // Автономный генератор 2 волн (Amp + Freq)
	HOWL_SOURCE_ACTIVITY  = 1, // Процедурные динамические активности
	HOWL_SOURCE_PLAYER    = 2  // Воспроизведение файлов с SD (.howl / .funscript)
};

enum HowlPulseMode : uint8_t {
	HOWL_PULSE_WAVELET    = 0, // Вейвлеты (пачки несущей с огибающей Ханна)
	HOWL_PULSE_MULTIPULSE = 1, // Симметричные двухфазные импульсы (TENS)
	HOWL_PULSE_CONTINUOUS = 2  // Непрерывный тон несущей
};

struct HowlPulse {
	float ampA;   // 0.0 .. 1.0
	float freqA;  // 0.0 .. 1.0 (нормализованная частота)
	float ampB;   // 0.0 .. 1.0
	float freqB;  // 0.0 .. 1.0
};

enum WaveShapeId : uint8_t {
	WAVE_SHAPE_SAWTOOTH = 0,
	WAVE_SHAPE_TRIANGLE,
	WAVE_SHAPE_SQUARE,
	WAVE_SHAPE_CONSTANT,
	WAVE_SHAPE_FANGS,
	WAVE_SHAPE_CURVY_TRIANGLE,
	WAVE_SHAPE_CURVY_FANGS,
	WAVE_SHAPE_CURVY_TRAPEZIUM,
	WAVE_SHAPE_GENTLE_ATTACK,
	WAVE_SHAPE_FAST_ATTACK,
	WAVE_SHAPE_FASTER_ATTACK,
	WAVE_SHAPE_RISING_TIDE,
	WAVE_SHAPE_FLOURISH,
	WAVE_SHAPE_JELLY,
	WAVE_SHAPE_TAP_SLIDE,
	WAVE_SHAPE_DOUBLE_TIME,
	WAVE_SHAPE_TRIPLE_TROUBLE,
	WAVE_SHAPE_STEPS,
	WAVE_SHAPE_COUNT
};

enum ActivityId : uint8_t {
	HOWL_ACT_LICK = 0,
	HOWL_ACT_PENETRATION,
	HOWL_ACT_VIBRO,
	HOWL_ACT_MILKER,
	HOWL_ACT_CHAOS,
	HOWL_ACT_LUXURY_HJ,
	HOWL_ACT_OPPOSITES,
	HOWL_ACT_RELENTLESS,
	HOWL_ACT_SINE_TIME,
	HOWL_ACT_MILLION_SEEDS,
	HOWL_ACT_COUNT
};

struct HowlSettings {
	uint8_t  pulse_mode;           // HowlPulseMode
	uint16_t carrier_hz;           // Несущая для Wavelet (500..2500 Гц)
	uint8_t  wavelet_cycles;       // Периодов в пачке (1..10)
	uint8_t  wavelet_fade;         // Процент сглаживания фронта (0..100%)
	uint16_t multipulse_width_us;  // Длительность полупериода в мкс (50..500)
	uint16_t multipulse_intra_us;  // Пауза между полуволнами в мкс (0..200)

	uint8_t  power_a;              // Мощность канала A (0..200)
	uint8_t  power_b;              // Мощность канала B (0..200)
	uint8_t  power_limit;          // Максимальная допустимая мощность (20..200)
	uint8_t  power_step;           // Шаг изменения мощности (1..10)
	bool     is_muted;             // Быстрое отключение (MUTE)

	float    min_freq_hz;          // Минимальная частота диапазона (5.0 Гц)
	float    max_freq_hz;          // Максимальная частота диапазона (100.0 Гц)

	bool     auto_inc_en;          // Авто-повышение мощности
	uint8_t  auto_inc_step;        // Шаг авто-повышения (+1..+5)
	uint16_t auto_inc_interval_s;  // Интервал в секундах (5..60)
};

struct HowlTelemetry {
	float       current_amp;       // 0.0 .. 1.0 (текущая амплитуда после мощности)
	float       current_freq_hz;   // Текущая частота в Гц
	uint8_t     meter_bar;         // 0 .. 100 для графического индикатора
	HowlSource  source;
	const char* source_name;
	const char* mode_name;
	const char* activity_name;
};

#endif /* HOWL_TYPES_H_ */

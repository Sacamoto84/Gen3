#ifndef HOWL_CORE_H_
#define HOWL_CORE_H_

#include "howl_types.h"
#include "howl_wave_gen.h"
#include "howl_activities.h"
#include "howl_player.h"

class HowlEngine {
public:
	bool               is_active;
	HowlSource         source;
	HowlSettings       settings;
	HowlTelemetry      telemetry;

	HowlWaveGenerator  wave_gen;
	HowlActivities     activities;
	HowlPlayer         player;

	void Init();
	void Start();
	void Stop();
	void Update();

	void SetPower(int16_t p);
	void SetPowerLimit(uint8_t limit);
	void ToggleMute();
	void SetSource(HowlSource src);
	void SetPulseMode(HowlPulseMode mode);

	// Вызывается из DMA ISR прерывания
	void DMA_Render(uint16_t* out, uint16_t* const out_end);

	// Сохранение и загрузка настроек с SD (/Config/howl.ini)
	bool Config_Save();
	bool Config_Load();

private:
	uint8_t  dma_chunk;
	uint32_t last_auto_inc_tick;

	// Параметры текущего импульса (обновляются раз в 25 мс / 40 Гц)
	float    current_target_amp;
	float    current_target_freq_hz;

	// Внутренние состояния синтезатора
	uint32_t phase_carrier;
	uint32_t wavelet_counter;
	uint8_t  wavelet_stage;
	uint32_t wavelet_rand_seed;

	uint32_t mp_counter;
	uint8_t  mp_stage;
	uint8_t  mp_pulses_left;

	void Tick_40Hz();
	void UpdateTelemetry();
};

extern HowlEngine Howl;

#endif /* HOWL_CORE_H_ */

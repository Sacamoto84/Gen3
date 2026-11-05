#include "Generator.h"
#include "tim.h"

#include "HiSpeedDWT.h"
extern HiSpeedDWT TimerT5;

#include "timber.h"

uint16_t OUTPUT[OUTPUTSAMPLES] RAM_16;

FIL SDFile1 CCMRAM;

//Функция рендерит carrier из файла "/Carrier/_CH->Carrier_mod"
void GENERATOR::Create_Carrier(_structure_ch *_CH) {
	char str[40] = { 0 };

	//По индексу определяем имя файла, его открываем и записываем во временный буффер
	sprintf(str, "/Carrier/%s", _CH->Carrier_mod);

	if (f_open(&SDFile, str, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
		timber.print(">Open OK %s\n", str);
		UINT testByte;
		f_read(&SDFile, &buffer_temp, 2048, &testByte);
		f_close(&SDFile);
	} else
	    timber.print(">Open Error %s\n", str);

	uint16_t *p;
    p = (uint16_t*)&buffer_temp[0];

	for (uint16_t i = 0; i < 1024; i++)
		_CH->buffer_carrier[i] = *p++;

}

//Прочечть файл и заполнить массив AM модуляции "/Mod/_CH->AM_mod"
void GENERATOR::Create_AM_Modulation(_structure_ch *_CH) {

	//SEGGER_RTT_printf(0, "┌─ Create_AM_Modulation\n");

	char str[40] = { 0 };

	//По индексу определяем имя файла, его открываем и записываем во временный буффер
	sprintf(str, "/Mod/%s", _CH->AM_mod);

	uint16_t i;

	if (f_open(&SDFile, str, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
		//SEGGER_RTT_printf(0, "└> OK %s\n", str);
		UINT testByte;
		f_read(&SDFile, &buffer_temp, 2048, &testByte);
		f_close(&SDFile);
	} else
	    timber.print("└> Ошибка %s\n", str);

	uint16_t *p;
    p = (uint16_t*)&buffer_temp[0];

    //14us
	for (i = 0; i < 1024; i++){
		_CH->buffer_am[i] = *p++;}
}

//Прочесть файл FM модуляции и записать в буффер "/Mod/_CH->FM_mod"
void GENERATOR::Create_FM_Modulation(_structure_ch *_CH) {

    timber.print("\033[01;38;05;232;48;05;201mGEN\x1B[0m>Create_FM_Modulation CH%d\n", _CH->CH + 1);

	float x, y;
	//UINT *p;
	char str[40] = { 0 };

	x = _CH->FM_Base - _CH->FM_Dev;
	y = _CH->FM_Dev * 2;

	sprintf(str, "/Mod/%s", _CH->FM_mod);

	uint16_t i;

	if (f_open(&SDFile, str, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
		timber.print(">Open OK %s\n", str);
		UINT testByte;

		f_read(&SDFile, &cacheFM, 2048, &testByte);

		f_close(&SDFile);
	} else
        timber.print(">Open Error %s\n", str);
	///////////////////
	uint16_t *p = (uint16_t*)&cacheFM[0];

	float buffer[FM_MODSAMPLES];

	 //40us
	for (i = 0; i < 512; i++) {
		//_CH->buffer_fm[i] = x + (y * (float)*p / 4095.0F);
		buffer[i] = x + (y * (float)*p / 4095.0F);
		p += 2;
	}

	//7 us
	uint32_t *f1 = (uint32_t*)&_CH->buffer_fm[0];
	uint32_t *f2 = (uint32_t*)&buffer[0];
	for (i = 0; i < 512/4/2/2; i++) {
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
	}

	//TimerT5.Loger((char *)"FM1rr");

}

//Прочесть файл FM модуляции и записать в буффер "/Mod/_CH->FM_mod"
void GENERATOR::Refresh_FM_Modulation(_structure_ch *_CH) {

    timber.print("\033[01;38;05;232;48;05;68mGEN\x1B[0m>Refresh_FM_Modulation CH%d\n", _CH->CH + 1);

	float x, y;

	x = _CH->FM_Base - _CH->FM_Dev;
	y = _CH->FM_Dev * 2;

	uint16_t i;

	uint16_t *p = (uint16_t*)&cacheFM[0];

	float buffer[FM_MODSAMPLES];

	 //40us
	for (i = 0; i < 512; i++) {
		//_CH->buffer_fm[i] = x + (y * (float)*p / 4095.0F);
		buffer[i] = x + (y * (float)*p / 4095.0F);
		p += 2;
	}

	//7 us
	uint32_t *f1 = (uint32_t*)&_CH->buffer_fm[0];
	uint32_t *f2 = (uint32_t*)&buffer[0];
	for (i = 0; i < 512/4/2/2; i++) {
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
		*f1++ = *f2++;
	}
}

void GENERATOR::Init(void) {
	CH1.Volume_Script = 1.0F;

	//Значения по умолчанию для новых режимов стимуляции
	CH1.Mode = GEN_MODE_CONTINUOUS;
	CH1.Wavelet_Pulse_fr = 25.0F;
	CH1.Wavelet_Cycles = 5;
	CH1.Wavelet_Fade = 50;
	CH1.Multi_Pulse_fr = 30.0F;
	CH1.Multi_Pulse_us = 300;
	CH1.Multi_Intra_us = 50;
	CH1.Multi_Count = 1;
	Reset_Stim_States();
	Howl.Init();

	/**************************
	 *  DMA DAC
	 **************************/
	DAC1->CR |= DAC_CR_DMAEN1 | DAC_CR_EN1;

	//Размер передачи 16 бит задан в MX_DAC_Init (hdma_dac1)
	DMA1_Stream5->CR &= ~DMA_SxCR_EN; //Выключаем DMA
	DMA1_Stream5->NDTR = OUTPUTSAMPLES / 2;
	DMA1_Stream5->PAR = (uint32_t) &DAC1->DHR12R1; //DAC канал 1, 12 бит
	DMA1_Stream5->M0AR = (uint32_t) &OUTPUT[0];
	DMA1_Stream5->M1AR = (uint32_t) &OUTPUT[OUTPUTSAMPLES / 2];
	//DMA1_Stream5->CR |=  DMA_SxCR_CIRC;// | DMA_SxCR_HTIE;
	////DMA1_Stream5->CR &= ~DMA_SxCR_TCIE;
	DMA1_Stream5->CR |= DMA_SxCR_TCIE | DMA_SxCR_CIRC | DMA_SxCR_DBM;// DMA_SxCR_HTIE; //Прерывание | Циклический режим | //Двойной буффер
	DMA1_Stream5->CR |= DMA_SxCR_EN;

	HAL_TIM_Base_Start(&htim6);  //Запуск таймера6 для DAC
}

//Секция ФНЧ, прямая форма I: в float32 меньше шумит при низком срезе, чем транспонированная II
static inline float biquad(const _biquad &c, float *s, float x) {
	const float y = c.b0 * (x + 2.0F * s[0] + s[1]) - c.a1 * s[2] - c.a2 * s[3];
	s[1] = s[0];
	s[0] = x;
	s[3] = s[2];
	s[2] = y;
	return y;
}

void GENERATOR::syg_update(int Buffer_Current) {
	uint16_t *out = &OUTPUT[Buffer_Current ? OUTPUTSAMPLES / 2 : 0];
	uint16_t *const out_end = out + OUTPUTSAMPLES / 2;

	if (Howl.is_active) {
		Howl.DMA_Render(out, out_end);
		return;
	}

	if (!CH1.CH_EN) {
		memset(filter_state, 0, sizeof(filter_state));
		while (out < out_end)
			*out++ = 2048;
		return;
	}

	const float volume = CH1.Volume * CH1.Volume_Script;
	const _filter_coef coef = filter_coef[filter_active];
	float state[FILTER_SECTIONS][4];
	memcpy(state, filter_state, sizeof(state));

	//Отключенные секции начинают с нуля, когда их включат снова
	for (uint32_t n = coef.sections; n < FILTER_SECTIONS; n++)
		memset(state[n], 0, sizeof(state[n]));

	if (CH1.Mode == GEN_MODE_WAVELET) {
		// ════════════════════════════════════════════════════════════════
		// РЕЖИМ 1: WAVELET (пачки несущей с огибающей Ханна + паузы)
		// ════════════════════════════════════════════════════════════════
		CH1.rC = convertHzToR(CH1.Carrier_fr);
		const uint32_t rC = CH1.rC;
		uint32_t phase_carrier = CH1.phase_accumulator_carrier;

		const float carrier_fr = (CH1.Carrier_fr < 10) ? 10.0F : (float)CH1.Carrier_fr;
		const float pulse_fr = (CH1.Wavelet_Pulse_fr < 0.1F) ? 0.1F : CH1.Wavelet_Pulse_fr;
		const uint8_t cycles = (CH1.Wavelet_Cycles < 1) ? 1 : CH1.Wavelet_Cycles;

		const uint32_t burst_len = (uint32_t)(cycles * (SAMPLE_RATE_HZ / carrier_fr));
		const uint32_t period_len = (uint32_t)(SAMPLE_RATE_HZ / pulse_fr);
		const uint32_t rest_len = (period_len > burst_len) ? (period_len - burst_len) : 100;

		float fade_ratio = (float)CH1.Wavelet_Fade * 0.005F; // 50% -> 0.25 от длины пачки на фронт
		if (fade_ratio > 0.45F) fade_ratio = 0.45F;
		uint32_t fade_len = (uint32_t)(burst_len * fade_ratio);
		if (fade_len < 1) fade_len = 1;
		const float inv_fade = 1.0F / (float)fade_len;

		while (out < out_end) {
			float sample = 0.0F;

			if (wavelet_stage == 1) { // BURST (активная вспышка)
				phase_carrier += rC;
				const float raw = (float)CH1.buffer_carrier[phase_carrier >> 22] - 2048.0F;

				// Гладкая огибающая С1 (3t^2 - 2t^3)
				float env = 1.0F;
				if (wavelet_counter < fade_len) {
					const float t = (float)wavelet_counter * inv_fade;
					env = t * t * (3.0F - 2.0F * t);
				} else if (wavelet_counter > (burst_len - fade_len)) {
					float t = (float)(burst_len - wavelet_counter) * inv_fade;
					if (t < 0.0F) t = 0.0F;
					env = t * t * (3.0F - 2.0F * t);
				}

				sample = volume * raw * env;

				wavelet_counter++;
				if (wavelet_counter >= burst_len) {
					wavelet_stage = 0; // переход в паузу (REST)
					wavelet_counter = 0;
				}
			} else { // REST (пауза между пачками)
				sample = 0.0F;

				wavelet_counter++;
				if (wavelet_counter >= rest_len) {
					wavelet_stage = 1; // старт новой пачки
					wavelet_counter = 0;
					// Zero DC bias: случайная фаза при старте пачки
					wavelet_rand_seed = wavelet_rand_seed * 1103515245U + 12345U;
					phase_carrier = (wavelet_rand_seed & 0x3FF) << 22;
				}
			}

			if (coef.sections > 0) {
				sample = biquad(coef.s[0], state[0], sample);
				if (coef.sections > 1)
					sample = biquad(coef.s[1], state[1], sample);
			}

			*out++ = (uint16_t) __USAT((int32_t) (2048.0F + sample), 12);
		}

		CH1.phase_accumulator_carrier = phase_carrier;

	} else if (CH1.Mode == GEN_MODE_MULTIPULSE) {
		// ════════════════════════════════════════════════════════════════
		// РЕЖИМ 2: MULTIPULSE (двухфазные симметричные прямоугольные импульсы)
		// ════════════════════════════════════════════════════════════════
		const float pulse_fr = (CH1.Multi_Pulse_fr < 0.1F) ? 0.1F : CH1.Multi_Pulse_fr;
		const float us_to_samples = SAMPLE_RATE_HZ * 1e-6F; // ровно 0.5 сэмплов на 1 мкс (1 сэмпл = 2.0 мкс)

		const uint32_t pos_len   = (uint32_t)((float)CH1.Multi_Pulse_us * us_to_samples);
		const uint32_t intra_len = (uint32_t)((float)CH1.Multi_Intra_us * us_to_samples);
		const uint32_t inter_len = (pos_len > 0) ? pos_len : 1;
		const uint8_t  burst_cnt = (CH1.Multi_Count < 1) ? 1 : CH1.Multi_Count;

		const uint32_t period_len = (uint32_t)(SAMPLE_RATE_HZ / pulse_fr);
		const uint32_t burst_total = burst_cnt * (2 * pos_len + intra_len) + (burst_cnt - 1) * inter_len;
		const uint32_t rest_len = (period_len > burst_total) ? (period_len - burst_total) : 100;

		while (out < out_end) {
			float sample = 0.0F;

			switch (mp_stage) {
				case 1: // POS (+V)
					sample = 2047.0F * volume;
					mp_counter++;
					if (mp_counter >= pos_len) {
						mp_stage = (intra_len > 0) ? 2 : 3;
						mp_counter = 0;
					}
					break;

				case 2: // INTRA (пауза между полуволнами)
					sample = 0.0F;
					mp_counter++;
					if (mp_counter >= intra_len) {
						mp_stage = 3; // переходим к NEG
						mp_counter = 0;
					}
					break;

				case 3: // NEG (-V, равная площадь заряда)
					sample = -2047.0F * volume;
					mp_counter++;
					if (mp_counter >= pos_len) {
						mp_counter = 0;
						if (mp_pulses_left > 1) {
							mp_pulses_left--;
							mp_stage = 4; // INTER
						} else {
							mp_stage = 0; // REST
						}
					}
					break;

				case 4: // INTER (пауза между импульсами в пачке)
					sample = 0.0F;
					mp_counter++;
					if (mp_counter >= inter_len) {
						mp_stage = 1; // следующий импульс
						mp_counter = 0;
					}
					break;

				case 0: // REST (пауза между периодами)
				default:
					sample = 0.0F;
					mp_counter++;
					if (mp_counter >= rest_len) {
						mp_stage = 1; // старт нового периода
						mp_counter = 0;
						mp_pulses_left = burst_cnt;
					}
					break;
			}

			if (coef.sections > 0) {
				sample = biquad(coef.s[0], state[0], sample);
				if (coef.sections > 1)
					sample = biquad(coef.s[1], state[1], sample);
			}

			*out++ = (uint16_t) __USAT((int32_t) (2048.0F + sample), 12);
		}

	} else {
		// ════════════════════════════════════════════════════════════════
		// РЕЖИМ 0: CONTINUOUS (классический DDS Несущая + AM/FM)
		// ════════════════════════════════════════════════════════════════
		CH1.rC = convertHzToR(CH1.Carrier_fr);
		CH1.rAM = convertHzToR(CH1.AM_fr);
		CH1.rFM = convertHzToR(CH1.FM_mod_fr);

		const uint32_t rC = CH1.rC;
		const uint32_t rAM = CH1.rAM;
		const uint32_t rFM = CH1.rFM;
		const float fm_to_r = convertHzToR(1.0F);
		const bool am_en = CH1.AM_EN;
		const bool fm_en = CH1.FM_EN;

		uint32_t phase_carrier = CH1.phase_accumulator_carrier;
		uint32_t phase_am = CH1.phase_accumulator_mod;
		uint32_t phase_fm = CH1.phase_accumulator_fm;

		while (out < out_end) {
			if (fm_en) {
				phase_fm += rFM;
				phase_carrier += (uint32_t) (CH1.buffer_fm[phase_fm >> 23] * fm_to_r);
			} else
				phase_carrier += rC;

			float sample = volume * (CH1.buffer_carrier[phase_carrier >> 22] - 2048.0F);

			if (am_en) {
				phase_am += rAM;
				sample *= CH1.buffer_am[phase_am >> 22] * (1.0F / 4095.0F);
			}

			if (coef.sections > 0) {
				sample = biquad(coef.s[0], state[0], sample);
				if (coef.sections > 1)
					sample = biquad(coef.s[1], state[1], sample);
			}

			*out++ = (uint16_t) __USAT((int32_t) (2048.0F + sample), 12);
		}

		CH1.phase_accumulator_carrier = phase_carrier;
		CH1.phase_accumulator_mod = phase_am;
		CH1.phase_accumulator_fm = phase_fm;
	}

	memcpy(filter_state, state, sizeof(state));
}

//tan цепной дробью на FPU, без libm. Ошибка < 1e-6 при x <= 1 рад (частота среза * 1.6 при 100 кГц)
static float tan_small(float x) {
	const float x2 = x * x;
	float t = 7.0F - x2 / 9.0F;
	t = 5.0F - x2 / t;
	t = 3.0F - x2 / t;
	t = 1.0F - x2 / t;
	return x / t;
}

void GENERATOR::Filter_Apply(void) {
	//Q и множитель частоты секции, чтобы срез всего фильтра был по -3 dB
	static const struct { float q, m; } proto[2][2][FILTER_SECTIONS] = {
		{ { { 0.70711F, 1.0F },     { 0.0F, 0.0F } },         //Butterworth 2
		  { { 0.54120F, 1.0F },     { 1.30656F, 1.0F } } },   //Butterworth 4
		{ { { 0.57735F, 1.27202F }, { 0.0F, 0.0F } },         //Bessel 2
		  { { 0.52193F, 1.41930F }, { 0.80554F, 1.59102F } } }, //Bessel 4
	};

	const uint8_t next = filter_active ^ 1;
	_filter_coef &c = filter_coef[next];
	const int order_index = (filter_cfg.Order == 2) ? 0 : 1;

	c.sections = filter_cfg.EN ? order_index + 1 : 0;

	for (int n = 0; n <= order_index; n++) {
		const float q = proto[filter_cfg.Type][order_index][n].q;
		const float m = proto[filter_cfg.Type][order_index][n].m;
		const float k = tan_small(3.14159265F * filter_cfg.Fr * m / SAMPLE_RATE_HZ);
		const float d = 1.0F + k / q + k * k;

		c.s[n].a1 = 2.0F * (k * k - 1.0F) / d;
		c.s[n].a2 = (1.0F - k / q + k * k) / d;
		c.s[n].b0 = (1.0F + c.s[n].a1 + c.s[n].a2) / 4.0F; //Из округленных a1 a2: усиление на DC ровно 1
	}

	filter_active = next;
}

void GENERATOR::Filter_Load(void) {
	filter_cfg.EN = 0;
	filter_cfg.Type = FILTER_BESSEL; //Без выбросов на прямоугольнике
	filter_cfg.Order = 4;
	filter_cfg.Fr = 20000;

	memset(filter_coef, 0, sizeof(filter_coef));
	memset(filter_state, 0, sizeof(filter_state));
	filter_active = 0;

	if (f_open(&SDFile, "/Config/filter.ini", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
		char line[24];
		while (f_gets(line, sizeof(line), &SDFile)) {
			if (strncmp(line, "EN=", 3) == 0)
				filter_cfg.EN = atoi(&line[3]) != 0;
			else if (strncmp(line, "TYPE=", 5) == 0)
				filter_cfg.Type = (strncmp(&line[5], "BUTTER", 6) == 0) ? FILTER_BUTTERWORTH : FILTER_BESSEL;
			else if (strncmp(line, "ORDER=", 6) == 0)
				filter_cfg.Order = (atoi(&line[6]) == 2) ? 2 : 4;
			else if (strncmp(line, "FR=", 3) == 0) {
				const int fr = atoi(&line[3]);
				filter_cfg.Fr = (fr < FILTER_FR_MIN) ? FILTER_FR_MIN : (fr > FILTER_FR_MAX) ? FILTER_FR_MAX : fr;
			}
		}
		f_close(&SDFile);
	}

	Filter_Apply();
}

bool GENERATOR::Filter_Save(void) {
	char str[64];
	const int len = snprintf(str, sizeof(str), "EN=%d\r\nTYPE=%s\r\nORDER=%d\r\nFR=%lu\r\n",
			filter_cfg.EN, (filter_cfg.Type == FILTER_BUTTERWORTH) ? "BUTTER" : "BESSEL",
			filter_cfg.Order, filter_cfg.Fr);

	return Save_File("/Config/filter.ini", str, len);
}

bool GENERATOR::Save_File(const char *path, const char *text, int len) {
	pause(); //Как в MSD: без прерываний DMA на время записи на SD

	f_mkdir("/Config"); //Если уже есть, вернет FR_EXIST

	bool ok = false;
	if (f_open(&SDFile, path, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
		UINT written;
		ok = (f_write(&SDFile, text, len, &written) == FR_OK) && (written == (UINT) len);
		ok = (f_close(&SDFile) == FR_OK) && ok;
	}

	start();

	if (!ok)
		timber.print(">Save error %s\n", path);

	return ok;
}

bool GENERATOR::Config_Save(void) {
	char str[384];
	const int len = snprintf(str, sizeof(str),
			"CH_EN=%d\r\nCR_FR=%d\r\nCR_MOD=%s\r\n"
			"AM_EN=%d\r\nAM_FR=%.1f\r\nAM_MOD=%s\r\n"
			"FM_EN=%d\r\nFM_BASE=%d\r\nFM_DEV=%.1f\r\nFM_MOD=%s\r\nFM_FR=%.1f\r\n"
			"VOLUME=%d\r\n"
			"MODE=%d\r\n"
			"WAV_FR=%.1f\r\nWAV_CYC=%d\r\nWAV_FADE=%d\r\n"
			"MP_FR=%.1f\r\nMP_US=%d\r\nMP_INTRA=%d\r\nMP_CNT=%d\r\n",
			CH1.CH_EN, CH1.Carrier_fr, CH1.Carrier_mod,
			CH1.AM_EN, CH1.AM_fr, CH1.AM_mod,
			CH1.FM_EN, CH1.FM_Base, CH1.FM_Dev, CH1.FM_mod, CH1.FM_mod_fr,
			Volume_Get(),
			CH1.Mode,
			CH1.Wavelet_Pulse_fr, CH1.Wavelet_Cycles, CH1.Wavelet_Fade,
			CH1.Multi_Pulse_fr, CH1.Multi_Pulse_us, CH1.Multi_Intra_us, CH1.Multi_Count);

	return Save_File("/Config/ch1.ini", str, len) && Filter_Save();
}

bool GENERATOR::Config_Load(void) {
	Volume_Set(VOLUME_DEFAULT);
	CH1.Volume_Script = 1.0F;

	//Значения по умолчанию для стимуляции
	CH1.Mode = GEN_MODE_CONTINUOUS;
	CH1.Wavelet_Pulse_fr = 25.0F;
	CH1.Wavelet_Cycles = 5;
	CH1.Wavelet_Fade = 50;
	CH1.Multi_Pulse_fr = 30.0F;
	CH1.Multi_Pulse_us = 300;
	CH1.Multi_Intra_us = 50;
	CH1.Multi_Count = 1;
	Reset_Stim_States();
	Howl.Init();

	if (f_open(&SDFile, "/Config/ch1.ini", FA_OPEN_EXISTING | FA_READ) != FR_OK)
		return false;

	char line[48];
	while (f_gets(line, sizeof(line), &SDFile)) {
		line[strcspn(line, "\r\n")] = 0;

		char *value = strchr(line, '=');
		if (value == NULL)
			continue;
		*value++ = 0;

		if      (strcmp(line, "CH_EN") == 0)   CH1.CH_EN = atoi(value) != 0;
		else if (strcmp(line, "CR_FR") == 0)   CH1.Carrier_fr = atoi(value);
		else if (strcmp(line, "CR_MOD") == 0)  snprintf(CH1.Carrier_mod, sizeof(CH1.Carrier_mod), "%s", value);
		else if (strcmp(line, "AM_EN") == 0)   CH1.AM_EN = atoi(value) != 0;
		else if (strcmp(line, "AM_FR") == 0)   CH1.AM_fr = atof(value);
		else if (strcmp(line, "AM_MOD") == 0)  snprintf(CH1.AM_mod, sizeof(CH1.AM_mod), "%s", value);
		else if (strcmp(line, "FM_EN") == 0)   CH1.FM_EN = atoi(value) != 0;
		else if (strcmp(line, "FM_BASE") == 0) CH1.FM_Base = atoi(value);
		else if (strcmp(line, "FM_DEV") == 0)  CH1.FM_Dev = atof(value);
		else if (strcmp(line, "FM_MOD") == 0)  snprintf(CH1.FM_mod, sizeof(CH1.FM_mod), "%s", value);
		else if (strcmp(line, "FM_FR") == 0)   CH1.FM_mod_fr = atof(value);
		else if (strcmp(line, "VOLUME") == 0)  Volume_Set(atoi(value));
		else if (strcmp(line, "MODE") == 0)    CH1.Mode = (uint8_t)atoi(value);
		else if (strcmp(line, "WAV_FR") == 0)  CH1.Wavelet_Pulse_fr = (float)atof(value);
		else if (strcmp(line, "WAV_CYC") == 0) CH1.Wavelet_Cycles = (uint8_t)atoi(value);
		else if (strcmp(line, "WAV_FADE") == 0)CH1.Wavelet_Fade = (uint8_t)atoi(value);
		else if (strcmp(line, "MP_FR") == 0)   CH1.Multi_Pulse_fr = (float)atof(value);
		else if (strcmp(line, "MP_US") == 0)   CH1.Multi_Pulse_us = (uint16_t)atoi(value);
		else if (strcmp(line, "MP_INTRA") == 0)CH1.Multi_Intra_us = (uint16_t)atoi(value);
		else if (strcmp(line, "MP_CNT") == 0)  CH1.Multi_Count = (uint8_t)atoi(value);
	}
	f_close(&SDFile);

	if (CH1.Carrier_mod[0] == 0) //Файл пустой или битый: пусть стартует скрипт по умолчанию
		return false;

	Create_Carrier(&CH1);
	Create_AM_Modulation(&CH1);
	Create_FM_Modulation(&CH1);

	Reset_Stim_States();

	return true;
}

void GENERATOR::read_save_ini_to_structure_ch(void)
{
}

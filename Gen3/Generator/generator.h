#ifndef GENERATOR_H
#define GENERATOR_H

#include "main.h"
#include "tim.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "dac.h"

extern DMA_HandleTypeDef hdma_dac1;

#define SAMPLES        1024
#define AM_MODSAMPLES  1024
#define FM_MODSAMPLES  512
#define OUTPUTSAMPLES  2500 //По 1250 на половину DMA буфера (ровно 2.5 мс при 500 кГц, ровно 10 буферов на такт 40 Гц Howl)
#define SAMPLES_PER_HALF_BUFFER (OUTPUTSAMPLES / 2) //1250

enum GeneratorMode : uint8_t {
	GEN_MODE_CONTINUOUS = 0, //Классический DDS (Несущая + AM/FM)
	GEN_MODE_WAVELET    = 1, //Вейвлеты (пачки несущей с огибающей Ханна + паузы)
	GEN_MODE_MULTIPULSE = 2  //Двухфазные симметричные импульсы (MultiPulse TENS)
};

#include "fatfs.h"

extern uint8_t retSD; /* Return value for SD */
extern FRESULT fr;

extern uint16_t OUTPUT[OUTPUTSAMPLES] RAM_16;

extern FIL SDFile1 CCMRAM;

#pragma pack(push,1)
typedef struct {

	uint8_t CH;               //Номер канала. Не удалять: держит buffer_fm на границе 4 байт (копируется через uint32_t*)

	uint8_t CH_EN;
	uint8_t AM_EN;
	uint8_t FM_EN;

	uint16_t Carrier_fr;       //Частота нечущей
	char Carrier_mod[28];      //Имя файла

	float AM_fr;              //Часта модуляции
	char AM_mod[28];          //Имя файла

	uint16_t FM_Base;
	float    FM_Dev;
	char  FM_mod[28];       //Имя файла
	float FM_mod_fr;        //Часта модуляции

	//Буфферы
	uint16_t buffer_carrier[SAMPLES];
	uint16_t buffer_am[AM_MODSAMPLES];
	float    buffer_fm[FM_MODSAMPLES];

	uint32_t rC;		     //Частота несущей 1
	uint32_t angle_cr;

	uint32_t rAM;
	uint32_t angle_m;

	uint32_t rFM;
	uint32_t angle_fm;

	float Volume; //Громкость 0..1
	float Volume_Script; //Громкость из скрипта 0..1 (множитель)

	//Аккумуляторы
	uint32_t phase_accumulator_carrier;
	uint32_t phase_accumulator_mod;
	uint32_t phase_accumulator_fm;

	//Новые режимы стимуляции (Wavelet, MultiPulse)
	uint8_t  Mode;              //GeneratorMode: 0=DDS, 1=Wavelet, 2=MultiPulse
	float    Wavelet_Pulse_fr;  //Частота следования пачек (Гц, 1..100)
	uint8_t  Wavelet_Cycles;    //Периодов несущей в пачке (1..20)
	uint8_t  Wavelet_Fade;      //Процент сглаживания фронтов пачки (0..100)
	float    Multi_Pulse_fr;    //Частота двухфазных импульсов (Гц, 1..200)
	uint16_t Multi_Pulse_us;    //Длительность полупериода в мкс (50..1000)
	uint16_t Multi_Intra_us;    //Пауза между полуволнами в мкс (0..200)
	uint8_t  Multi_Count;       //Количество импульсов в пачке (1..10)

} _structure_ch;
#pragma pack(pop)

#define VOLUME_DEFAULT  65 //Если в ch1.ini нет громкости. Такой уровень был зашит раньше

//Цифровой ФНЧ на выходе
#define SAMPLE_RATE_HZ  500000.0F //TIM6: 108 МГц / (215 + 1) = ровно 500.000 кГц
#define FILTER_FR_MIN   200
#define FILTER_FR_MAX   100000
#define FILTER_SECTIONS 2                       //2 секции = 4 порядок

enum { FILTER_BUTTERWORTH, FILTER_BESSEL };

typedef struct {
	uint8_t  EN;
	uint8_t  Type;  //FILTER_BUTTERWORTH, FILTER_BESSEL
	uint8_t  Order; //2, 4
	uint32_t Fr;    //Частота среза по -3 dB
} _filter_cfg;

//Секция ФНЧ: b1 = 2 * b0, b2 = b0
typedef struct {
	float b0, a1, a2;
} _biquad;

typedef struct {
	uint32_t sections; //0 - фильтр выключен
	_biquad  s[FILTER_SECTIONS];
} _filter_coef;

class GENERATOR {
public:

	float convertHzToR(float hz) {	return hz * (4294967296.0F / SAMPLE_RATE_HZ);	} //при 500 кГц: hz * 8589.934592F
	float convertHzToR_FM(float hz) { return hz * (4294967296.0F / SAMPLE_RATE_HZ); }
	float convertRToHz(float r) {	return r * (SAMPLE_RATE_HZ / 4294967296.0F);	}

    //dma_dac_it.c

    //Generator.c
	void Create_Carrier(_structure_ch *_CH);
	void Create_AM_Modulation(_structure_ch *_CH);
	void Create_FM_Modulation(_structure_ch *_CH);

	void Create_Carrier1(void)	{  Create_Carrier(&CH1);  }

	void Create_AM_Modulation1(void)  {  Create_AM_Modulation(&CH1);  }

	void Create_FM_Modulation1(void) {  Create_FM_Modulation(&CH1);  }

	void Refresh_FM_Modulation(_structure_ch *_CH); //Для Base Dev без чтения с диска

	void Refresh_FM_Modulation1(void)  {  Refresh_FM_Modulation(&CH1); }


	void RAW_to_Structure(_structure_ch *_CH);
	void Structure_to_RAW(_structure_ch *_CH);
	void read_save_file_to_structure_ch(void); //Чтение файла сохранения и запись его в структуру

	void Init(void);
	void syg_update(int Buffer_Current);


	void read_save_ini_to_structure_ch(void);

	void Filter_Apply(void);  //Пересчитать коэффициенты из filter_cfg
	void Filter_Load(void);   //Прочитать /Config/filter.ini (если нет: фильтр выключен) и применить
	bool Filter_Save(void);   //Записать /Config/filter.ini, на время записи выход замирает

	bool Config_Save(void);   //Записать настройки CH1 в /Config/ch1.ini и фильтр
	bool Config_Load(void);   //Прочитать /Config/ch1.ini и загрузить файлы сигналов, false если файла нет
	bool Save_File(const char *path, const char *text, int len); //Записать текст в файл в /Config

	void pause(void){
		HAL_TIM_Base_Stop(&htim6);
	}

	void start(void){
	  HAL_TIM_Base_Start(&htim6);
	}

	//Громкость в процентах, 100 это весь размах ЦАП
	void Volume_Set(int percent) {
		if (percent < 0) percent = 0;
		if (percent > 100) percent = 100;
		CH1.Volume = percent / 100.0F;
	}

	int Volume_Get(void) {
		return (int) (CH1.Volume * 100.0F + 0.5F);
	}

    //Сброс фазы и аккумулятора
	void syg_reset_phase(void) {
		CH1.phase_accumulator_carrier = 0;
		CH1.phase_accumulator_mod = 0;
		CH1.phase_accumulator_fm = 0;
	}

	void Reset_Stim_States(void) {
		wavelet_stage = 0;
		wavelet_counter = 0;
		wavelet_rand_seed = 0x12345678U;
		mp_stage = 0;
		mp_counter = 0;
		mp_pulses_left = 0;
		howl_dma_chunk = 0;
	}

	void Mode_Set(GeneratorMode mode) {
		CH1.Mode = (uint8_t)mode;
		Reset_Stim_States();
	}

	_structure_ch CH1; //Используется для чтения настроек из файла save.dat

	UINT buffer_temp[1024+16]; //Сохраняются данные файлов

	uint16_t cacheFM [1024]; //Кеш для перенастройки Base Dev без чтения из диска

	int32_t DMA_Buffer_Current;

	_filter_cfg  filter_cfg;
	_filter_coef filter_coef[2];        //Основной поток пишет в неактивный набор, прерывание читает активный
	volatile uint8_t filter_active;
	float filter_state[FILTER_SECTIONS][4]; //x1 x2 y1 y2 каждой секции

	//Автоматы состояний стимуляции
	uint8_t  wavelet_stage;
	uint32_t wavelet_counter;
	uint32_t wavelet_rand_seed;

	uint8_t  mp_stage;
	uint32_t mp_counter;
	uint8_t  mp_pulses_left;
	uint8_t  howl_dma_chunk; //Счётчик полубуферов DMA (0..9) для такта Howl (40 Гц = 25 мс)

};

#endif

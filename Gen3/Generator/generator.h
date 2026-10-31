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
#define OUTPUTSAMPLES  4096 //По 2048 на половину DMA буфера (16 бит, секция .out 8K)

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

	//Аккумуляторы
	uint32_t phase_accumulator_carrier;
	uint32_t phase_accumulator_mod;
	uint32_t phase_accumulator_fm;

} _structure_ch;
#pragma pack(pop)

//Цифровой ФНЧ на выходе
#define SAMPLE_RATE_HZ  (108000000.0F / 217.0F) //TIM6: 108 МГц / (216 + 1)
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

	float convertHzToR(float hz) {	hz = hz * 16384.0F / 3.798F * 2.0F; 	return hz;	}
	float convertHzToR_FM(float hz) { hz = hz * 16384.0F;	return hz; }
	float convertRToHz(float hz) {	hz = hz / 16384.0F * 3.798F / 2.0F; 	return hz;	}

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

	void pause(void){
		HAL_TIM_Base_Stop(&htim6);
	}

	void start(void){
	  HAL_TIM_Base_Start(&htim6);
	}

    //Сброс фазы и аккумулятора
	void syg_reset_phase(void) {
		CH1.phase_accumulator_carrier = 0;
		CH1.phase_accumulator_mod = 0;
		CH1.phase_accumulator_fm = 0;
	}

	_structure_ch CH1; //Используется для чтения настроек из файла save.dat

	UINT buffer_temp[1024+16]; //Сохраняются данные файлов

	uint16_t cacheFM [1024]; //Кеш для перенастройки Base Dev без чтения из диска

	int32_t DMA_Buffer_Current;

	_filter_cfg  filter_cfg;
	_filter_coef filter_coef[2];        //Основной поток пишет в неактивный набор, прерывание читает активный
	volatile uint8_t filter_active;
	float filter_state[FILTER_SECTIONS][4]; //x1 x2 y1 y2 каждой секции

};

#endif

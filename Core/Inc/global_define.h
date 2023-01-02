#ifndef __GLOBAL_DEFINE_H
#define __GLOBAL_DEFINE_H

//┌───────────────────────────┬─────────────────────────────────────┐
   #define USE_GEN          //│                                     │
//#define USE_CLI           //│ Если нужно использовать CLI         │
//#define USE_CLI_GEN       //│                                     │
//#define USE_CLI_LCD       //│                                     │
///#define USE_NOTSAVE_FONT //│ Использование не безопасных фонтов  │
//────────────────────────────┴─────────────────────────────────────┘

//┌───────┬──────────────────────────────────────────────────────────────────────────────────────┐
//│ COLOR │                                                                                      │
//└───────┴───────────────────────────────────────────────┬──────────────────────────────────────┤
    #define COLOR_TEXT_DEFAULT   RGB565(229, 229, 229)  //│ Цвет текста по умолчанию        100  │
	#define COLOR_TEXT_SELECT    RGB565(16, 16, 16)     //│ Цвет текста выбранной строки    101  │
	#define COLOR_TEXT_DISABLE   RGB565(128, 128, 128)  //│ Текст отключенного текста       102  │
	#define COLOR_RECTAGLE       RGB565(255, 140, 50)   //│ Текст выделенной строки         103  │
	#define COLOR_BACKGROUND     RGB565(0, 0, 7)        //│ tft.RGB565(0, 7, 43)   // Фон   104  │
//────────────────────────────────────────────────────────┴──────────────────────────────────────┘

//┌─────────────────────────────────────────┐
	//Для картинки генератора индексы строк │
	#define INDEX_CH1_EN      0           //│
	#define INDEX_CH1_FR      1           //│
	#define INDEX_CH1_CR      2           //│
	#define INDEX_CH1_AM_EN   3 		  //│
	#define INDEX_CH1_AM_MOD  4 		  //│
	#define INDEX_CH1_AM_FR   5 		  //│
	#define INDEX_CH1_FM_EN   6 	      //│
	#define INDEX_CH1_FM_BASE 7 	      //│
	#define INDEX_CH1_FM_DEV  8 	      //│
	#define INDEX_CH1_FM_MOD  9 	      //│
	#define INDEX_CH1_FM_FR   10 	      //│
 	 	 	 	 	 	 	 	 	      //│
	#define INDEX_CH2_EN      11 	 	  //│
	#define INDEX_CH2_FR      12 	 	  //│
	#define INDEX_CH2_CR      13 	 	  //│
	#define INDEX_CH2_AM_EN   14 	 	  //│
	#define INDEX_CH2_AM_MOD  15 	 	  //│
	#define INDEX_CH2_AM_FR   16 	 	  //│
	#define INDEX_CH2_FM_EN   17 	 	  //│
	#define INDEX_CH2_FM_BASE 18 	 	  //│
	#define INDEX_CH2_FM_DEV  19 	      //│
	#define INDEX_CH2_FM_MOD  20 	      //│
	#define INDEX_CH2_FM_FR   21 	      //│
									      //│
	#define NUM_ITEM_GENERETOR   22       //│
//└─────────────────────────────────────────┘

//┌─────────┬─────────────────────────┐
//│ include │                         │
//└─────────┴─────────────────────────┤
	#include "stm32f4xx_hal.h"	    //│
	#include "stm32f4xx.h"          //│
	#include "stm32f4xx_it.h"  	    //│
	#include "crc.h"                //│
	#include "dma.h"      	        //│
	#include "i2c.h"      	        //│
	#include "spi.h"                //│
	#include "tim.h"       	  	    //│
	#include "gpio.h"               //│
	#include "main.h"          		//│

	#include "GyverButton.h"        //│
	#include "TFT.h"            	//│
    #include "bitmap.h"             //│
    #include <TFT_color.h>
	#include "Generator.h"          //│
	#include "HiSpeedDWT.h"         //│
	#include "LOG.h"                //│
	#include "TFT_gif.h"            //│
	#include "scripting.h"          //│
	#include "mString.h"            //│

    #include "logUART.h"

//────────────────────────────────────┘
#include "button.h"


//┌─────────┬──────────────────────┐
//│ extern  │                      │
//└─────────┴──────────────────────┤
	extern TFT       tft CCMRAM;  //│
	extern GENERATOR Gen CCMRAM;//│
//─────────────────────────────────┘


//* ┌ ┐ └ ┘├ ┤ ┬ ┴ ┼ ─ │
//┌─────────┬──────────────────────────────────────────┐
//│ typedef │                                          │
//└─────────┴──────────────────────────────────────────┤
	typedef char __attribute__((aligned (4))) char4; //│
//──────────────────────────────┬──────────────────────┤
	typedef struct            //│       Энкодер        │
	{                         //└──────────────────────┤
		uint8_t Left;                                //│
		uint8_t Right;                               //│
	}Encoder_typedef;                                //│
//─────────────────────────────────────────────────────┘

//───────────────────────────────────────────────────────────────────────────────────┤
//Создается для каждого файла в папке                                              //│
typedef struct                                                                     //│
{                                                                                  //│
 char  Name[16];    //Имя файла                                                    //│
 uint8_t  maxFileCount = 0;                                                        //│
} Dir_File_Info_Array;                                                             //│
//───────────────────────────────────────────────────────────────────────────────────┘

//┌─────────┬────────────────────────────────┐
//│ extern  │                                │
//└─────────┴────────────────────────────────┤

extern Scripting script;

extern HiSpeedDWT TimerDWT;
extern HiSpeedDWT TimerT5;

extern Encoder_typedef Encoder;
extern TIM_HandleTypeDef htim2;

extern GButton KEY;
extern button_typedef button;

extern classLog rtt;

extern uint8_t  u8Volume;
extern float    fVolume;

extern uTFT_LCD_t LCD_0;

extern Dir_File_Info_Array Dir_File_Info[] CCMRAM; //Массив всех файлов в папке

//Сформировать и отправить текущие настройки
extern void sendStructCHtoHost(unsigned int ch);

extern uint32_t DMA_Run_Tick;
extern uint32_t DMA_Run_us;
extern uint32_t Other_Run_Tick;
extern uint32_t Other_Run_us;
extern uint32_t DMA_Sum_Tick;
extern uint32_t DMA_Sum_us;
extern float    DMA_zagruzka;

#endif

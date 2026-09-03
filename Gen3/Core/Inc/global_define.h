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

//┌──────────────┬──────────────────────────────────────────────────────────┐
//│ ID Ресурсов  │                                                          │
//└──────────────┴──────────────────────────────────────────────────────────┤
    #define Roboto_Medium_en_ru_30 (uint8_t *)(getResAdressFontID(0))     //│
    #define Roboto_Medium_en_ru_18 (uint8_t *)(getResAdressFontID(1))     //│
//──────────────────────────────────────────────────────────────────────────┤
    #define id_ON_DISABLE  2                                              //│
    #define id_OFF_DISABLE 3                                              //│
    #define id_ON          4                                              //│
    #define id_OFF         5                                              //│
//──────────────────────────────────────────────────────────────────────────┘

#endif

#include "global_define.h"
#include "menu_typedef.h"
#include "generator.h"
#include "global_typedef.h"
#include "Button.h"

menu_typedef menu_generator;
menu_typedef menu_setting;

item_typedef   item_generator[NUM_ITEM_GENERETOR] CCMRAM;
item_typedef   item_setting[8] CCMRAM;

//GButton KEY(ENTER_GPIO_Port, ENTER_Pin);
button_typedef button;

uint8_t  u8Volume;
float    fVolume;

//Dir_File_Info_Array Dir_File_Info[32] CCMRAM; //Массив всех файлов в папке

TFT tft CCMRAM;

GENERATOR Gen CCMRAM;

//GFXFONT gfxfont CCMRAM;

HiSpeedDWT TimerDWT;
HiSpeedDWT TimerT5;

classLog rtt;

//Статистика работы DMA1_5
uint32_t DMA_Run_Tick;
uint32_t DMA_Run_us;
uint32_t Other_Run_Tick;
uint32_t Other_Run_us;
uint32_t DMA_Sum_Tick;
uint32_t DMA_Sum_us;
float    DMA_zagruzka;




//ST7789 135 240 16 bit
uint16_t LCD_Buffer16[240 * 240];
//uint16_t LCD_Buffer16_0[1];



uTFT_LCD_t LCD_0 = {
		240,     // Ширина экрана
		240,                  // Высота экрана
		ST7789,               // Драйвер
		16,                    // bit
		&hspi1,               // Spi
		NULL,	              // I2C
		&LCD_Buffer16[0],      // 16 бит буффер
		NULL,                 // !16 бит буффер
		NULL,                 // Палитра
		0,                    // Смещение по X
		0,                    // Смещение по Y
		0,                    // CS PORT
		0,                    // CS PIN
		A0_GPIO_Port,         // DC PORT
		A0_Pin,               // DC PIN
		RESET_GPIO_Port,      // RESET PORT
		RESET_Pin,	          // RESET PIN
		0, {                     //MADCTL
		0,   //MY
				0,   //MX
				0,   //MV
				0,   //ML
				0,   //RGB
				0,   //MH
		}, {                     //Смещение но X Y
		240, 240, 0,   //DX_0
				0,   //DY_0
				240, 240, 0,   //DX_90
				0,   //DY_90
				240, 240, 0,	 //DX_180
				0,   //DY_180
				240, 240, 0,   //DX_270
				0,   //DY_270
		} };










#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "main.h"

#include "TFT_gif.h"

void PAGE_BT_Audio(void) {

 Gen.pause();	                                                    //Отключить генератор
 HAL_GPIO_WritePin(BT_Audio_GPIO_Port, BT_Audio_Pin, GPIO_PIN_SET); //Включить реле

 //┌─────┬──────────────────────┐
 //│ Gif │                      │
 //└─────┴──────────────────────┤
 TFT_gif gif;                 //│
 gif.init(&tft);              //│
 gif.setName((char *)"BT");   //│
 gif.setDelay(0);             //│
                              //│
 TFT_gif gif2;                //│
 gif2.init(&tft);             //│
 gif2.setName((char *)"Q1");  //│
 gif2.setDelay(0);            //│
 gif2.setXY(120, 120);        //│
 //─────────────────────────────┘

 tft.Fill16(BLUE);
 tft.Font_Smooth_Load( Roboto_Medium_en_ru_30 );
 tft.Font_Smooth_drawStr(40, 100, "BT Audio");
 tft.Font_Smooth_drawStr(18, 200, "Press to Back");

 tft.ST7789_Update();
 //gfxfont.Puts(10, 22, "USB Audio");

 while (1) {
   //──────────────────────────────────────────────────────┬───────────────────┬─────────────────────┐
   button.tick();                                           //│ Выход при нажатии │                     │
   if (button.isClick()){                                   //└───────────────────┤                     │
		HAL_GPIO_WritePin(BT_Audio_GPIO_Port, BT_Audio_Pin, GPIO_PIN_RESET); //│ Отключаем реле      │
		Gen.start();                                                         //│ Запускаем генератор │
    	return;                                                              //│                     │
   }                                                                         //│                     │
   //──────────────────────────────────────────────────────────────────────────┴─────────────────────┘
 }

}

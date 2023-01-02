#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "main.h"
#include "bitmap.h"
#include "FontSmooth.h"

#include "Generator.h"
extern GENERATOR Gen;

#include "Resource/resource.h"

#include "button.h"
extern button_typedef button;

void PAGE_BT_Audio(void) {

 Gen.pause();	                                                    //Отключить генератор
 HAL_GPIO_WritePin(BT_Audio_GPIO_Port, BT_Audio_Pin, GPIO_PIN_SET); //Включить реле

// //┌─────┬──────────────────────┐
// //│ Gif │                      │
// //└─────┴──────────────────────┤
// TFT_gif gif;                 //│
// gif.init(&tft);              //│
// gif.setName((char *)"BT");   //│
// gif.setDelay(0);             //│
//                              //│
// TFT_gif gif2;                //│
// gif2.init(&tft);             //│
// gif2.setName((char *)"Q1");  //│
// gif2.setDelay(0);            //│
// gif2.setXY(120, 120);        //│
// //─────────────────────────────┘


 BMPFromFile(&tft, 0, 0, (char*)"/Config/bt2.bmp");

 //tft.Fill16(BLUE);
 Font_Smooth_Load( Roboto_Medium_en_ru_30 );
 //tft.Font_Smooth_drawStr(40, 100, "BT Audio");
 tft.uTFT.Color = 0xFFFF;
 Font_Smooth_drawStr(&tft, 28, 195, "Press to Back");

 tft.driver.ST7789_Update();
 //gfxfont.Puts(10, 22, "USB Audio");

 while (1) {
   //──────────────────────────────────────────────────────┬───────────────────┬─────────────────────┐
   button.tick();                                        //│ Выход при нажатии │                     │
   if (button.isClick()){                                //└───────────────────┤                     │
		HAL_GPIO_WritePin(BT_Audio_GPIO_Port, BT_Audio_Pin, GPIO_PIN_RESET); //│ Отключаем реле      │
		Gen.start();                                                         //│ Запускаем генератор │
    	return;                                                              //│                     │
   }                                                                         //│                     │
   //──────────────────────────────────────────────────────────────────────────┴─────────────────────┘
 }

}

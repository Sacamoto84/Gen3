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

#include "logUART.h"
extern classLog rtt;

void PAGE_BT_Audio(void) {

 Gen.pause();	                                                    //Отключить генератор
 HAL_GPIO_WritePin(BT_Audio_GPIO_Port, BT_Audio_Pin, GPIO_PIN_SET); //Включить реле

 BMPFromFile(&tft, 0, 0, (char*)"/Config/bt2.bmp");

 Font_Smooth_Load( Roboto_Medium_en_ru_30 );
 tft.uTFT.Color = 0xFFFF;
 Font_Smooth_drawStr(&tft, 28, 195, "Press to Back");

 tft.driver.ST7789_Update();

 rtt.info("BT Audio On");

 while (1) {
   //──────────────────────────────────────────────────────┬───────────────────┬─────────────────────┐
   button.tick();                                        //│ Выход при нажатии │                     │
   if (button.isClick()){                                //└───────────────────┤                     │
		HAL_GPIO_WritePin(BT_Audio_GPIO_Port, BT_Audio_Pin, GPIO_PIN_RESET); //│ Отключаем реле      │
		Gen.start();                                                         //│ Запускаем генератор │
		rtt.info("BT Audio Off");
    	return;                                                              //│                     │
   }                                                                         //│                     │
   //──────────────────────────────────────────────────────────────────────────┴─────────────────────┘
 }

}

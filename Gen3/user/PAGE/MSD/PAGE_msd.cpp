#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "ff.h"
#include "fatfs.h"
#include "bitmap.h"
#include "FontSmooth.h"
#include "generator.h"
#include "resource.h"

extern GENERATOR Gen CCMRAM;

#define APPLICATION_ADDRESS    0x08004000

static void Go_To_User_App(void)
{
    uint32_t app_jump_address;

    typedef void(*pFunction)(void);
    pFunction Jump_To_Application;

     __disable_irq();

    app_jump_address = *( uint32_t*) (APPLICATION_ADDRESS + 4);
    Jump_To_Application = (pFunction)app_jump_address;
	SCB->VTOR = APPLICATION_ADDRESS;
    __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
    Jump_To_Application();
}

void PAGE_MSD(void)
{
	Gen.pause();

	tft.Fill16(BLUE);

	BMPFromFile(&tft, 0, 0, (char*)"/Config/MSD2.bmp");

	Font_Smooth_Load(Roboto_Medium_en_ru_30);
	Font_Smooth_drawStr(&tft, 18, 195, "Press to Reset");

	tft.driver.ST7789_Update();
    //HAL_Delay(1000);

    __disable_irq();
    for(int i = 0; i< 81; i++)
	  NVIC_DisableIRQ((IRQn_Type)i);

	Go_To_User_App();
	while (1) {
	}

}


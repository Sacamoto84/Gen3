#include "TFT_convert.h"
#include "Page.h"
#include "global_typedef.h"
#include "global_define.h"
#include "stdio.h"
#include "mString.h"
#include "FontSmooth.h"
#include "fatfs.h"
#include "logUART.h"
#include "Generator.h"
#include "TFT_color.h"
#include "scripting.h"
#include "button.h"

extern classLog  rtt;
extern TFT       tft;
extern GENERATOR Gen;
extern Scripting script;
extern button_typedef button;
extern Encoder_typedef Encoder;

#define APPLICATION_ADDRESS    0x08040000

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

typedef struct {
	char text[20];
} item_files;

void PAGE_Player(void) {
	Gen.pause();

	tft.Fill16(BLACK);

	tft.driver.ST7789_Update();

    __disable_irq();
    for(int i = 0; i< 81; i++)
	  NVIC_DisableIRQ((IRQn_Type)i);

    HAL_DeInit();

	Go_To_User_App();
	while (1) {
	}

}





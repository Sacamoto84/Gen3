#include <Page.h>
#include "main.h"

#include "stdio.h"

#define NUM_OF(x) (sizeof(x)/sizeof(x[0]))

#include "global_define.h"
#include "TimesNRCyr16.h"
#include "logUART.h"
#include "button.h"
extern button_typedef button;

extern int U3_DMA_TX_Complete;

void illegal_instruction_execution(void) {
	void (*func_name) (void);
	func_name = (void (*)())0x77777777;
	func_name();
}

void setup(void) {

	TimerDWT.init();
	TimerT5.init(&htim5);

	rtt.init(&huart3);
	rtt.dma_completed  = &U3_DMA_TX_Complete;
	*rtt.dma_completed = 1;
	rtt.useDMA = true;

	button.init(ENTER_GPIO_Port, ENTER_Pin);

	HAL_Delay(1000);

	__HAL_SPI_DISABLE(&hspi1);
	SPI1->CR1 |= (0x1UL << (5U));
	__HAL_SPI_ENABLE(&hspi1);

	tft.init(&LCD_0);

    __HAL_SPI_DISABLE(&hspi1);
    SPI1->CR1 &= ~(0x1UL << (5U));
    __HAL_SPI_ENABLE(&hspi1);

	tft.ST77XX_Update_MADCTL();

	//tft.video_play((char*)"intro.raw", 20);
	//tft.video_play((char*)"intro.raw", 20);

	script.G = &Gen; //Привязать генератор
#ifdef USE_GEN
	//script.Unit5Load();
	Gen.CH1.CH = 0;
	Gen.CH2.CH = 1;
	script.load((char *)"load_def");
	script.start();
	Gen.Init();//Инициализация генератора
#endif

	gfxfont.init(&tft);

	tft.setResStartAdress(0x08020000); //Установим начало ресурсов

	//gfxfont.setFont(&FreeMonoBold12pt7b);
	//gfxfont.setFont(&FreeSansBold12pt7b);

	//gfxfont.setFont(&DejaVu_Sans_Mono_Bold_24);

	//////gfxfont.setFont(&FreeMono14pt8b);

	//gfxfont.setFont(&CourierCyr16pt8b);

	//gfxfont.setFont(&CourierCyr14pt8b);
	//gfxfont.setFont(&Cooper18pt8b);

	tft.SetFontColor(4, 14);

	//tft.Font_Smooth_Load(&_acJetBrainsMono_Bold_en_ru_32[0]);

	//Инициализация структур для экранов
	PAGE_init_struct_generator(); //Картинка генератора
	PAGE_init_struct_setting();   //Настройка

	//tft.Font_Smooth_Load(&_acRoboto_Medium_en_ru_30[0]);
	//tft.Font_Smooth_Load(&_acJetBrainsMono_Medium_en_ru_30[0]);
	//HardFault_Handler();

	//illegal_instruction_execution();

	//Gen.pause();

	//HAL_Delay(3000);

	//script.load((char *)"test1");

	PAGE_Generator();

	//tft.Font_Smooth_Load(&_acJetBrainsMono_Medium_en_ru_30[0]);
	//PAGE_Palitra0_15();

	//PAGE_Menu( page_setting,  &page_menu[0], NUM_OF(page_menu));

	//tft.GotoXY(0, 20);
	//gfxfont.set_delta_x(0);
	//gfxfont.drawChar('T');
	//gfxfont.Puts((char*)"Hello12345");

	//tft.ST7789_Update();

}


void loop() {

	while (1) {
		//SEGGER_SYSVIEW_Warn("loop\n");
		//HAL_Delay(10000);
		//tft.ST7789_UpdateDMA4bit();

		//BLE_Task();

		//LED.run();

		//	i = (uint32_t) HAL_ADC_GetValue(&hadc1);
		//	Vbus = i / 4096.0F * 3.3F * 3.95F/2.65F ;

		//	__WFI();
		//
	}
}

extern "C" void main_cpp(void) {
	setup();
	loop();
}

extern "C" void DMA1_Stream5_IRQHandler(void)
{

#ifdef USE_GEN

	//Other_Run_Tick = TimerT5.End(); //Получаем время работы без тиков
	DMA_Sum_Tick   = Other_Run_Tick + DMA_Run_Tick;
	DMA_zagruzka = (float)DMA_Run_Tick / (float)DMA_Sum_Tick * 100.0F;

	//DMA_Run_us   = DMA_Run_Tick / TimerT5.tickToUs;
	//Other_Run_us = Other_Run_Tick / TimerT5.tickToUs;
	//DMA_Sum_us   = DMA_Sum_Tick / TimerT5.tickToUs;

	//TimerT5.Start();

	if (DMA1_Stream5->CR & DMA_SxCR_CT)
	{
		Gen.DMA_Buffer_Current = 0;
	    Gen.syg_update(0);
	}
	else
	{
		Gen.DMA_Buffer_Current = 1;
	    Gen.syg_update(1);
	}
	HAL_DMA_IRQHandler(&hdma_dac1);

	//DMA_Run_Tick = TimerT5.End(); //Получили тики работы
	//TimerT5.Start();

#endif

}



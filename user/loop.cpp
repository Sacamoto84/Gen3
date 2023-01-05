#include <Page.h>
#include "main.h"
#include "stdio.h"
#include "tim.h"

#define NUM_OF(x) (sizeof(x)/sizeof(x[0]))

#include "global_define.h"
#include "logUART.h"
#include "button.h"
extern button_typedef button;

#include "HiSpeedDWT.h"
extern HiSpeedDWT TimerDWT;
extern HiSpeedDWT TimerT5;

extern TFT tft;

#include "generator.h"
extern GENERATOR Gen;

#include "scripting.h"
extern Scripting script;

extern uint32_t DMA_Sum_Tick;
extern uint32_t DMA_Sum_us;
extern float    DMA_zagruzka;
extern uint32_t DMA_Run_Tick;
extern uint32_t Other_Run_Tick;

extern uTFT_LCD_t LCD_0;

#include "resource.h"

void illegal_instruction_execution(void) {
	void (*func_name) (void);
	func_name = (void (*)())0x77777777;
	func_name();
}

int * U3_DMA_TX_Complete;

void setup(void) {

	TimerDWT.init();
	TimerT5.init(&htim5);

	rtt.init(&huart3);
	rtt.dma_completed  = U3_DMA_TX_Complete;
	*rtt.dma_completed = 1;
	rtt.useDMA = false;

	HAL_Delay(1000);

	__HAL_SPI_DISABLE(&hspi1);
	SPI1->CR1 |= (0x1UL << (5U));
	__HAL_SPI_ENABLE(&hspi1);

	tft.init(&LCD_0);

    __HAL_SPI_DISABLE(&hspi1);
    SPI1->CR1 &= ~(0x1UL << (5U));
    __HAL_SPI_ENABLE(&hspi1);

	tft.driver.ST77XX_Update_MADCTL();

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

	setResStartAdress(0x08020000); //Установим начало ресурсов
	tft.SetFontColor(4, 14);

	//Инициализация структур для экранов
	PAGE_init_struct_generator(); //Картинка генератора
	PAGE_init_struct_setting();   //Настройка

	PAGE_Generator();

}


void loop() {

	while (1) {

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
	//DMA_Sum_Tick   = Other_Run_Tick + DMA_Run_Tick;
	//DMA_zagruzka = (float)DMA_Run_Tick / (float)DMA_Sum_Tick * 100.0F;

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



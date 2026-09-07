#include "main.h"
#include "stdio.h"
#include "Resource/resource.h"
#include "global.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "fatfs.h"

#include "mp3dec.h"

#include "mString.h"

#include "timber.h"

extern void PAGE_init_palitra(void); //Инициализация палитры

void stopError(void);

TFT tft;
GFXFONT gfxfont;

void loop(){while (1){
	task_HMI();
	timber.info ("Свободно памяти %d", xPortGetFreeHeapSize());
	osDelay(20000);
}}


HAL_SD_CardInfoTypeDef Card_Info;

void setup(void) {

	PAGE_init_palitra();

	//rtt.init(&huart3);

	timber.init();

	timber.clear();

	timber.setBold();
	timber.info("MP3 V40");
	timber.reset();

	TimerDWT.init();
	TimerT5.init(&htim5);

	__HAL_SPI_DISABLE(&hspi1);
	SPI1->CR1 |= (0x1UL << (5U));
	__HAL_SPI_ENABLE(&hspi1);

	tft.init(&LCD_0);

    __HAL_SPI_DISABLE(&hspi1);
    SPI1->CR1 &= ~(0x1UL << (5U));
    __HAL_SPI_ENABLE(&hspi1);

	setResStartAdress(0x08020000); //Установим начало ресурсов

	gfxfont.init(&tft);

	//FreeMonoBold12pt8b
	//gfxfont.setFont(&FreeSerif12pt8b);
	//gfxfont.setFont(&Roboto_Mono_24);
	//gfxfont.setFont(&FreeMono12pt8b);
	//gfxfont.setFont(&FreeMonoBold12pt8b);
//	gfxfont.setFont(&CourierCyr12pt8b);

	gfxfont.setFont(&JetBrainsMono_VariableFont_wght12pt8b);

	//tft.SetFontColor(4, 14);
	tft.SetFontColor(14, 15);
    //tft.setColorToPalete(0,tft.RGB565(255,4,6));
    //tft.setColorToPalete(1,tft.RGB565(1,56,33));

	tft.Fill(0);
	tft.RectangleFilled(10, 10, 20, 20, 1);
	tft.driver.Update();

	timber.info("Start testing SDCARD");
	timber.color(87);
	BSP_SD_GetCardInfo(&Card_Info);
	timber.print("Block Size      -> 0x%x\n", (int)Card_Info.BlockSize);
	timber.print("Capacity blocks -> 0x%x(%uGB)\n", (int)Card_Info.BlockNbr,(int)((((float)Card_Info.BlockNbr/1000)*(float)Card_Info.BlockSize/1000000)+0.5));
	timber.reset();
	timber.warning("Монтирования диска") ; // ("монтирования диска\n");

	// смонтировать диск
	FRESULT result = f_mount(&SDFatFS, SDPath, 1); //Mount MicroSd
	if (result != FR_OK)
	{
		timber.error("Ошибка монтирования диска %d", result);
	  stopError();
	}
	else
		timber.successful("Монтирование диска: успешно");

	//Открыть файл 'GIFtoBIN.exe'
	result = f_open(&SDFile, "GIFtoBIN.exe", FA_READ);
	if (result == FR_OK)
	{
		timber.successful("GIFtoBIN.exe..OK");
	  f_close(&SDFile);
	}
	else
		timber.error("GIFtoBIN.exe..Ошибка открытия файла");

	globalPach = "/music";
	readDir((char *) globalPach.c_str(), &list_mp3);
	list_mp3.root =  false;
	addTreeDot(&list_mp3);

	timber.info ("Свободно памяти Всего %d", xPortGetFreeHeapSize());

}

extern "C" void main_cpp(void) {
	setup();
	loop();
}







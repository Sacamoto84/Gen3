#ifndef TASKCLI_H_
#define TASKCLI_H_

#include "main.h"

#include <SimpleCLI.h>
#include <string.h>
#include "SEGGER_RTT.h"
#include "generator.h"
#include "usart.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <stdio.h> /* for printf */
#include <string.h> /* for printf */
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "usart.h"
#include "stdlib.h"
#include "string.h"
#include "fatfs.h"

#include "BLE_Commands.h"

#include "taskCLI_Generator.h"
#include "taskCLI_LCD.h"

#include "global_define.h"

#include "TFT.h"
#include "TFT_SPI.h"

extern GENERATOR Gen;

#define Debug_Socket 99

int StrToInt(String s);
float StrToFloat(String s);

extern TFT tft CCMRAM;
extern int U3_DMA_TX_Complete;

/* Блок LCD */
extern Command LCD_Invers;  //Команда инверсии экрана 0 1
extern Command LCD_Gamma_P; //Команда положительной гаммы // Готово
extern Command LCD_Gamma_N; //Команда положительной гаммы // Готово
extern Command LCD_Block;   //Блокировка обновления экрана
extern Command LCD_SoftwareReset;   //Програмный брос экрана
extern Command LCD_Gamma_BMP;       //Показ картинки GAMMA

extern Command LCD_GAMSET; //GAMSET (Gamma Set)

extern Command LCD_Frame_Rate_Control; //B1 FRMCTR1 (Frame Rate Control)

extern Command LCD_VMCTR1;
extern Command LCD_Init;

extern Command LCD_Init_Custom;

//Команды чтение настроек
extern Command LCD_Read_backlight;
extern Command LCD_Read_fps;
extern Command LCD_Read_gammaset;
extern Command LCD_Read_vcom1;
extern Command LCD_Read_gp;
extern Command LCD_Read_gn;

//Команды сохранения настроек
extern Command LCD_Save_backlight;
extern Command LCD_Save_fps;
extern Command LCD_Save_gammaset;
extern Command LCD_Save_vcom1;
extern Command LCD_Save_gp;
extern Command LCD_Save_gn;

/* Блок Generator */
extern Command comandCH1_EN;
extern Command comandCH1_CARRIER_FR;
extern Command comandCH1_CARRIER_MOD;

extern Command comandCH1_AM_EN;
extern Command comandCH1_AM_MOD;
extern Command comandCH1_AM_FR;

extern Command comandCH1_FM_EN;
extern Command comandCH1_FM_BASE;
extern Command comandCH1_FM_DEV;
extern Command comandCH1_FM_FR;
extern Command comandCH1_FM_MOD;

extern Command File_Read_Dir; //Получить список файлов в папке
extern Command Command_Read_File;

/* Системное */
extern Command MCU_Reset;//Сброс контроллера
extern Command MCU_Reset;//Сброс контроллера

extern Command RGB_DEF;  //Text по умолчанию
extern Command RGB_TS;   //Text Select
extern Command RGB_TD;   //Текст Disable
extern Command RGB_REC;  //Установить прямоугольник
extern Command RGB_BG;   //Установить фон

#endif /* TASKCLI_H_ */

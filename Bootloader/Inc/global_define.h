#ifndef __GLOBAL_DEFINE_H
#define __GLOBAL_DEFINE_H



#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"

#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_Conf.h"

#include "tim.h"
#include "fonts.h"

#include "uTFT_define.h"
#include "fonts.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include "mysignal.h"

extern char str1[60];

typedef struct
{
	uint8_t Left;
	uint8_t Right;
} Encoder_typedef;

extern Encoder_typedef Encoder_Left;
extern Encoder_typedef Encoder;

//extern TIM_HandleTypeDef htim2;
//extern TIM_HandleTypeDef htim3;
//extern UART_HandleTypeDef huart1;

//Создается для каждого файла в папке
typedef struct
{
	uint8_t Name[30]; //Имя файла
	uint32_t Size; //Размер файла байт
	uint8_t Size_Text[10]; //Размер файла текст
	uint32_t Time;
	uint8_t  Time_text[10];
	uint32_t Visible; //Признак того что нужно поазывать файл
} VC_File_Info_Array;	

//Создается для каждого файла в папке
typedef struct
{
	VC_File_Info_Array File; //Имя файла
	uint32_t Size; //Размер файла байт
	uint32_t Time[20];
	char     Time_text[20];
	uint32_t Visible; //Признак того что нужно поазывать файл
} VC_File_Window_Array;	

/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

extern float Vbus;
extern float Ibus;
	
extern uint16_t ADC3_Data;

//extern USBD_HandleTypeDef hUsbDeviceFS;

extern uint32_t USB_Connent; //Признак того что USB подключени и к карточке не следует обращаться

extern volatile FontDefMicroElectronika_t Font;

////Структура для джойстика
//typedef struct
//{
//	uint8_t Left_Press;
//	uint8_t Left_Release;
//	uint8_t Right_Press;
//	uint8_t Right_Release;
//	uint8_t Up_Press;
//	uint8_t Up_Release;
//	uint8_t Down_Press;
//	uint8_t Down_Release;
//	uint8_t E_Press;
//	uint8_t E_Release;
//} Key_typedef;

//extern Key_typedef Key;

//MESSAGE
#define MESSAGE_USB_CONNECTED uTFT_Message_Create(0);
#define MESSAGE_FILE_ERROR    uTFT_Message_Create(1);

#define  OUT_DAC1   0 
#define  OUT_DAC2   1
	 
//Структура для определения каналов
typedef struct
{
	uint8_t Out_Left;
	uint8_t Out_Right;
} Out_typedef;
	
extern Out_typedef OUT;


void SEGGER_Init(void);
void PAGE_Generator(void);
void ST7789_Update_DMA_Cicle_Off(void);

uint32_t E_isDouble(void);
uint32_t E_isHolded(void);
uint32_t E_isPress(void);
uint32_t E_isSingle(void);

uint32_t F_isPress(void);
uint32_t F_isHolded(void);


void key_clear(void);

#ifdef __cplusplus
}
#endif

#endif

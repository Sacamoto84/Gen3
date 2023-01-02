/*
 * global_extern.h
 *
 *  Created on: 2 янв. 2023 г.
 *      Author: Ivan
 */

#ifndef INC_GLOBAL_EXTERN_H_
#define INC_GLOBAL_EXTERN_H_

//┌─────────┬──────────────────────┐
//│ extern  │                      │
//└─────────┴──────────────────────┤
	extern TFT       tft CCMRAM;  //│
	extern GENERATOR Gen CCMRAM;//│
//─────────────────────────────────┘

//┌─────────┬────────────────────────────────┐
//│ extern  │                                │
//└─────────┴────────────────────────────────┤

extern Scripting script;

extern HiSpeedDWT TimerDWT;
extern HiSpeedDWT TimerT5;

extern Encoder_typedef Encoder;
extern TIM_HandleTypeDef htim2;

//extern GButton KEY;
extern button_typedef button;

extern classLog rtt;

extern uint8_t  u8Volume;
extern float    fVolume;

extern uTFT_LCD_t LCD_0;

extern Dir_File_Info_Array Dir_File_Info[] CCMRAM; //Массив всех файлов в папке

//Сформировать и отправить текущие настройки
extern void sendStructCHtoHost(unsigned int ch);

extern uint32_t DMA_Run_Tick;
extern uint32_t DMA_Run_us;
extern uint32_t Other_Run_Tick;
extern uint32_t Other_Run_us;
extern uint32_t DMA_Sum_Tick;
extern uint32_t DMA_Sum_us;
extern float    DMA_zagruzka;



#endif /* INC_GLOBAL_EXTERN_H_ */

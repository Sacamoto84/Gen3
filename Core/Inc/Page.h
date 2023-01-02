#ifndef __PAGE_PALLETE_H
#define __PAGE_PALLETE_H
	 
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"

#include "TFT.h"
extern TFT tft;

//#include "global_define.h"
//#include "generator.h"

//#include "menu_typedef.h"

//extern GENERATOR Gen CCMRAM;
//extern uint16_t palitra_0[16];

extern void PAGE_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t select, char *str);
extern void PAGE_Button_Pressed(uint16_t x, uint16_t y, uint16_t w, uint16_t h, char *str);
extern void PAGE_Background_Board(void);
//extern void PAGE_Menu(menu_typedef * menu , item_typedef * item, int NUM);
//extern void PAGE_Menu2(menu_typedef * menu , item_typedef * item, int NUM);
extern void PAGE_Video(void);
extern void PAGE_Font(void);

extern void PAGE_BT_Audio(void);
extern void PAGE_Volume(void);
extern void PAGE_Generator();
extern void PAGE_Setting();
extern void PAGE_Script();
extern void PAGE_Player();

//Wifi
extern void PAGE_wifi(void);
extern void PAGE_wifi_switch(void);

extern void PAGE_MSD(void);

extern void PAGE_init_palitra(void); //Инициализация палитры

extern void PAGE_init_struct_generator(void);
extern void PAGE_init_struct_setting(void);


extern void prePageGenerator(void); //Пред функция для картинки генератор
extern void PAGE_generator_CH1_CH_EN_switch(void);
extern void PAGE_generator_CH1_AM_EN_switch(void);
extern void PAGE_generator_CH1_FM_EN_switch(void);

extern void PAGE_generator_CH2_CH_EN_switch(void);
extern void PAGE_generator_CH2_AM_EN_switch(void);
extern void PAGE_generator_CH2_FM_EN_switch(void);

extern void PAGE_generator_encoder_block_switch(void);
extern void PAGE_generator_select_modulation(void);

extern void generator_pre_CH1_FR(uint32_t index);
extern void generator_pre_CH2_FR(uint32_t index);
extern void generator_pre_CH1_AM_FR(uint32_t index);
extern void generator_pre_CH2_AM_FR(uint32_t index);
extern void generator_pre_CH1_FM_BASE(uint32_t index);
extern void generator_pre_CH2_FM_BASE(uint32_t index);
extern void generator_pre_CH1_FM_DEV(uint32_t index);
extern void generator_pre_CH2_FM_DEV(uint32_t index);
extern void generator_pre_CH1_FM_FR(uint32_t index);
extern void generator_pre_CH2_FM_FR(uint32_t index);
extern void generator_pre_CH1_EN(uint32_t index);
extern void generator_pre_CH2_EN(uint32_t index);
extern void generator_pre_CH1_AM_EN(uint32_t index);
extern void generator_pre_CH2_AM_EN(uint32_t index);
extern void generator_pre_CH1_FM_EN(uint32_t index);
extern void generator_pre_CH2_FM_EN(uint32_t index);

extern void setting_save_click(void);
extern void setting_script_stop(void);

#endif

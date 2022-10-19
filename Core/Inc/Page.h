#ifndef __PAGE_PALLETE_H
#define __PAGE_PALLETE_H
	 
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"



#include "TFT.h"

extern TFT tft;

#include "gfxfont.h"
#include "gfx_fonts_includes.h"

extern GFXFONT gfxfont;
extern GFXFONT gfxfontVdd;

#include "global_define.h"
#include "generator.h"

extern GENERATOR Gen CCMRAM;
extern uint16_t palitra_0[16];


void PAGE_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t select, char *str);
void PAGE_Button_Pressed(uint16_t x, uint16_t y, uint16_t w, uint16_t h, char *str);
void PAGE_Background_Board(void);
void PAGE_Menu(menu_typedef * menu , item_typedef * item, int NUM);
void PAGE_Menu2(menu_typedef * menu , item_typedef * item, int NUM);
void PAGE_Video(void);
void PAGE_Font(void);

void PAGE_BT_Audio(void);

void PAGE_Volume(void);

void PAGE_Generator();

void PAGE_Setting();

void PAGE_Script();

void PAGE_Player();

//Wifi
void PAGE_wifi(void);
void PAGE_wifi_switch(void);

void PAGE_MSD(void);

void PAGE_init_palitra(void); //Инициализация палитры

void PAGE_init_struct_generator(void);
void PAGE_init_struct_setting(void);


void prePageGenerator(void); //Пред функция для картинки генератор
void PAGE_generator_CH1_CH_EN_switch(void);
void PAGE_generator_CH1_AM_EN_switch(void);
void PAGE_generator_CH1_FM_EN_switch(void);

void PAGE_generator_CH2_CH_EN_switch(void);
void PAGE_generator_CH2_AM_EN_switch(void);
void PAGE_generator_CH2_FM_EN_switch(void);

void PAGE_generator_encoder_block_switch(void);
void PAGE_generator_select_modulation(void);

void generator_pre_CH1_FR(uint32_t index);
void generator_pre_CH2_FR(uint32_t index);
void generator_pre_CH1_AM_FR(uint32_t index);
void generator_pre_CH2_AM_FR(uint32_t index);
void generator_pre_CH1_FM_BASE(uint32_t index);
void generator_pre_CH2_FM_BASE(uint32_t index);
void generator_pre_CH1_FM_DEV(uint32_t index);
void generator_pre_CH2_FM_DEV(uint32_t index);
void generator_pre_CH1_FM_FR(uint32_t index);
void generator_pre_CH2_FM_FR(uint32_t index);
void generator_pre_CH1_EN(uint32_t index);
void generator_pre_CH2_EN(uint32_t index);
void generator_pre_CH1_AM_EN(uint32_t index);
void generator_pre_CH2_AM_EN(uint32_t index);
void generator_pre_CH1_FM_EN(uint32_t index);
void generator_pre_CH2_FM_EN(uint32_t index);

void setting_save_click(void);
void setting_script_stop(void);

#endif

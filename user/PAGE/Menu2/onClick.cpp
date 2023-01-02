#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "menu_typedef.h"

void PAGE_generator_CH1_CH_EN_switch(void){
	if (Gen.CH1.CH_EN)
	  Gen.CH1.CH_EN = 0;
    else
	  Gen.CH1.CH_EN = 1;

	menu_generator.field.needUpdate = 1;
	menu_generator.field.needRender = 1;
}

void PAGE_generator_CH1_AM_EN_switch(void){
if (Gen.CH1.AM_EN)
	Gen.CH1.AM_EN = 0;
else
	Gen.CH1.AM_EN = 1;
  menu_generator.field.needUpdate = 1;
  menu_generator.field.needRender = 1;
}

void PAGE_generator_CH1_FM_EN_switch(void)
{ if (Gen.CH1.FM_EN) Gen.CH1.FM_EN = 0; else Gen.CH1.FM_EN = 1;
  menu_generator.field.needUpdate = 1;
  menu_generator.field.needRender = 1;
}

void PAGE_generator_CH2_CH_EN_switch(void)
{ if (Gen.CH2.CH_EN) Gen.CH2.CH_EN = 0; else Gen.CH2.CH_EN = 1;
  menu_generator.field.needUpdate = 1;
  menu_generator.field.needRender = 1;
}

void PAGE_generator_CH2_AM_EN_switch(void)
{ if (Gen.CH2.AM_EN) Gen.CH2.AM_EN = 0; else Gen.CH2.AM_EN = 1;
  menu_generator.field.needUpdate = 1;
  menu_generator.field.needRender = 1;
}

void PAGE_generator_CH2_FM_EN_switch(void)
{ if (Gen.CH2.FM_EN) Gen.CH2.FM_EN = 0; else Gen.CH2.FM_EN = 1;
  menu_generator.field.needUpdate = 1;
  menu_generator.field.needRender = 1;
}


//Включение отключение блокировки
void PAGE_generator_encoder_block_switch(void){

  if (item_generator[menu_generator.index].field.disable == 0)
  {
	if (menu_generator.field.encoder_block)
		menu_generator.field.encoder_block = 0;
	else
		menu_generator.field.encoder_block = 1;
  }

  menu_generator.field.needUpdate = 1;
  menu_generator.field.needRender = 1;

}

#include "Page.h"
#include "global_define.h"
#include "FontSmooth.h"

#include "menu_typedef.h"

#include "global_typedef.h"
extern Encoder_typedef Encoder;

#include "Button.h"
extern button_typedef button;

#include "scripting.h"
extern Scripting script;

void PAGE_Menu2(menu_typedef * menu , item_typedef * item, int NUM);

void PAGE_Generator()
{
  PAGE_Menu2( &menu_generator,  &item_generator[0], NUM_ITEM_GENERETOR);
}

uint8_t render;

//┌───────────────────────────────────────────────────────────────────┐
//│  Построение в две строки                                          │
//└──────────────────────────────────────────────────┬────────────────┤
void PAGE_Menu2(menu_typedef *menu, item_typedef *item, int NUM) {

	Font_Smooth_Load(menu->font);

	volatile uint8_t i;

	int8_t index = 0;

	int max_window_item;
	int window_start;
	int window_end;

	max_window_item = NUM - 1; //6 реальных
	window_start = 0;
	window_end = (NUM >= max_window_item) ? max_window_item : NUM;

	void (*func_name)(void); //объявляем указатель на функцию

	tft.driver.needUpdate = 1;

	menu->field.needUpdate = 1;
	menu->field.needRender = 1;

	tft.uTFT.BColor = 0;
	tft.uTFT.GetColor = 0;

	menu->window_start = window_start;
	menu->window_end   = window_end;
	menu->index = index;
	menu->max_item = NUM - 1;

	while (1) {

		script.returnToMenu2 = false;

		script.run();

		button.tick();

		if (menu->preCallBackFunc) {
			menu->index = index;
			func_name = menu->preCallBackFunc;
			func_name();
		}

		//Блокировка энкодера, нужно чтобы обрабатывать в пре
		if (menu->field.encoder_block == 0) {
			if (Encoder.Left) {
				Encoder.Left = 0;
				index--;
				if (index < 0)
					index = NUM - 1;
				menu->field.needUpdate = 1;
				menu->field.needRender = 1; //Пенерисовка экрана
				menu->index = index;
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				index++;
				if (index >= NUM) index = 0;
				menu->field.needUpdate = 1;
				menu->field.needRender = 1; //Пенерисовка экрана
				menu->index = index;
			}
		}

		if (render)
		{
			render = 0;
			menu->field.needRender = 1;
			menu->field.needUpdate = 1;
		}

		if(menu->field.needRender) //Перерисовка экрана ~7ms
		{
			menu->field.needRender = 0;
			tft.Fill16(menu->ColorBackground); //Фон
            menu->refreshDisable();            //Обновим признаки Disable
	    	for (i = 0; i < NUM; i++) {
            	menu->run2(i);                 //Рендер строки экрана
	    	}
		}

		if ((menu->field.needUpdate) || (tft.driver.needUpdate)) {
			menu->field.needUpdate = 0;
			tft.driver.needUpdate = 0;
		    //LOG((char*)"MENU2",'I',(char*)"ST7789_UpdateDMA16bitV3");
			tft.driver.ST7789_UpdateDMA16bitV3(); //DMA8bitV2();
		}

		if (button.isHolded()) {
			rtt.info((char*)"MENU2 KEY.isHolded()");

			if (item[index].callBackFunc_isHolded) {
				func_name = item[index].callBackFunc_isHolded;
				func_name();
				tft.driver.needUpdate = 1;
				menu->field.needRender  = 1;
				Font_Smooth_Load(menu->font);
			}
		}

		if (button.isClick()) {
			button.isHolded();
			if (item[index].field.disable == 0)
			{
				if (item[index].callBackFunc_isClick) {
					func_name = item[index].callBackFunc_isClick;
					func_name();
					menu->field.needUpdate = 1; //При выходе из события обновляем экран
				}
			}
			tft.driver.needUpdate = 1; //При выходе из события обновляем экран
			menu->field.needRender  = 1;
		}

	}
}




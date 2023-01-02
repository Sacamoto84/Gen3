#include "Page.h"
#include "global_define.h"

#include "stdio.h"

//#include "TFT_Wiget_Animated_Rectagle.h"

#include "Gif.h"

extern Bitmap bmpBackground240240;

#include "menu_typedef.h"
#include "Button.h"
#include "scripting.h"

extern button_typedef button;
extern Scripting script;

#include "global_typedef.h"
extern Encoder_typedef Encoder;

void PAGE_Menu(menu_typedef *menu, item_typedef *item, int NUM) {

	Font_Smooth_Load(menu->font);

	volatile uint8_t i;

	uint8_t index = 0;

	int max_window_item;
	int window_start;
	int window_end;

	max_window_item = menu->item_count - 1; //6 реальных
	window_start = 0;
	window_end = (NUM >= max_window_item) ? max_window_item : NUM;

	void (*func_name)(void); //объявляем указатель на функцию

	uint32_t H = menu->item_height;
	uint32_t StartY = menu->item_start_y;

	tft.driver.needUpdate = 1;

	//* ┌ ┐ └ ┘├ ┤ ┬ ┴ ┼ ─ │
	//──────────────────────────────────┬──────────────────┐
	int gif_count = 0;                //│ Количество гифок │
	for (int i = 0; i < NUM; i++) {   //└──────────────────┤
		if (item[i].nameGif)                             //│
			gif_count++;                                 //│
	}                                                    //│
	//─────────────────────────────────────────────────────┘

	Gif gif1[gif_count];
	//Bitmap  bmp [gif_count];
	int8_t id;

	if (gif_count) {
		gif_count = 0;

		for (int i = 0; i < NUM; i++) {

			if (item[i].nameGif) {
				item[i].gif = &gif1[gif_count];
				item[i].gif->init(&tft);
				item[i].gif->setName(item[i].nameGif);
                id = item[i].resid_first;
				item[i].gif->bmpStop = getResBitmapID(id);
				item[i].gif->setDelay(0);
				item[i].gif->setX(item[i].gif_x);
				item[i].gif->trigger = item[i].gif_trigger;
				if (item[i].gif_init_state != NULL)
				{
					if (*item[i].gif_init_state != 0)
						item[i].gif->setIndexMax();
				}
				gif_count++;
			}
		}
	}
	//─────────────────────────────────────────────────────┘

	//item[1].gif->field.bit32 = 0;

	//char str[64];

	menu->field.needUpdate = 1;
	tft.uTFT.GetColor = 1;

	while (1) {

		if (script.returnToMenu2) return;

		script.run();

		button.tick();

		//tft.needUpdate = 0;

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
				if (index > 127)
					index = 0;
				if (index < window_start) {
					window_start = index;
					window_end = window_start + max_window_item;
				}
				menu->field.needUpdate = 1;
				//tft.needUpdate = 1;
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				index++;
				if (index >= NUM)
					index = NUM - 1;
				if (index > window_end) {
					window_end = index;
					window_start = window_end - max_window_item;
				}
				//tft.needUpdate = 1;
				menu->field.needUpdate = 1;
			}
		}

		menu->window_start = window_start;
		menu->window_end = window_end;
		menu->index = index;
		menu->max_item = NUM - 1;

		tft.Fill16(menu->ColorBackground); //Фон

		//TimerT5.Start();

		menu->ii = 0;
	    for (i = window_start; i <= window_end; i++) {

	    	if (i == menu->index) tft.RectangleFilled(0, StartY + H * (menu->ii % menu->item_count),
	    			239, H, COLOR_RECTAGLE);

            menu->run(i);

    		//if (item[i].gif) {
    		//		item[i].gif->setY(StartY + H * (menu->ii % menu->item_count));
    		//		item[i].gif->run(); //32x32x32 4800us(914285 Байт/Сек) -Of Gen Off    5000us Gen On 4096байт
    				//sprintf(str, "item[%d].gif->run();", i);
    				//LOG((char*)"MENU",'I',str);
    		//	}

    		//if (item[i].gif) {tft.ST7789_Update(item[i].gif->info());}

    		menu->ii++;

	    }



		//TimerT5.Loger("for (i = window_start; i <= window_end; i++)");


		//TimerT5.Start();


	    //┌───────────────────────────────────────────────────────────────────────┐
	    //│ Рисуем вертикальный скролл //87 us                                    │
	    //└───────────────────────────────────────────────────────────────────────┤
		if (menu->field.verticalScroll) {                                       //│
			tft.RectangleFilled(230, menu->item_start_y, 14,                    //│
					menu->item_count * menu->item_height, RGB565(0, 0, 0));     //│
			int H = (menu->item_count * menu->item_height) - 4;                 //│ Общая высота области в которой идет рисование
			float Hw = H * ((float) (menu->item_count)                          //│
									/ (float) (menu->max_item + 1));            //│ Размер самого ползунка
			float delta = float(H - Hw)                                         //│
					/ (float) (menu->max_item + 1 - menu->item_count);          //│
			tft.RectangleFilled(222 + 5 + 5,                                    //│
					menu->item_start_y + 2 + delta * (menu->window_start),      //│
					10 - 5, Hw, RGB565(205, 205, 205));                         //│
		}                                                                       //│
		//────────────────────────────────────────────────────────────────────────┘

		if (menu->postCallBackFunc) {
			func_name = menu->postCallBackFunc;
			func_name();
		}

		if ((menu->field.needUpdate) || (tft.driver.needUpdate)) {
			menu->field.needUpdate = 0;

		    rtt.println("MENU I ST7789_UpdateDMA16bitV3");

			tft.driver.ST7789_UpdateDMA16bitV3(); //DMA8bitV2();
		}

		if (button.isHolded()) {
			if (item[index].callBackFunc_isHolded) {
				func_name = item[index].callBackFunc_isHolded;
				func_name();
				tft.driver.needUpdate = 1;
			}
		}

		if (button.isDouble()) {
			if (item[index].callBackFunc_isDouble) {
				func_name = item[index].callBackFunc_isDouble;
				func_name();
				tft.driver.needUpdate = 1;
			}
		}

		if (button.isClick()) {

			button.isHolded();
			button.isDouble();

			if (item[index].field.disable == 0)
			{
				if (item[index].callBackFunc_isClick) {
					func_name = item[index].callBackFunc_isClick;
					func_name();
					menu->field.needUpdate = 1; //При выходе из события обновляем экран
				}
			}

			if (item[index].field.exit)
			{
				tft.driver.needUpdate = 1; //При выходе из события обновляем экран
				return;
			}
		}

#ifdef USE_CLI
		BLE_Task();
#endif

		//sendStructCHtoHost(0);

	}

}



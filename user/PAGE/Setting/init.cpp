#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "define.h"

#include "menu_typedef.h"

extern menu_typedef menu_generator;
extern menu_typedef menu_setting;

extern item_typedef item_setting[];
extern item_typedef item_generator[];

extern void PAGE_Menu(menu_typedef * menu , item_typedef * item, int NUM);

void PAGE_Setting(void)
{
  PAGE_Menu(&menu_setting , &item_setting[0], NUM_ITEM_SETTING);
}

void PAGE_init_struct_setting(void) {

	sprintf(item_setting[0].text, "Назад");
	item_setting[0].field.exit = 1;


	sprintf(item_setting[1].text, "Скрипт");
	item_setting[1].callBackFunc_isClick = &PAGE_Script;

	sprintf(item_setting[2].text, "Скрипт Stop");
	item_setting[2].callBackFunc_isClick = &setting_script_stop;

    //
	sprintf(item_setting[INDEX_SETTING_SAVE].text, "Сохранить");
	//page_item_setting[1].bmp = &bmpSave13232;
	item_setting[INDEX_SETTING_SAVE].nameGif = (char*)"save";
	item_setting[INDEX_SETTING_SAVE].gif_trigger = ONCE;
	item_setting[INDEX_SETTING_SAVE].gif_x = 170;
	item_setting[INDEX_SETTING_SAVE].callBackFunc_isClick = &setting_save_click;
    //

	//page_item_setting[2].bmp = &bmpProgram3232;

	//
	sprintf(item_setting[4].text, "USB MSD");
	item_setting[4].callBackFunc_isClick = &PAGE_MSD; //По нажатию

	//
	sprintf(item_setting[5].text, "Видео");
	item_setting[5].callBackFunc_isClick = &PAGE_Video;


	sprintf(item_setting[6].text, "BT Audio");
	item_setting[6].callBackFunc_isClick = &PAGE_BT_Audio;


	sprintf(item_setting[7].text, "Плеер");
	item_setting[7].callBackFunc_isClick = &PAGE_Player;


	for(int i = 0; i < 8 ; i++)
	{
		item_setting[i].text_color = -1;

	}

	//┌─────────────────────────────────────────────────────────────────┐
	//│  Описание как рендерить меню                                    │
	//└────────────────────────────────────────────────┬────────────────┤
	menu_setting.item_count   = 6;                   //│                │
	menu_setting.item_height  = 40;                  //│                │
	menu_setting.item_start_y = 0;                   //│                │
	                                                 //│                │
	menu_setting.font = Roboto_Medium_en_ru_30;      //│ Шрифт          │
	                                                 //│                │
	menu_setting.field.verticalScroll = 1;           //│ Включит скролл │
	menu_setting.items = item_setting;               //│ <-Items        │
	                                                 //│                │
	menu_setting.ColorBackground = COLOR_BACKGROUND; //│ Фон            │
	//─────────────────────────────────────────────────┴────────────────┘
}

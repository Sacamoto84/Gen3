#include "Page.h"
#include "global_define.h"
#include "stdio.h"

#include "menu_typedef.h"

#include "global_typedef.h"
extern Encoder_typedef Encoder;

extern void PAGE_Menu(menu_typedef * menu , item_typedef * item, int NUM);

//┌─────────────────────────────────────────┐
	#define INDEX_FILTER_BACK   0         //│
	#define INDEX_FILTER_EN     1         //│
	#define INDEX_FILTER_FR     2         //│
	#define INDEX_FILTER_TYPE   3         //│
	#define INDEX_FILTER_ORDER  4         //│
	#define INDEX_FILTER_SAVE   5         //│
	                                      //│
	#define NUM_ITEM_FILTER     6         //│
//└─────────────────────────────────────────┘

static menu_typedef menu_filter CCMRAM;
static item_typedef item_filter[NUM_ITEM_FILTER] CCMRAM;

static const char * save_text; //Текст строки сохранения

//Настройка изменилась: применяем сразу, чтобы смотреть осциллографом
static void filter_changed(void)
{
	Gen.Filter_Apply();
	save_text = "Сохранить";
	menu_filter.field.needUpdate = 1;
}

//Шаг растет с частотой, чтобы весь диапазон проходился энкодером
static uint32_t filter_fr_step(uint32_t fr)
{
	if (fr < 2000)  return 100;
	if (fr < 10000) return 500;
	if (fr < 30000) return 1000;
	return 5000;
}

static void filter_pre_EN(uint32_t index)
{
	sprintf(item_filter[INDEX_FILTER_EN].text, "Фильтр: %s", Gen.filter_cfg.EN ? "ON" : "OFF");
}

static void filter_pre_FR(uint32_t index)
{
	if (menu_filter.field.encoder_block) {

		if (index == INDEX_FILTER_FR)
		{
			uint32_t temp = Gen.filter_cfg.Fr;

			if (Encoder.Left) {
				Encoder.Left = 0;
				temp -= filter_fr_step(temp - 1);
				if (temp < FILTER_FR_MIN)
					temp = FILTER_FR_MIN;
				Gen.filter_cfg.Fr = temp;
				filter_changed();
			}

			if (Encoder.Right) {
				Encoder.Right = 0;
				temp += filter_fr_step(temp);
				if (temp > FILTER_FR_MAX)
					temp = FILTER_FR_MAX;
				Gen.filter_cfg.Fr = temp;
				filter_changed();
			}
		}

		sprintf(item_filter[INDEX_FILTER_FR].text, "> %lu Hz <", Gen.filter_cfg.Fr);
	} else {
		sprintf(item_filter[INDEX_FILTER_FR].text, "%lu Hz", Gen.filter_cfg.Fr);
	}
}

static void filter_pre_TYPE(uint32_t index)
{
	sprintf(item_filter[INDEX_FILTER_TYPE].text, "%s", (Gen.filter_cfg.Type == FILTER_BUTTERWORTH) ? "Butterworth" : "Bessel");
}

static void filter_pre_ORDER(uint32_t index)
{
	sprintf(item_filter[INDEX_FILTER_ORDER].text, "Порядок %d", Gen.filter_cfg.Order);
}

static void filter_pre_SAVE(uint32_t index)
{
	sprintf(item_filter[INDEX_FILTER_SAVE].text, "%s", save_text);
}

static void filter_EN_click(void)
{
	Gen.filter_cfg.EN = !Gen.filter_cfg.EN;
	filter_changed();
}

static void filter_FR_click(void)
{
	menu_filter.field.encoder_block = !menu_filter.field.encoder_block;
	menu_filter.field.needUpdate = 1;
}

static void filter_TYPE_click(void)
{
	Gen.filter_cfg.Type = (Gen.filter_cfg.Type == FILTER_BUTTERWORTH) ? FILTER_BESSEL : FILTER_BUTTERWORTH;
	filter_changed();
}

static void filter_ORDER_click(void)
{
	Gen.filter_cfg.Order = (Gen.filter_cfg.Order == 2) ? 4 : 2;
	filter_changed();
}

static void filter_SAVE_click(void)
{
	save_text = Gen.Filter_Save() ? "Сохранено" : "Ошибка SD";
}

void PAGE_Filter(void)
{
	for (int i = 0; i < NUM_ITEM_FILTER; i++)
		item_filter[i].text_color = -1;

	sprintf(item_filter[INDEX_FILTER_BACK].text, "Назад");
	item_filter[INDEX_FILTER_BACK].field.exit = 1;

	item_filter[INDEX_FILTER_EN].preCallBackFunc = &filter_pre_EN;
	item_filter[INDEX_FILTER_EN].callBackFunc_isClick = &filter_EN_click;

	item_filter[INDEX_FILTER_FR].preCallBackFunc = &filter_pre_FR;
	item_filter[INDEX_FILTER_FR].callBackFunc_isClick = &filter_FR_click;

	item_filter[INDEX_FILTER_TYPE].preCallBackFunc = &filter_pre_TYPE;
	item_filter[INDEX_FILTER_TYPE].callBackFunc_isClick = &filter_TYPE_click;

	item_filter[INDEX_FILTER_ORDER].preCallBackFunc = &filter_pre_ORDER;
	item_filter[INDEX_FILTER_ORDER].callBackFunc_isClick = &filter_ORDER_click;

	item_filter[INDEX_FILTER_SAVE].preCallBackFunc = &filter_pre_SAVE;
	item_filter[INDEX_FILTER_SAVE].callBackFunc_isClick = &filter_SAVE_click;

	//┌───────────────────────────────────────────────────────────────────┐
	//│  Описание как рендерить меню                                      │
	//└──────────────────────────────────────────────────┬────────────────┤
	menu_filter.item_count   = NUM_ITEM_FILTER;        //│                │
	menu_filter.item_height  = 40;                     //│                │
	menu_filter.item_start_y = 0;                      //│                │
	                                                   //│                │
	menu_filter.font = Roboto_Medium_en_ru_30;         //│ Шрифт          │
	menu_filter.items = item_filter;                   //│ <-Items        │
	menu_filter.ColorBackground = COLOR_BACKGROUND;    //│ Фон            │
	menu_filter.field.encoder_block = 0;               //│                │
	//───────────────────────────────────────────────────┴────────────────┘

	save_text = "Сохранить";

	PAGE_Menu(&menu_filter, &item_filter[0], NUM_ITEM_FILTER);

	Font_Smooth_Load(menu_generator.font);
}

#include "PAGE_Howl.h"
#include "TFT_color.h"
#include "Page.h"
#include "global_define.h"
#include "global_typedef.h"
#include "menu_typedef.h"
#include "howl_core.h"
#include <stdio.h>

extern Encoder_typedef Encoder;
extern void PAGE_Menu(menu_typedef *menu, item_typedef *item, int NUM);

#define INDEX_HACT_BACK        0
#define INDEX_HACT_NAME        1
#define INDEX_HACT_SPEED       2
#define INDEX_HACT_INTENSITY   3
#define INDEX_HACT_SEED        4
#define INDEX_HACT_RESET       5

#define NUM_ITEM_HOWL_ACT      6

static menu_typedef menu_howl_act CCMRAM;
static item_typedef item_howl_act[NUM_ITEM_HOWL_ACT] CCMRAM;

static void hact_pre_NAME(uint32_t index) {
	snprintf(item_howl_act[INDEX_HACT_NAME].text, sizeof(item_howl_act[INDEX_HACT_NAME].text),
		"Акт: %s", HowlActivities::GetActivityName(Howl.activities.active_id));
}

static void hact_click_NAME(void) {
	uint8_t next = (Howl.activities.active_id + 1) % HOWL_ACT_COUNT;
	Howl.activities.SelectActivity((ActivityId)next);
	menu_howl_act.field.needUpdate = 1;
}

static void hact_pre_SPEED(uint32_t index) {
	bool editing = (menu_howl_act.field.encoder_block && menu_howl_act.index == INDEX_HACT_SPEED);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float s = Howl.activities.speed_scale + delta * 0.1f;
			if (s < 0.2f) s = 0.2f;
			if (s > 4.0f) s = 4.0f;
			Howl.activities.speed_scale = s;
			menu_howl_act.field.needUpdate = 1;
		}
		snprintf(item_howl_act[INDEX_HACT_SPEED].text, sizeof(item_howl_act[INDEX_HACT_SPEED].text),
			"> Скор: %.1fx <", Howl.activities.speed_scale);
	} else {
		snprintf(item_howl_act[INDEX_HACT_SPEED].text, sizeof(item_howl_act[INDEX_HACT_SPEED].text),
			"Скор: %.1fx", Howl.activities.speed_scale);
	}
}

static void hact_pre_INTENSITY(uint32_t index) {
	bool editing = (menu_howl_act.field.encoder_block && menu_howl_act.index == INDEX_HACT_INTENSITY);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float val = Howl.activities.intensity_scale + delta * 0.05f;
			if (val < 0.2f) val = 0.2f;
			if (val > 2.0f) val = 2.0f;
			Howl.activities.intensity_scale = val;
			menu_howl_act.field.needUpdate = 1;
		}
		snprintf(item_howl_act[INDEX_HACT_INTENSITY].text, sizeof(item_howl_act[INDEX_HACT_INTENSITY].text),
			"> Интенс: %u%% <", (uint8_t)(Howl.activities.intensity_scale * 100.0f));
	} else {
		snprintf(item_howl_act[INDEX_HACT_INTENSITY].text, sizeof(item_howl_act[INDEX_HACT_INTENSITY].text),
			"Интенс: %u%%", (uint8_t)(Howl.activities.intensity_scale * 100.0f));
	}
}

static void hact_pre_SEED(uint32_t index) {
	bool editing = (menu_howl_act.field.encoder_block && menu_howl_act.index == INDEX_HACT_SEED);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int32_t s = (int32_t)Howl.activities.million_seed + delta * 1000;
			if (s < 0) s = 999000;
			if (s > 999999) s = 0;
			Howl.activities.million_seed = (uint32_t)s;
			Howl.activities.Reset();
			menu_howl_act.field.needUpdate = 1;
		}
		snprintf(item_howl_act[INDEX_HACT_SEED].text, sizeof(item_howl_act[INDEX_HACT_SEED].text),
			"> Seed: %lu <", (unsigned long)Howl.activities.million_seed);
	} else {
		snprintf(item_howl_act[INDEX_HACT_SEED].text, sizeof(item_howl_act[INDEX_HACT_SEED].text),
			"Seed: %lu", (unsigned long)Howl.activities.million_seed);
	}
}

static void hact_click_RESET(void) {
	Howl.activities.Reset();
	menu_howl_act.field.needUpdate = 1;
}

static void hact_param_click(void) {
	menu_howl_act.field.encoder_block = !menu_howl_act.field.encoder_block;
	menu_howl_act.field.needUpdate = 1;
}

void PAGE_Howl_Act(void) {
	item_howl_act[INDEX_HACT_BACK].field.exit = 1;
	snprintf(item_howl_act[INDEX_HACT_BACK].text, sizeof(item_howl_act[INDEX_HACT_BACK].text), "Назад");

	item_howl_act[INDEX_HACT_NAME].preCallBackFunc = &hact_pre_NAME;
	item_howl_act[INDEX_HACT_NAME].callBackFunc_isClick = &hact_click_NAME;

	item_howl_act[INDEX_HACT_SPEED].preCallBackFunc = &hact_pre_SPEED;
	item_howl_act[INDEX_HACT_SPEED].callBackFunc_isClick = &hact_param_click;

	item_howl_act[INDEX_HACT_INTENSITY].preCallBackFunc = &hact_pre_INTENSITY;
	item_howl_act[INDEX_HACT_INTENSITY].callBackFunc_isClick = &hact_param_click;

	item_howl_act[INDEX_HACT_SEED].preCallBackFunc = &hact_pre_SEED;
	item_howl_act[INDEX_HACT_SEED].callBackFunc_isClick = &hact_param_click;

	snprintf(item_howl_act[INDEX_HACT_RESET].text, sizeof(item_howl_act[INDEX_HACT_RESET].text), "Сброс ритма");
	item_howl_act[INDEX_HACT_RESET].preCallBackFunc = NULL;
	item_howl_act[INDEX_HACT_RESET].callBackFunc_isClick = &hact_click_RESET;

	for (int i = 0; i < NUM_ITEM_HOWL_ACT; i++) {
		item_howl_act[i].text_color = -1;
	}

	menu_howl_act.item_count = 6;
	menu_howl_act.item_height = 36;
	menu_howl_act.item_start_y = 0;
	menu_howl_act.font = Roboto_Medium_en_ru_18;
	menu_howl_act.field.verticalScroll = 1;
	menu_howl_act.items = item_howl_act;
	menu_howl_act.ColorBackground = COLOR_BACKGROUND;
	menu_howl_act.preCallBackFunc = NULL;

	PAGE_Menu(&menu_howl_act, item_howl_act, NUM_ITEM_HOWL_ACT);
}

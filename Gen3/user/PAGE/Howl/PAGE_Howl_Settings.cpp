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

#define INDEX_HSET_BACK         0
#define INDEX_HSET_MODE         1
#define INDEX_HSET_CARRIER      2
#define INDEX_HSET_CYCLES       3
#define INDEX_HSET_FADE         4
#define INDEX_HSET_MP_WIDTH     5
#define INDEX_HSET_LIMIT        6
#define INDEX_HSET_STEP         7
#define INDEX_HSET_AUTO_EN      8
#define INDEX_HSET_AUTO_STEP    9
#define INDEX_HSET_AUTO_INT    10
#define INDEX_HSET_SAVE        11

#define NUM_ITEM_HOWL_SET      12

static menu_typedef menu_howl_set CCMRAM;
static item_typedef item_howl_set[NUM_ITEM_HOWL_SET] CCMRAM;
static const char* save_status_text = "Сохранить";

static void hset_pre_MODE(uint32_t index) {
	const char* m = "Вейв";
	if (Howl.settings.pulse_mode == HOWL_PULSE_MULTIPULSE) m = "MultiP";
	else if (Howl.settings.pulse_mode == HOWL_PULSE_CONTINUOUS) m = "Непр";
	snprintf(item_howl_set[INDEX_HSET_MODE].text, sizeof(item_howl_set[INDEX_HSET_MODE].text),
		"Режим:%s", m);
}

static void hset_click_MODE(void) {
	uint8_t next = (Howl.settings.pulse_mode + 1) % 3;
	Howl.SetPulseMode((HowlPulseMode)next);
	save_status_text = "Сохранить";
	menu_howl_set.field.needUpdate = 1;
}

static void hset_pre_CARRIER(uint32_t index) {
	bool editing = (menu_howl_set.field.encoder_block && menu_howl_set.index == INDEX_HSET_CARRIER);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int v = (int)Howl.settings.carrier_hz + delta * 50;
			if (v < 300) v = 300;
			if (v > 3000) v = 3000;
			Howl.settings.carrier_hz = (uint16_t)v;
			save_status_text = "Сохранить";
			menu_howl_set.field.needUpdate = 1;
		}
		snprintf(item_howl_set[INDEX_HSET_CARRIER].text, sizeof(item_howl_set[INDEX_HSET_CARRIER].text),
			"> Нес:%uHz <", Howl.settings.carrier_hz);
	} else {
		snprintf(item_howl_set[INDEX_HSET_CARRIER].text, sizeof(item_howl_set[INDEX_HSET_CARRIER].text),
			"Несущ: %uHz", Howl.settings.carrier_hz);
	}
}

static void hset_pre_CYCLES(uint32_t index) {
	bool editing = (menu_howl_set.field.encoder_block && menu_howl_set.index == INDEX_HSET_CYCLES);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int v = (int)Howl.settings.wavelet_cycles + delta;
			if (v < 1) v = 1;
			if (v > 15) v = 15;
			Howl.settings.wavelet_cycles = (uint8_t)v;
			save_status_text = "Сохранить";
			menu_howl_set.field.needUpdate = 1;
		}
		snprintf(item_howl_set[INDEX_HSET_CYCLES].text, sizeof(item_howl_set[INDEX_HSET_CYCLES].text),
			"> Пер: %u <", Howl.settings.wavelet_cycles);
	} else {
		snprintf(item_howl_set[INDEX_HSET_CYCLES].text, sizeof(item_howl_set[INDEX_HSET_CYCLES].text),
			"Периоды: %u", Howl.settings.wavelet_cycles);
	}
}

static void hset_pre_FADE(uint32_t index) {
	bool editing = (menu_howl_set.field.encoder_block && menu_howl_set.index == INDEX_HSET_FADE);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int v = (int)Howl.settings.wavelet_fade + delta * 5;
			if (v < 0) v = 0;
			if (v > 100) v = 100;
			Howl.settings.wavelet_fade = (uint8_t)v;
			save_status_text = "Сохранить";
			menu_howl_set.field.needUpdate = 1;
		}
		snprintf(item_howl_set[INDEX_HSET_FADE].text, sizeof(item_howl_set[INDEX_HSET_FADE].text),
			"> Сглаж:%u <", Howl.settings.wavelet_fade);
	} else {
		snprintf(item_howl_set[INDEX_HSET_FADE].text, sizeof(item_howl_set[INDEX_HSET_FADE].text),
			"Сглаж: %u", Howl.settings.wavelet_fade);
	}
}

static void hset_pre_MP_WIDTH(uint32_t index) {
	bool editing = (menu_howl_set.field.encoder_block && menu_howl_set.index == INDEX_HSET_MP_WIDTH);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int v = (int)Howl.settings.multipulse_width_us + delta * 10;
			if (v < 50) v = 50;
			if (v > 500) v = 500;
			Howl.settings.multipulse_width_us = (uint16_t)v;
			save_status_text = "Сохранить";
			menu_howl_set.field.needUpdate = 1;
		}
		snprintf(item_howl_set[INDEX_HSET_MP_WIDTH].text, sizeof(item_howl_set[INDEX_HSET_MP_WIDTH].text),
			"> MultiP:%uus <", Howl.settings.multipulse_width_us);
	} else {
		snprintf(item_howl_set[INDEX_HSET_MP_WIDTH].text, sizeof(item_howl_set[INDEX_HSET_MP_WIDTH].text),
			"MultiP: %uus", Howl.settings.multipulse_width_us);
	}
}

static void hset_pre_LIMIT(uint32_t index) {
	bool editing = (menu_howl_set.field.encoder_block && menu_howl_set.index == INDEX_HSET_LIMIT);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int v = (int)Howl.settings.power_limit + delta * 5;
			if (v < 20) v = 20;
			if (v > 200) v = 200;
			Howl.SetPowerLimit((uint8_t)v);
			save_status_text = "Сохранить";
			menu_howl_set.field.needUpdate = 1;
		}
		snprintf(item_howl_set[INDEX_HSET_LIMIT].text, sizeof(item_howl_set[INDEX_HSET_LIMIT].text),
			"> Лимит: %u <", Howl.settings.power_limit);
	} else {
		snprintf(item_howl_set[INDEX_HSET_LIMIT].text, sizeof(item_howl_set[INDEX_HSET_LIMIT].text),
			"Лимит: %u", Howl.settings.power_limit);
	}
}

static void hset_pre_STEP(uint32_t index) {
	bool editing = (menu_howl_set.field.encoder_block && menu_howl_set.index == INDEX_HSET_STEP);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int v = (int)Howl.settings.power_step + delta;
			if (v < 1) v = 1;
			if (v > 20) v = 20;
			Howl.settings.power_step = (uint8_t)v;
			save_status_text = "Сохранить";
			menu_howl_set.field.needUpdate = 1;
		}
		snprintf(item_howl_set[INDEX_HSET_STEP].text, sizeof(item_howl_set[INDEX_HSET_STEP].text),
			"> Шаг: %u <", Howl.settings.power_step);
	} else {
		snprintf(item_howl_set[INDEX_HSET_STEP].text, sizeof(item_howl_set[INDEX_HSET_STEP].text),
			"Шаг: %u", Howl.settings.power_step);
	}
}

static void hset_pre_AUTO_EN(uint32_t index) {
	snprintf(item_howl_set[INDEX_HSET_AUTO_EN].text, sizeof(item_howl_set[INDEX_HSET_AUTO_EN].text),
		"Авто: %s", Howl.settings.auto_inc_en ? "ВКЛ" : "ВЫКЛ");
}

static void hset_click_AUTO_EN(void) {
	Howl.settings.auto_inc_en = !Howl.settings.auto_inc_en;
	save_status_text = "Сохранить";
	menu_howl_set.field.needUpdate = 1;
}

static void hset_pre_AUTO_STEP(uint32_t index) {
	bool editing = (menu_howl_set.field.encoder_block && menu_howl_set.index == INDEX_HSET_AUTO_STEP);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int v = (int)Howl.settings.auto_inc_step + delta;
			if (v < 1) v = 1;
			if (v > 10) v = 10;
			Howl.settings.auto_inc_step = (uint8_t)v;
			save_status_text = "Сохранить";
			menu_howl_set.field.needUpdate = 1;
		}
		snprintf(item_howl_set[INDEX_HSET_AUTO_STEP].text, sizeof(item_howl_set[INDEX_HSET_AUTO_STEP].text),
			"> Шаг:+%u <", Howl.settings.auto_inc_step);
	} else {
		snprintf(item_howl_set[INDEX_HSET_AUTO_STEP].text, sizeof(item_howl_set[INDEX_HSET_AUTO_STEP].text),
			"Шаг: +%u", Howl.settings.auto_inc_step);
	}
}

static void hset_pre_AUTO_INT(uint32_t index) {
	bool editing = (menu_howl_set.field.encoder_block && menu_howl_set.index == INDEX_HSET_AUTO_INT);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			int v = (int)Howl.settings.auto_inc_interval_s + delta * 5;
			if (v < 5) v = 5;
			if (v > 120) v = 120;
			Howl.settings.auto_inc_interval_s = (uint16_t)v;
			save_status_text = "Сохранить";
			menu_howl_set.field.needUpdate = 1;
		}
		snprintf(item_howl_set[INDEX_HSET_AUTO_INT].text, sizeof(item_howl_set[INDEX_HSET_AUTO_INT].text),
			"> Пер:%us <", Howl.settings.auto_inc_interval_s);
	} else {
		snprintf(item_howl_set[INDEX_HSET_AUTO_INT].text, sizeof(item_howl_set[INDEX_HSET_AUTO_INT].text),
			"Пер: %us", Howl.settings.auto_inc_interval_s);
	}
}

static void hset_pre_SAVE(uint32_t index) {
	snprintf(item_howl_set[INDEX_HSET_SAVE].text, sizeof(item_howl_set[INDEX_HSET_SAVE].text),
		"%s", save_status_text);
}

static void hset_click_SAVE(void) {
	if (Howl.Config_Save()) {
		save_status_text = "Сохранено!";
	} else {
		save_status_text = "Ошибка!";
	}
	menu_howl_set.field.needUpdate = 1;
}

static void hset_param_click(void) {
	menu_howl_set.field.encoder_block = !menu_howl_set.field.encoder_block;
	menu_howl_set.field.needUpdate = 1;
}

void PAGE_Howl_Settings(void) {
	save_status_text = "Сохранить";

	item_howl_set[INDEX_HSET_BACK].field.exit = 1;
	snprintf(item_howl_set[INDEX_HSET_BACK].text, sizeof(item_howl_set[INDEX_HSET_BACK].text), "Назад");

	item_howl_set[INDEX_HSET_MODE].preCallBackFunc = &hset_pre_MODE;
	item_howl_set[INDEX_HSET_MODE].callBackFunc_isClick = &hset_click_MODE;

	item_howl_set[INDEX_HSET_CARRIER].preCallBackFunc = &hset_pre_CARRIER;
	item_howl_set[INDEX_HSET_CARRIER].callBackFunc_isClick = &hset_param_click;

	item_howl_set[INDEX_HSET_CYCLES].preCallBackFunc = &hset_pre_CYCLES;
	item_howl_set[INDEX_HSET_CYCLES].callBackFunc_isClick = &hset_param_click;

	item_howl_set[INDEX_HSET_FADE].preCallBackFunc = &hset_pre_FADE;
	item_howl_set[INDEX_HSET_FADE].callBackFunc_isClick = &hset_param_click;

	item_howl_set[INDEX_HSET_MP_WIDTH].preCallBackFunc = &hset_pre_MP_WIDTH;
	item_howl_set[INDEX_HSET_MP_WIDTH].callBackFunc_isClick = &hset_param_click;

	item_howl_set[INDEX_HSET_LIMIT].preCallBackFunc = &hset_pre_LIMIT;
	item_howl_set[INDEX_HSET_LIMIT].callBackFunc_isClick = &hset_param_click;

	item_howl_set[INDEX_HSET_STEP].preCallBackFunc = &hset_pre_STEP;
	item_howl_set[INDEX_HSET_STEP].callBackFunc_isClick = &hset_param_click;

	item_howl_set[INDEX_HSET_AUTO_EN].preCallBackFunc = &hset_pre_AUTO_EN;
	item_howl_set[INDEX_HSET_AUTO_EN].callBackFunc_isClick = &hset_click_AUTO_EN;

	item_howl_set[INDEX_HSET_AUTO_STEP].preCallBackFunc = &hset_pre_AUTO_STEP;
	item_howl_set[INDEX_HSET_AUTO_STEP].callBackFunc_isClick = &hset_param_click;

	item_howl_set[INDEX_HSET_AUTO_INT].preCallBackFunc = &hset_pre_AUTO_INT;
	item_howl_set[INDEX_HSET_AUTO_INT].callBackFunc_isClick = &hset_param_click;

	item_howl_set[INDEX_HSET_SAVE].preCallBackFunc = &hset_pre_SAVE;
	item_howl_set[INDEX_HSET_SAVE].callBackFunc_isClick = &hset_click_SAVE;

	for (int i = 0; i < NUM_ITEM_HOWL_SET; i++) {
		item_howl_set[i].text_color = -1;
	}

	menu_howl_set.item_count = 6;
	menu_howl_set.item_height = 36;
	menu_howl_set.item_start_y = 0;
	menu_howl_set.font = Roboto_Medium_en_ru_18;
	menu_howl_set.field.verticalScroll = 1;
	menu_howl_set.items = item_howl_set;
	menu_howl_set.ColorBackground = COLOR_BACKGROUND;
	menu_howl_set.preCallBackFunc = NULL;

	PAGE_Menu(&menu_howl_set, item_howl_set, NUM_ITEM_HOWL_SET);
}

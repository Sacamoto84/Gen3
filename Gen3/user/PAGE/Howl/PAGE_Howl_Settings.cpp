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

enum HowlSettingParamId {
	HPARAM_BACK,
	HPARAM_MODE,
	HPARAM_CARRIER,
	HPARAM_CYCLES,
	HPARAM_FADE,
	HPARAM_MP_WIDTH,
	HPARAM_LIMIT,
	HPARAM_STEP,
	HPARAM_AUTO_EN,
	HPARAM_AUTO_STEP,
	HPARAM_AUTO_INT,
	HPARAM_SAVE,
	HPARAM_MAX
};

static int param_to_item_index[HPARAM_MAX];
static const int MAX_HOWL_SETTINGS_ITEMS = 14;

static menu_typedef menu_howl_set CCMRAM;
static item_typedef item_howl_set[MAX_HOWL_SETTINGS_ITEMS] CCMRAM;
static const char* save_status_text = "Сохранить";
static bool exit_requested = false;

// ── 1. Навигация и Режим ──────────────────────────────────────────
static void hset_click_BACK(void) {
	exit_requested = true;
}

static void hset_pre_MODE(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_MODE];
	if (my_idx < 0) return;
	const char* m = "Вейвлет";
	if (Howl.settings.pulse_mode == HOWL_PULSE_MULTIPULSE) m = "MultiPulse";
	else if (Howl.settings.pulse_mode == HOWL_PULSE_CONTINUOUS) m = "Непрерыв";
	snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
		"Реж: %s", m);
}

static void hset_click_MODE(void) {
	uint8_t next = (Howl.settings.pulse_mode + 1) % 3;
	Howl.SetPulseMode((HowlPulseMode)next);
	save_status_text = "Сохранить";
	menu_howl_set.index = 1;
	// field.exit = 1 triggers menu rebuild in while(!exit_requested)
}

// ── 2. Параметры режима Wavelet ──────────────────────────────────
static void hset_pre_CARRIER(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_CARRIER];
	if (my_idx < 0) return;
	bool editing = (menu_howl_set.field.encoder_block && (int)menu_howl_set.index == my_idx);
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
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"> [W] Нес:%uHz <", Howl.settings.carrier_hz);
	} else {
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"[W] Несущ: %uHz", Howl.settings.carrier_hz);
	}
}

static void hset_pre_CYCLES(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_CYCLES];
	if (my_idx < 0) return;
	bool editing = (menu_howl_set.field.encoder_block && (int)menu_howl_set.index == my_idx);
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
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"> [W] Пер: %u <", Howl.settings.wavelet_cycles);
	} else {
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"[W] Периоды: %u", Howl.settings.wavelet_cycles);
	}
}

static void hset_pre_FADE(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_FADE];
	if (my_idx < 0) return;
	bool editing = (menu_howl_set.field.encoder_block && (int)menu_howl_set.index == my_idx);
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
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"> [W] Сглаж:%u%% <", Howl.settings.wavelet_fade);
	} else {
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"[W] Сглаж: %u%%", Howl.settings.wavelet_fade);
	}
}

// ── 3. Параметры режима MultiPulse ───────────────────────────────
static void hset_pre_MP_WIDTH(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_MP_WIDTH];
	if (my_idx < 0) return;
	bool editing = (menu_howl_set.field.encoder_block && (int)menu_howl_set.index == my_idx);
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
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"> [MP] %uus <", Howl.settings.multipulse_width_us);
	} else {
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"[MP] Длит: %uus", Howl.settings.multipulse_width_us);
	}
}

// ── 5. Общие параметры мощности и безопасности ───────────────────
static void hset_pre_LIMIT(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_LIMIT];
	if (my_idx < 0) return;
	bool editing = (menu_howl_set.field.encoder_block && (int)menu_howl_set.index == my_idx);
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
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"> [Общ] Лим:%u <", Howl.settings.power_limit);
	} else {
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"[Общ] Лимит: %u", Howl.settings.power_limit);
	}
}

static void hset_pre_STEP(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_STEP];
	if (my_idx < 0) return;
	bool editing = (menu_howl_set.field.encoder_block && (int)menu_howl_set.index == my_idx);
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
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"> [Общ] Шаг:%u <", Howl.settings.power_step);
	} else {
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"[Общ] Шаг: %u", Howl.settings.power_step);
	}
}

// ── 6. Общие параметры авто-нарастания ────────────────────────────
static void hset_pre_AUTO_EN(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_AUTO_EN];
	if (my_idx < 0) return;
	snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
		"[Авт] Рост: %s", Howl.settings.auto_inc_en ? "ВКЛ" : "ВЫКЛ");
}

static void hset_click_AUTO_EN(void) {
	Howl.settings.auto_inc_en = !Howl.settings.auto_inc_en;
	save_status_text = "Сохранить";
	menu_howl_set.field.needUpdate = 1;
}

static void hset_pre_AUTO_STEP(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_AUTO_STEP];
	if (my_idx < 0) return;
	bool editing = (menu_howl_set.field.encoder_block && (int)menu_howl_set.index == my_idx);
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
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"> [Авт] +%u <", Howl.settings.auto_inc_step);
	} else {
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"[Авт] Шаг: +%u", Howl.settings.auto_inc_step);
	}
}

static void hset_pre_AUTO_INT(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_AUTO_INT];
	if (my_idx < 0) return;
	bool editing = (menu_howl_set.field.encoder_block && (int)menu_howl_set.index == my_idx);
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
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"> [Авт] %us <", Howl.settings.auto_inc_interval_s);
	} else {
		snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
			"[Авт] Пер: %us", Howl.settings.auto_inc_interval_s);
	}
}

// ── 7. Сохранение ────────────────────────────────────────────────
static void hset_pre_SAVE(uint32_t index) {
	int my_idx = param_to_item_index[HPARAM_SAVE];
	if (my_idx < 0) return;
	snprintf(item_howl_set[my_idx].text, sizeof(item_howl_set[my_idx].text),
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

static int setup_items_for_current_mode(void) {
	for (int i = 0; i < HPARAM_MAX; i++) {
		param_to_item_index[i] = -1;
	}

	int idx = 0;

	// 0: Назад
	param_to_item_index[HPARAM_BACK] = idx;
	item_howl_set[idx].field.exit = 1;
	item_howl_set[idx].field.disable = 0;
	item_howl_set[idx].text_color = -1;
	item_howl_set[idx].preCallBackFunc = NULL;
	item_howl_set[idx].callBackFunc_isClick = &hset_click_BACK;
	snprintf(item_howl_set[idx].text, sizeof(item_howl_set[idx].text), "Назад");
	idx++;

	// 1: Режим
	param_to_item_index[HPARAM_MODE] = idx;
	item_howl_set[idx].field.exit = 1; // клик переключает режим и перезагружает список
	item_howl_set[idx].field.disable = 0;
	item_howl_set[idx].text_color = -1;
	item_howl_set[idx].preCallBackFunc = &hset_pre_MODE;
	item_howl_set[idx].callBackFunc_isClick = &hset_click_MODE;
	hset_pre_MODE(idx);
	idx++;

	// 2: Специфичные параметры выбранного режима
	if (Howl.settings.pulse_mode == HOWL_PULSE_WAVELET) {
		// Несущая частота
		param_to_item_index[HPARAM_CARRIER] = idx;
		item_howl_set[idx].field.exit = 0;
		item_howl_set[idx].field.disable = 0;
		item_howl_set[idx].text_color = -1;
		item_howl_set[idx].preCallBackFunc = &hset_pre_CARRIER;
		item_howl_set[idx].callBackFunc_isClick = &hset_param_click;
		hset_pre_CARRIER(idx);
		idx++;

		// Периоды всплеска
		param_to_item_index[HPARAM_CYCLES] = idx;
		item_howl_set[idx].field.exit = 0;
		item_howl_set[idx].field.disable = 0;
		item_howl_set[idx].text_color = -1;
		item_howl_set[idx].preCallBackFunc = &hset_pre_CYCLES;
		item_howl_set[idx].callBackFunc_isClick = &hset_param_click;
		hset_pre_CYCLES(idx);
		idx++;

		// Сглаживание огибающей
		param_to_item_index[HPARAM_FADE] = idx;
		item_howl_set[idx].field.exit = 0;
		item_howl_set[idx].field.disable = 0;
		item_howl_set[idx].text_color = -1;
		item_howl_set[idx].preCallBackFunc = &hset_pre_FADE;
		item_howl_set[idx].callBackFunc_isClick = &hset_param_click;
		hset_pre_FADE(idx);
		idx++;
	} else if (Howl.settings.pulse_mode == HOWL_PULSE_MULTIPULSE) {
		// Длительность импульса MultiPulse
		param_to_item_index[HPARAM_MP_WIDTH] = idx;
		item_howl_set[idx].field.exit = 0;
		item_howl_set[idx].field.disable = 0;
		item_howl_set[idx].text_color = -1;
		item_howl_set[idx].preCallBackFunc = &hset_pre_MP_WIDTH;
		item_howl_set[idx].callBackFunc_isClick = &hset_param_click;
		hset_pre_MP_WIDTH(idx);
		idx++;
	} else { // HOWL_PULSE_CONTINUOUS
		// Непрерывный гармонический режим: частота модулируется динамически (5..100 Гц),
		// поэтому импульсных параметров пачки нет
	}

	// 3: Общие параметры мощности и безопасности
	param_to_item_index[HPARAM_LIMIT] = idx;
	item_howl_set[idx].field.exit = 0;
	item_howl_set[idx].field.disable = 0;
	item_howl_set[idx].text_color = -1;
	item_howl_set[idx].preCallBackFunc = &hset_pre_LIMIT;
	item_howl_set[idx].callBackFunc_isClick = &hset_param_click;
	hset_pre_LIMIT(idx);
	idx++;

	param_to_item_index[HPARAM_STEP] = idx;
	item_howl_set[idx].field.exit = 0;
	item_howl_set[idx].field.disable = 0;
	item_howl_set[idx].text_color = -1;
	item_howl_set[idx].preCallBackFunc = &hset_pre_STEP;
	item_howl_set[idx].callBackFunc_isClick = &hset_param_click;
	hset_pre_STEP(idx);
	idx++;

	// 4: Общие параметры авто-нарастания
	param_to_item_index[HPARAM_AUTO_EN] = idx;
	item_howl_set[idx].field.exit = 0;
	item_howl_set[idx].field.disable = 0;
	item_howl_set[idx].text_color = -1;
	item_howl_set[idx].preCallBackFunc = &hset_pre_AUTO_EN;
	item_howl_set[idx].callBackFunc_isClick = &hset_click_AUTO_EN;
	hset_pre_AUTO_EN(idx);
	idx++;

	param_to_item_index[HPARAM_AUTO_STEP] = idx;
	item_howl_set[idx].field.exit = 0;
	item_howl_set[idx].field.disable = 0;
	item_howl_set[idx].text_color = -1;
	item_howl_set[idx].preCallBackFunc = &hset_pre_AUTO_STEP;
	item_howl_set[idx].callBackFunc_isClick = &hset_param_click;
	hset_pre_AUTO_STEP(idx);
	idx++;

	param_to_item_index[HPARAM_AUTO_INT] = idx;
	item_howl_set[idx].field.exit = 0;
	item_howl_set[idx].field.disable = 0;
	item_howl_set[idx].text_color = -1;
	item_howl_set[idx].preCallBackFunc = &hset_pre_AUTO_INT;
	item_howl_set[idx].callBackFunc_isClick = &hset_param_click;
	hset_pre_AUTO_INT(idx);
	idx++;

	// 5: Сохранить
	param_to_item_index[HPARAM_SAVE] = idx;
	item_howl_set[idx].field.exit = 0;
	item_howl_set[idx].field.disable = 0;
	item_howl_set[idx].text_color = -1;
	item_howl_set[idx].preCallBackFunc = &hset_pre_SAVE;
	item_howl_set[idx].callBackFunc_isClick = &hset_click_SAVE;
	hset_pre_SAVE(idx);
	idx++;

	return idx;
}

static void hset_pre_menu(void) {
	Howl.Update();
}

void PAGE_Howl_Settings(void) {
	save_status_text = "Сохранить";
	exit_requested = false;

	menu_howl_set.item_count = 6;
	menu_howl_set.item_height = 36;
	menu_howl_set.item_start_y = 0;
	menu_howl_set.font = Roboto_Medium_en_ru_18;
	menu_howl_set.field.verticalScroll = 1;
	menu_howl_set.field.encoder_block = 0;
	menu_howl_set.items = item_howl_set;
	menu_howl_set.ColorBackground = COLOR_BACKGROUND;
	menu_howl_set.preCallBackFunc = &hset_pre_menu;
	menu_howl_set.index = 1; // Фокус по умолчанию на строке "Режим"

	while (!exit_requested) {
		int num_items = setup_items_for_current_mode();
		menu_howl_set.max_item = num_items - 1;
		PAGE_Menu(&menu_howl_set, item_howl_set, num_items);
	}

	Font_Smooth_Load(menu_howl_set.font);
}

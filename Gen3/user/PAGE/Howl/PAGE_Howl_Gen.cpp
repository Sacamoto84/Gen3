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

#define INDEX_HGEN_BACK         0
#define INDEX_HGEN_AMP_SHAPE    1
#define INDEX_HGEN_AMP_SPEED    2
#define INDEX_HGEN_AMP_MIN      3
#define INDEX_HGEN_AMP_MAX      4
#define INDEX_HGEN_FREQ_SHAPE   5
#define INDEX_HGEN_FREQ_SPEED   6
#define INDEX_HGEN_FREQ_MIN     7
#define INDEX_HGEN_FREQ_MAX     8
#define INDEX_HGEN_AUTO_CHANGE  9
#define INDEX_HGEN_RANDOMIZE   10

#define NUM_ITEM_HOWL_GEN      11

static menu_typedef menu_howl_gen CCMRAM;
static item_typedef item_howl_gen[NUM_ITEM_HOWL_GEN] CCMRAM;

static void hgen_pre_AMP_SHAPE(uint32_t index) {
	snprintf(item_howl_gen[INDEX_HGEN_AMP_SHAPE].text, sizeof(item_howl_gen[INDEX_HGEN_AMP_SHAPE].text),
		"A: %.15s", HowlWaveGenerator::GetShapeName(Howl.wave_gen.amp_wave.shape_id));
}

static void hgen_click_AMP_SHAPE(void) {
	uint8_t next = (Howl.wave_gen.amp_wave.shape_id + 1) % WAVE_SHAPE_COUNT;
	Howl.wave_gen.amp_wave.shape_id = (WaveShapeId)next;
	menu_howl_gen.field.needUpdate = 1;
}

static void hgen_pre_AMP_SPEED(uint32_t index) {
	bool editing = (menu_howl_gen.field.encoder_block && menu_howl_gen.index == INDEX_HGEN_AMP_SPEED);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float s = Howl.wave_gen.amp_wave.speed_hz + delta * 0.02f;
			if (s < 0.02f) s = 0.02f;
			if (s > 2.00f) s = 2.00f;
			Howl.wave_gen.amp_wave.speed_hz = s;
			menu_howl_gen.field.needUpdate = 1;
		}
		snprintf(item_howl_gen[INDEX_HGEN_AMP_SPEED].text, sizeof(item_howl_gen[INDEX_HGEN_AMP_SPEED].text),
			"> СкорA:%.2f <", Howl.wave_gen.amp_wave.speed_hz);
	} else {
		snprintf(item_howl_gen[INDEX_HGEN_AMP_SPEED].text, sizeof(item_howl_gen[INDEX_HGEN_AMP_SPEED].text),
			"СкорA: %.2fHz", Howl.wave_gen.amp_wave.speed_hz);
	}
}

static void hgen_pre_AMP_MIN(uint32_t index) {
	bool editing = (menu_howl_gen.field.encoder_block && menu_howl_gen.index == INDEX_HGEN_AMP_MIN);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float v = Howl.wave_gen.amp_wave.min_val + delta * 0.05f;
			if (v < 0.0f) v = 0.0f;
			if (v > Howl.wave_gen.amp_wave.max_val) v = Howl.wave_gen.amp_wave.max_val;
			Howl.wave_gen.amp_wave.min_val = v;
			menu_howl_gen.field.needUpdate = 1;
		}
		snprintf(item_howl_gen[INDEX_HGEN_AMP_MIN].text, sizeof(item_howl_gen[INDEX_HGEN_AMP_MIN].text),
			"> Мин A: %u%% <", (uint8_t)(Howl.wave_gen.amp_wave.min_val * 100.0f));
	} else {
		snprintf(item_howl_gen[INDEX_HGEN_AMP_MIN].text, sizeof(item_howl_gen[INDEX_HGEN_AMP_MIN].text),
			"Мин A: %u%%", (uint8_t)(Howl.wave_gen.amp_wave.min_val * 100.0f));
	}
}

static void hgen_pre_AMP_MAX(uint32_t index) {
	bool editing = (menu_howl_gen.field.encoder_block && menu_howl_gen.index == INDEX_HGEN_AMP_MAX);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float v = Howl.wave_gen.amp_wave.max_val + delta * 0.05f;
			if (v < Howl.wave_gen.amp_wave.min_val) v = Howl.wave_gen.amp_wave.min_val;
			if (v > 1.0f) v = 1.0f;
			Howl.wave_gen.amp_wave.max_val = v;
			menu_howl_gen.field.needUpdate = 1;
		}
		snprintf(item_howl_gen[INDEX_HGEN_AMP_MAX].text, sizeof(item_howl_gen[INDEX_HGEN_AMP_MAX].text),
			"> МаксA: %u%% <", (uint8_t)(Howl.wave_gen.amp_wave.max_val * 100.0f));
	} else {
		snprintf(item_howl_gen[INDEX_HGEN_AMP_MAX].text, sizeof(item_howl_gen[INDEX_HGEN_AMP_MAX].text),
			"Макс A: %u%%", (uint8_t)(Howl.wave_gen.amp_wave.max_val * 100.0f));
	}
}

static void hgen_pre_FREQ_SHAPE(uint32_t index) {
	snprintf(item_howl_gen[INDEX_HGEN_FREQ_SHAPE].text, sizeof(item_howl_gen[INDEX_HGEN_FREQ_SHAPE].text),
		"F: %.15s", HowlWaveGenerator::GetShapeName(Howl.wave_gen.freq_wave.shape_id));
}

static void hgen_click_FREQ_SHAPE(void) {
	uint8_t next = (Howl.wave_gen.freq_wave.shape_id + 1) % WAVE_SHAPE_COUNT;
	Howl.wave_gen.freq_wave.shape_id = (WaveShapeId)next;
	menu_howl_gen.field.needUpdate = 1;
}

static void hgen_pre_FREQ_SPEED(uint32_t index) {
	bool editing = (menu_howl_gen.field.encoder_block && menu_howl_gen.index == INDEX_HGEN_FREQ_SPEED);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float s = Howl.wave_gen.freq_wave.speed_hz + delta * 0.02f;
			if (s < 0.02f) s = 0.02f;
			if (s > 2.00f) s = 2.00f;
			Howl.wave_gen.freq_wave.speed_hz = s;
			menu_howl_gen.field.needUpdate = 1;
		}
		snprintf(item_howl_gen[INDEX_HGEN_FREQ_SPEED].text, sizeof(item_howl_gen[INDEX_HGEN_FREQ_SPEED].text),
			"> СкорF:%.2f <", Howl.wave_gen.freq_wave.speed_hz);
	} else {
		snprintf(item_howl_gen[INDEX_HGEN_FREQ_SPEED].text, sizeof(item_howl_gen[INDEX_HGEN_FREQ_SPEED].text),
			"СкорF: %.2fHz", Howl.wave_gen.freq_wave.speed_hz);
	}
}

static void hgen_pre_FREQ_MIN(uint32_t index) {
	bool editing = (menu_howl_gen.field.encoder_block && menu_howl_gen.index == INDEX_HGEN_FREQ_MIN);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float v = Howl.wave_gen.freq_wave.min_val + delta * 0.05f;
			if (v < 0.0f) v = 0.0f;
			if (v > Howl.wave_gen.freq_wave.max_val) v = Howl.wave_gen.freq_wave.max_val;
			Howl.wave_gen.freq_wave.min_val = v;
			menu_howl_gen.field.needUpdate = 1;
		}
		snprintf(item_howl_gen[INDEX_HGEN_FREQ_MIN].text, sizeof(item_howl_gen[INDEX_HGEN_FREQ_MIN].text),
			"> Мин F: %u%% <", (uint8_t)(Howl.wave_gen.freq_wave.min_val * 100.0f));
	} else {
		snprintf(item_howl_gen[INDEX_HGEN_FREQ_MIN].text, sizeof(item_howl_gen[INDEX_HGEN_FREQ_MIN].text),
			"Мин F: %u%%", (uint8_t)(Howl.wave_gen.freq_wave.min_val * 100.0f));
	}
}

static void hgen_pre_FREQ_MAX(uint32_t index) {
	bool editing = (menu_howl_gen.field.encoder_block && menu_howl_gen.index == INDEX_HGEN_FREQ_MAX);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float v = Howl.wave_gen.freq_wave.max_val + delta * 0.05f;
			if (v < Howl.wave_gen.freq_wave.min_val) v = Howl.wave_gen.freq_wave.min_val;
			if (v > 1.0f) v = 1.0f;
			Howl.wave_gen.freq_wave.max_val = v;
			menu_howl_gen.field.needUpdate = 1;
		}
		snprintf(item_howl_gen[INDEX_HGEN_FREQ_MAX].text, sizeof(item_howl_gen[INDEX_HGEN_FREQ_MAX].text),
			"> МаксF: %u%% <", (uint8_t)(Howl.wave_gen.freq_wave.max_val * 100.0f));
	} else {
		snprintf(item_howl_gen[INDEX_HGEN_FREQ_MAX].text, sizeof(item_howl_gen[INDEX_HGEN_FREQ_MAX].text),
			"Макс F: %u%%", (uint8_t)(Howl.wave_gen.freq_wave.max_val * 100.0f));
	}
}

static void hgen_pre_AUTO_CHANGE(uint32_t index) {
	snprintf(item_howl_gen[INDEX_HGEN_AUTO_CHANGE].text, sizeof(item_howl_gen[INDEX_HGEN_AUTO_CHANGE].text),
		"Авто: %s", Howl.wave_gen.amp_wave.auto_change_en ? "ВКЛ" : "ВЫКЛ");
}

static void hgen_click_AUTO_CHANGE(void) {
	bool next = !Howl.wave_gen.amp_wave.auto_change_en;
	Howl.wave_gen.amp_wave.auto_change_en = next;
	Howl.wave_gen.freq_wave.auto_change_en = next;
	menu_howl_gen.field.needUpdate = 1;
}

static void hgen_click_RANDOMIZE(void) {
	Howl.wave_gen.Randomize();
	menu_howl_gen.field.needUpdate = 1;
}

static void hgen_param_click(void) {
	menu_howl_gen.field.encoder_block = !menu_howl_gen.field.encoder_block;
	menu_howl_gen.field.needUpdate = 1;
}

static void hgen_pre_menu(void) {
	Howl.Update();
}

void PAGE_Howl_Gen(void) {
	item_howl_gen[INDEX_HGEN_BACK].field.exit = 1;
	snprintf(item_howl_gen[INDEX_HGEN_BACK].text, sizeof(item_howl_gen[INDEX_HGEN_BACK].text), "Назад");

	item_howl_gen[INDEX_HGEN_AMP_SHAPE].preCallBackFunc = &hgen_pre_AMP_SHAPE;
	item_howl_gen[INDEX_HGEN_AMP_SHAPE].callBackFunc_isClick = &hgen_click_AMP_SHAPE;

	item_howl_gen[INDEX_HGEN_AMP_SPEED].preCallBackFunc = &hgen_pre_AMP_SPEED;
	item_howl_gen[INDEX_HGEN_AMP_SPEED].callBackFunc_isClick = &hgen_param_click;

	item_howl_gen[INDEX_HGEN_AMP_MIN].preCallBackFunc = &hgen_pre_AMP_MIN;
	item_howl_gen[INDEX_HGEN_AMP_MIN].callBackFunc_isClick = &hgen_param_click;

	item_howl_gen[INDEX_HGEN_AMP_MAX].preCallBackFunc = &hgen_pre_AMP_MAX;
	item_howl_gen[INDEX_HGEN_AMP_MAX].callBackFunc_isClick = &hgen_param_click;

	item_howl_gen[INDEX_HGEN_FREQ_SHAPE].preCallBackFunc = &hgen_pre_FREQ_SHAPE;
	item_howl_gen[INDEX_HGEN_FREQ_SHAPE].callBackFunc_isClick = &hgen_click_FREQ_SHAPE;

	item_howl_gen[INDEX_HGEN_FREQ_SPEED].preCallBackFunc = &hgen_pre_FREQ_SPEED;
	item_howl_gen[INDEX_HGEN_FREQ_SPEED].callBackFunc_isClick = &hgen_param_click;

	item_howl_gen[INDEX_HGEN_FREQ_MIN].preCallBackFunc = &hgen_pre_FREQ_MIN;
	item_howl_gen[INDEX_HGEN_FREQ_MIN].callBackFunc_isClick = &hgen_param_click;

	item_howl_gen[INDEX_HGEN_FREQ_MAX].preCallBackFunc = &hgen_pre_FREQ_MAX;
	item_howl_gen[INDEX_HGEN_FREQ_MAX].callBackFunc_isClick = &hgen_param_click;

	item_howl_gen[INDEX_HGEN_AUTO_CHANGE].preCallBackFunc = &hgen_pre_AUTO_CHANGE;
	item_howl_gen[INDEX_HGEN_AUTO_CHANGE].callBackFunc_isClick = &hgen_click_AUTO_CHANGE;

	snprintf(item_howl_gen[INDEX_HGEN_RANDOMIZE].text, sizeof(item_howl_gen[INDEX_HGEN_RANDOMIZE].text), "Рандом");
	item_howl_gen[INDEX_HGEN_RANDOMIZE].preCallBackFunc = NULL;
	item_howl_gen[INDEX_HGEN_RANDOMIZE].callBackFunc_isClick = &hgen_click_RANDOMIZE;

	for (int i = 0; i < NUM_ITEM_HOWL_GEN; i++) {
		item_howl_gen[i].text_color = -1;
	}

	menu_howl_gen.item_count = 6;
	menu_howl_gen.item_height = 36;
	menu_howl_gen.item_start_y = 0;
	menu_howl_gen.font = Roboto_Medium_en_ru_18;
	menu_howl_gen.field.verticalScroll = 1;
	menu_howl_gen.items = item_howl_gen;
	menu_howl_gen.ColorBackground = COLOR_BACKGROUND;
	menu_howl_gen.preCallBackFunc = &hgen_pre_menu;

	PAGE_Menu(&menu_howl_gen, item_howl_gen, NUM_ITEM_HOWL_GEN);
}

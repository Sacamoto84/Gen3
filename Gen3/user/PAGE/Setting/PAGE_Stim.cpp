#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "menu_typedef.h"
#include "global_typedef.h"
#include "PAGE_Stim.h"

extern Encoder_typedef Encoder;
extern void PAGE_Menu(menu_typedef *menu, item_typedef *item, int NUM);

#define INDEX_STIM_BACK        0
#define INDEX_STIM_MODE        1
#define INDEX_STIM_PARAM1      2
#define INDEX_STIM_PARAM2      3
#define INDEX_STIM_PARAM3      4
#define INDEX_STIM_PARAM4      5
#define INDEX_STIM_SAVE        6

#define NUM_ITEM_STIM          7

static menu_typedef menu_stim CCMRAM;
static item_typedef item_stim[NUM_ITEM_STIM] CCMRAM;

static const char *save_text = "Сохранить";

static void stim_mark_changed(void) {
	Gen.Reset_Stim_States();
	save_text = "Сохранить";
	menu_stim.field.needUpdate = 1;
}

static void stim_pre_MODE(uint32_t index) {
	switch (Gen.CH1.Mode) {
		case GEN_MODE_WAVELET:
			snprintf(item_stim[INDEX_STIM_MODE].text, sizeof(item_stim[INDEX_STIM_MODE].text), "Режим: Вейвлет");
			break;
		case GEN_MODE_MULTIPULSE:
			snprintf(item_stim[INDEX_STIM_MODE].text, sizeof(item_stim[INDEX_STIM_MODE].text), "Режим: MultiPulse");
			break;
		case GEN_MODE_CONTINUOUS:
		default:
			snprintf(item_stim[INDEX_STIM_MODE].text, sizeof(item_stim[INDEX_STIM_MODE].text), "Режим: Непрерыв");
			break;
	}
}

static void stim_MODE_click(void) {
	uint8_t next_mode = (Gen.CH1.Mode + 1) % 3;
	Gen.Mode_Set((GeneratorMode)next_mode);
	stim_mark_changed();
}

static void stim_pre_PARAM1(uint32_t index) {
	bool editing = (menu_stim.field.encoder_block && menu_stim.index == INDEX_STIM_PARAM1);

	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;

		if (delta) {
			if (Gen.CH1.Mode == GEN_MODE_WAVELET) {
				float v = Gen.CH1.Wavelet_Pulse_fr + (float)delta * 1.0F;
				if (v < 1.0F) v = 1.0F;
				if (v > 100.0F) v = 100.0F;
				Gen.CH1.Wavelet_Pulse_fr = v;
			} else if (Gen.CH1.Mode == GEN_MODE_MULTIPULSE) {
				float v = Gen.CH1.Multi_Pulse_fr + (float)delta * 1.0F;
				if (v < 1.0F) v = 1.0F;
				if (v > 200.0F) v = 200.0F;
				Gen.CH1.Multi_Pulse_fr = v;
			} else {
				int v = (int)Gen.CH1.Carrier_fr + delta * 100;
				if (v < 10) v = 10;
				if (v > 65535) v = 65535;
				Gen.CH1.Carrier_fr = (uint16_t)v;
			}
			stim_mark_changed();
		}
	}

	if (Gen.CH1.Mode == GEN_MODE_WAVELET) {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM1].text, sizeof(item_stim[INDEX_STIM_PARAM1].text), "> Пачки: %.1f Hz <", Gen.CH1.Wavelet_Pulse_fr);
		else
			snprintf(item_stim[INDEX_STIM_PARAM1].text, sizeof(item_stim[INDEX_STIM_PARAM1].text), "Пачки: %.1f Hz", Gen.CH1.Wavelet_Pulse_fr);
	} else if (Gen.CH1.Mode == GEN_MODE_MULTIPULSE) {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM1].text, sizeof(item_stim[INDEX_STIM_PARAM1].text), "> Чаcтота: %.1f Hz <", Gen.CH1.Multi_Pulse_fr);
		else
			snprintf(item_stim[INDEX_STIM_PARAM1].text, sizeof(item_stim[INDEX_STIM_PARAM1].text), "Чаcтота: %.1f Hz", Gen.CH1.Multi_Pulse_fr);
	} else {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM1].text, sizeof(item_stim[INDEX_STIM_PARAM1].text), "> Неcущая: %u Hz <", Gen.CH1.Carrier_fr);
		else
			snprintf(item_stim[INDEX_STIM_PARAM1].text, sizeof(item_stim[INDEX_STIM_PARAM1].text), "Неcущая: %u Hz", Gen.CH1.Carrier_fr);
	}
}

static void stim_pre_PARAM2(uint32_t index) {
	bool editing = (menu_stim.field.encoder_block && menu_stim.index == INDEX_STIM_PARAM2);

	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;

		if (delta) {
			if (Gen.CH1.Mode == GEN_MODE_WAVELET) {
				int v = (int)Gen.CH1.Wavelet_Cycles + delta;
				if (v < 1) v = 1;
				if (v > 20) v = 20;
				Gen.CH1.Wavelet_Cycles = (uint8_t)v;
			} else if (Gen.CH1.Mode == GEN_MODE_MULTIPULSE) {
				int v = (int)Gen.CH1.Multi_Pulse_us + delta * 25;
				if (v < 50) v = 50;
				if (v > 1000) v = 1000;
				Gen.CH1.Multi_Pulse_us = (uint16_t)v;
			} else {
				float v = Gen.CH1.AM_fr + (float)delta * 0.5F;
				if (v < 0.1F) v = 0.1F;
				if (v > 100.0F) v = 100.0F;
				Gen.CH1.AM_fr = v;
			}
			stim_mark_changed();
		}
	}

	if (Gen.CH1.Mode == GEN_MODE_WAVELET) {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM2].text, sizeof(item_stim[INDEX_STIM_PARAM2].text), "> Пачка: %u пер. <", Gen.CH1.Wavelet_Cycles);
		else
			snprintf(item_stim[INDEX_STIM_PARAM2].text, sizeof(item_stim[INDEX_STIM_PARAM2].text), "Пачка: %u пер.", Gen.CH1.Wavelet_Cycles);
	} else if (Gen.CH1.Mode == GEN_MODE_MULTIPULSE) {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM2].text, sizeof(item_stim[INDEX_STIM_PARAM2].text), "> Имп.: %u us <", Gen.CH1.Multi_Pulse_us);
		else
			snprintf(item_stim[INDEX_STIM_PARAM2].text, sizeof(item_stim[INDEX_STIM_PARAM2].text), "Имп.: %u us", Gen.CH1.Multi_Pulse_us);
	} else {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM2].text, sizeof(item_stim[INDEX_STIM_PARAM2].text), "> AM: %.1f Hz <", Gen.CH1.AM_fr);
		else
			snprintf(item_stim[INDEX_STIM_PARAM2].text, sizeof(item_stim[INDEX_STIM_PARAM2].text), "AM: %.1f Hz", Gen.CH1.AM_fr);
	}
}

static void stim_pre_PARAM3(uint32_t index) {
	bool editing = (menu_stim.field.encoder_block && menu_stim.index == INDEX_STIM_PARAM3);

	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;

		if (delta) {
			if (Gen.CH1.Mode == GEN_MODE_WAVELET) {
				int v = (int)Gen.CH1.Wavelet_Fade + delta * 5;
				if (v < 0) v = 0;
				if (v > 100) v = 100;
				Gen.CH1.Wavelet_Fade = (uint8_t)v;
			} else if (Gen.CH1.Mode == GEN_MODE_MULTIPULSE) {
				int v = (int)Gen.CH1.Multi_Intra_us + delta * 10;
				if (v < 0) v = 0;
				if (v > 200) v = 200;
				Gen.CH1.Multi_Intra_us = (uint16_t)v;
			} else {
				int v = (int)Gen.CH1.FM_Base + delta * 100;
				if (v < 10) v = 10;
				if (v > 65535) v = 65535;
				Gen.CH1.FM_Base = (uint16_t)v;
				Gen.Refresh_FM_Modulation1();
			}
			stim_mark_changed();
		}
	}

	if (Gen.CH1.Mode == GEN_MODE_WAVELET) {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM3].text, sizeof(item_stim[INDEX_STIM_PARAM3].text), "> Спад: %u %% <", Gen.CH1.Wavelet_Fade);
		else
			snprintf(item_stim[INDEX_STIM_PARAM3].text, sizeof(item_stim[INDEX_STIM_PARAM3].text), "Спад: %u %%", Gen.CH1.Wavelet_Fade);
	} else if (Gen.CH1.Mode == GEN_MODE_MULTIPULSE) {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM3].text, sizeof(item_stim[INDEX_STIM_PARAM3].text), "> Пауза: %u us <", Gen.CH1.Multi_Intra_us);
		else
			snprintf(item_stim[INDEX_STIM_PARAM3].text, sizeof(item_stim[INDEX_STIM_PARAM3].text), "Пауза: %u us", Gen.CH1.Multi_Intra_us);
	} else {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM3].text, sizeof(item_stim[INDEX_STIM_PARAM3].text), "> FM Base: %u <", Gen.CH1.FM_Base);
		else
			snprintf(item_stim[INDEX_STIM_PARAM3].text, sizeof(item_stim[INDEX_STIM_PARAM3].text), "FM Base: %u", Gen.CH1.FM_Base);
	}
}

static void stim_pre_PARAM4(uint32_t index) {
	bool editing = (menu_stim.field.encoder_block && menu_stim.index == INDEX_STIM_PARAM4);

	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;

		if (delta) {
			if (Gen.CH1.Mode == GEN_MODE_WAVELET) {
				int v = (int)Gen.CH1.Carrier_fr + delta * 100;
				if (v < 200) v = 200;
				if (v > 5000) v = 5000;
				Gen.CH1.Carrier_fr = (uint16_t)v;
			} else if (Gen.CH1.Mode == GEN_MODE_MULTIPULSE) {
				int v = (int)Gen.CH1.Multi_Count + delta;
				if (v < 1) v = 1;
				if (v > 10) v = 10;
				Gen.CH1.Multi_Count = (uint8_t)v;
			} else {
				int v = Gen.Volume_Get() + delta * 5;
				Gen.Volume_Set(v);
			}
			stim_mark_changed();
		}
	}

	if (Gen.CH1.Mode == GEN_MODE_WAVELET) {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM4].text, sizeof(item_stim[INDEX_STIM_PARAM4].text), "> Неcущая: %u Hz <", Gen.CH1.Carrier_fr);
		else
			snprintf(item_stim[INDEX_STIM_PARAM4].text, sizeof(item_stim[INDEX_STIM_PARAM4].text), "Неcущая: %u Hz", Gen.CH1.Carrier_fr);
	} else if (Gen.CH1.Mode == GEN_MODE_MULTIPULSE) {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM4].text, sizeof(item_stim[INDEX_STIM_PARAM4].text), "> Пачка: %u имп. <", Gen.CH1.Multi_Count);
		else
			snprintf(item_stim[INDEX_STIM_PARAM4].text, sizeof(item_stim[INDEX_STIM_PARAM4].text), "Пачка: %u имп.", Gen.CH1.Multi_Count);
	} else {
		if (editing)
			snprintf(item_stim[INDEX_STIM_PARAM4].text, sizeof(item_stim[INDEX_STIM_PARAM4].text), "> Громк.: %d %% <", Gen.Volume_Get());
		else
			snprintf(item_stim[INDEX_STIM_PARAM4].text, sizeof(item_stim[INDEX_STIM_PARAM4].text), "Громк.: %d %%", Gen.Volume_Get());
	}
}

static void stim_param_click(void) {
	menu_stim.field.encoder_block = !menu_stim.field.encoder_block;
	menu_stim.field.needUpdate = 1;
}

static void stim_pre_SAVE(uint32_t index) {
	snprintf(item_stim[INDEX_STIM_SAVE].text, sizeof(item_stim[INDEX_STIM_SAVE].text), "%s", save_text);
}

static void stim_save_click(void) {
	if (Gen.Config_Save()) {
		save_text = "Сохранено!";
	} else {
		save_text = "Ошибка запиcи";
	}
	menu_stim.field.needUpdate = 1;
}

void PAGE_Stim(void) {
	save_text = "Сохранить";

	item_stim[INDEX_STIM_BACK].field.exit = 1;
	snprintf(item_stim[INDEX_STIM_BACK].text, sizeof(item_stim[INDEX_STIM_BACK].text), "Назад");

	item_stim[INDEX_STIM_MODE].preCallBackFunc = &stim_pre_MODE;
	item_stim[INDEX_STIM_MODE].callBackFunc_isClick = &stim_MODE_click;

	item_stim[INDEX_STIM_PARAM1].preCallBackFunc = &stim_pre_PARAM1;
	item_stim[INDEX_STIM_PARAM1].callBackFunc_isClick = &stim_param_click;

	item_stim[INDEX_STIM_PARAM2].preCallBackFunc = &stim_pre_PARAM2;
	item_stim[INDEX_STIM_PARAM2].callBackFunc_isClick = &stim_param_click;

	item_stim[INDEX_STIM_PARAM3].preCallBackFunc = &stim_pre_PARAM3;
	item_stim[INDEX_STIM_PARAM3].callBackFunc_isClick = &stim_param_click;

	item_stim[INDEX_STIM_PARAM4].preCallBackFunc = &stim_pre_PARAM4;
	item_stim[INDEX_STIM_PARAM4].callBackFunc_isClick = &stim_param_click;

	item_stim[INDEX_STIM_SAVE].preCallBackFunc = &stim_pre_SAVE;
	item_stim[INDEX_STIM_SAVE].callBackFunc_isClick = &stim_save_click;

	for (int i = 0; i < NUM_ITEM_STIM; i++) {
		item_stim[i].text_color = -1;
	}

	menu_stim.item_count = 6;
	menu_stim.item_height = 36;
	menu_stim.item_start_y = 0;
	menu_stim.font = Roboto_Medium_en_ru_18;
	menu_stim.field.verticalScroll = 1;
	menu_stim.items = item_stim;
	menu_stim.ColorBackground = COLOR_BACKGROUND;
	menu_stim.preCallBackFunc = NULL;

	PAGE_Menu(&menu_stim, item_stim, NUM_ITEM_STIM);
}

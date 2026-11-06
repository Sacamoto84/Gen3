#include "PAGE_Howl.h"
#include "TFT_color.h"
#include "Page.h"
#include "global_define.h"
#include "global_typedef.h"
#include "menu_typedef.h"
#include "howl_core.h"
#include "fatfs.h"
#include "mString.h"
#include "Button.h"
#include <stdio.h>
#include <string.h>

extern Encoder_typedef Encoder;
extern button_typedef button;
extern void PAGE_Menu(menu_typedef *menu, item_typedef *item, int NUM);

#define INDEX_HPLAY_BACK     0
#define INDEX_HPLAY_FILE     1
#define INDEX_HPLAY_STATE    2
#define INDEX_HPLAY_TIME     3
#define INDEX_HPLAY_SPEED    4
#define INDEX_HPLAY_LOOP     5
#define INDEX_HPLAY_STOP     6

#define NUM_ITEM_HOWL_PLAY   7

static menu_typedef menu_howl_play CCMRAM;
static item_typedef item_howl_play[NUM_ITEM_HOWL_PLAY] CCMRAM;

static void File_Selector(void) {
	DIR dir;
	FILINFO fno;
	mString<32> sFiles[24];
	int countFiles = 1;

	sFiles[0] = "<- Назад";

	FRESULT res = f_opendir(&dir, "/howl");
	if (res != FR_OK) {
		res = f_opendir(&dir, "/");
	}

	if (res == FR_OK) {
		for (;;) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0 || countFiles >= 24) break;
			int len = strlen(fno.fname);
			if (len > 4) {
				const char* ext = &fno.fname[len - 4];
				if (strcasecmp(ext, ".hwl") == 0 || (len > 5 && strcasecmp(&fno.fname[len - 5], ".howl") == 0)) {
					sFiles[countFiles++] = fno.fname;
				}
			}
		}
		f_closedir(&dir);
	}

	int window_start = 0;
	int window_size = 5;
	int index = 0;
	bool need_redraw = true;
	char str[48];

	Font_Smooth_Load(Roboto_Medium_en_ru_18);

	while (1) {
		button.tick();
		Howl.Update();

		if (Encoder.Left) {
			Encoder.Left = 0;
			index--;
			if (index < 0) index = 0;
			if (index < window_start) window_start = index;
			need_redraw = true;
		}

		if (Encoder.Right) {
			Encoder.Right = 0;
			index++;
			if (index >= countFiles) index = countFiles - 1;
			if (index >= window_start + window_size) window_start = index - window_size + 1;
			need_redraw = true;
		}

		if (need_redraw) {
			need_redraw = false;

			tft.Fill16(COLOR_BACKGROUND);

			tft.RectangleFilled(0, 0, 240, 24, RGB565(20, 30, 50));
			Font_Smooth_drawStr(&tft, 8, 4, (char*)"ВЫБОР ФАЙЛА HOWL", RGB565(100, 200, 255));

			for (int i = 0; i < window_size; i++) {
				int item_idx = window_start + i;
				if (item_idx >= countFiles) break;

				int y = 30 + i * 40;
				if (item_idx == index) {
					tft.RectangleFilled(0, y, 239, 36, COLOR_RECTAGLE);
				}

				snprintf(str, sizeof(str), "%s", sFiles[item_idx].buf);
				uint16_t color = (item_idx == index) ? COLOR_TEXT_SELECT : COLOR_TEXT_DEFAULT;
				Font_Smooth_drawStr(&tft, 10, y + 8, str, color);
			}

			tft.driver.ST7789_UpdateDMA16bitV3();
		}

		if (button.isClick()) {
			button.isHolded();
			if (index > 0) {
				char path[64];
				snprintf(path, sizeof(path), "/howl/%s", sFiles[index].buf);
				if (!Howl.player.LoadFile(path)) {
					snprintf(path, sizeof(path), "/%s", sFiles[index].buf);
					Howl.player.LoadFile(path);
				}
				Howl.SetSource(HOWL_SOURCE_PLAYER);
				Howl.player.Play();
			}
			return;
		}

		if (button.isHolded()) return;
	}
}

static void hplay_pre_FILE(uint32_t index) {
	if (Howl.player.is_loaded) {
		const char* slash = strrchr(Howl.player.current_file, '/');
		const char* name = slash ? (slash + 1) : Howl.player.current_file;
		snprintf(item_howl_play[INDEX_HPLAY_FILE].text, sizeof(item_howl_play[INDEX_HPLAY_FILE].text),
			"%.19s", name);
	} else {
		snprintf(item_howl_play[INDEX_HPLAY_FILE].text, sizeof(item_howl_play[INDEX_HPLAY_FILE].text),
			"Файл: [...]");
	}
}

static void hplay_click_FILE(void) {
	File_Selector();
	menu_howl_play.field.needUpdate = 1;
}

static void hplay_pre_STATE(uint32_t index) {
	snprintf(item_howl_play[INDEX_HPLAY_STATE].text, sizeof(item_howl_play[INDEX_HPLAY_STATE].text),
		"Плеер: %s", Howl.player.is_playing ? "PLAY" : "PAUSE");
}

static void hplay_click_STATE(void) {
	if (Howl.player.is_playing) {
		Howl.player.Pause();
	} else {
		Howl.SetSource(HOWL_SOURCE_PLAYER);
		Howl.player.Play();
	}
	menu_howl_play.field.needUpdate = 1;
}

static void hplay_pre_TIME(uint32_t index) {
	uint32_t cur = Howl.player.GetCurrentSec();
	uint32_t tot = Howl.player.GetTotalSec();
	snprintf(item_howl_play[INDEX_HPLAY_TIME].text, sizeof(item_howl_play[INDEX_HPLAY_TIME].text),
		"%02u:%02u / %02u:%02u",
		(unsigned)((cur / 60) % 100), (unsigned)(cur % 60),
		(unsigned)((tot / 60) % 100), (unsigned)(tot % 60));
}

static void hplay_pre_SPEED(uint32_t index) {
	bool editing = (menu_howl_play.field.encoder_block && menu_howl_play.index == INDEX_HPLAY_SPEED);
	if (editing) {
		int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;
		if (delta) {
			float s = Howl.player.speed_scale + delta * 0.1f;
			if (s < 0.2f) s = 0.2f;
			if (s > 3.0f) s = 3.0f;
			Howl.player.speed_scale = s;
			menu_howl_play.field.needUpdate = 1;
		}
		snprintf(item_howl_play[INDEX_HPLAY_SPEED].text, sizeof(item_howl_play[INDEX_HPLAY_SPEED].text),
			"> Скор: %.1fx <", Howl.player.speed_scale);
	} else {
		snprintf(item_howl_play[INDEX_HPLAY_SPEED].text, sizeof(item_howl_play[INDEX_HPLAY_SPEED].text),
			"Скор: %.1fx", Howl.player.speed_scale);
	}
}

static void hplay_pre_LOOP(uint32_t index) {
	snprintf(item_howl_play[INDEX_HPLAY_LOOP].text, sizeof(item_howl_play[INDEX_HPLAY_LOOP].text),
		"Цикл: %s", Howl.player.is_loop ? "ВКЛ" : "ВЫКЛ");
}

static void hplay_click_LOOP(void) {
	Howl.player.is_loop = !Howl.player.is_loop;
	menu_howl_play.field.needUpdate = 1;
}

static void hplay_click_STOP(void) {
	Howl.player.Stop();
	menu_howl_play.field.needUpdate = 1;
}

static void hplay_param_click(void) {
	menu_howl_play.field.encoder_block = !menu_howl_play.field.encoder_block;
	menu_howl_play.field.needUpdate = 1;
}

static void hplay_pre_menu(void) {
	Howl.Update();
}

void PAGE_Howl_Player(void) {
	item_howl_play[INDEX_HPLAY_BACK].field.exit = 1;
	snprintf(item_howl_play[INDEX_HPLAY_BACK].text, sizeof(item_howl_play[INDEX_HPLAY_BACK].text), "Назад");

	item_howl_play[INDEX_HPLAY_FILE].preCallBackFunc = &hplay_pre_FILE;
	item_howl_play[INDEX_HPLAY_FILE].callBackFunc_isClick = &hplay_click_FILE;

	item_howl_play[INDEX_HPLAY_STATE].preCallBackFunc = &hplay_pre_STATE;
	item_howl_play[INDEX_HPLAY_STATE].callBackFunc_isClick = &hplay_click_STATE;

	item_howl_play[INDEX_HPLAY_TIME].preCallBackFunc = &hplay_pre_TIME;
	item_howl_play[INDEX_HPLAY_TIME].callBackFunc_isClick = NULL;

	item_howl_play[INDEX_HPLAY_SPEED].preCallBackFunc = &hplay_pre_SPEED;
	item_howl_play[INDEX_HPLAY_SPEED].callBackFunc_isClick = &hplay_param_click;

	item_howl_play[INDEX_HPLAY_LOOP].preCallBackFunc = &hplay_pre_LOOP;
	item_howl_play[INDEX_HPLAY_LOOP].callBackFunc_isClick = &hplay_click_LOOP;

	snprintf(item_howl_play[INDEX_HPLAY_STOP].text, sizeof(item_howl_play[INDEX_HPLAY_STOP].text), "Стоп");
	item_howl_play[INDEX_HPLAY_STOP].preCallBackFunc = NULL;
	item_howl_play[INDEX_HPLAY_STOP].callBackFunc_isClick = &hplay_click_STOP;

	for (int i = 0; i < NUM_ITEM_HOWL_PLAY; i++) {
		item_howl_play[i].text_color = -1;
	}

	menu_howl_play.item_count = 6;
	menu_howl_play.item_height = 36;
	menu_howl_play.item_start_y = 0;
	menu_howl_play.font = Roboto_Medium_en_ru_18;
	menu_howl_play.field.verticalScroll = 1;
	menu_howl_play.items = item_howl_play;
	menu_howl_play.ColorBackground = COLOR_BACKGROUND;
	menu_howl_play.preCallBackFunc = &hplay_pre_menu;

	PAGE_Menu(&menu_howl_play, item_howl_play, NUM_ITEM_HOWL_PLAY);
}

#include "PAGE_Howl.h"
#include "TFT_color.h"
#include "Page.h"
#include "global_define.h"
#include "global_typedef.h"
#include "menu_typedef.h"
#include "FontSmooth.h"
#include "Button.h"
#include "scripting.h"
#include "generator.h"
#include <stdio.h>

extern Encoder_typedef Encoder;
extern button_typedef button;
extern Scripting script;

#define HOWL_MAIN_NUM_ITEMS 7

static const char* menu_item_titles[HOWL_MAIN_NUM_ITEMS] = {
	"<- Выход (Legacy)",
	"Источник",
	"Мощность",
	"Генератор волн ->",
	"Активности ->",
	"Плеер ->",
	"Настройки ->"
};

void PAGE_Howl_Main(void) {
	// ════════════════════════════════════════════════════════════════
	// ВХОД В HOWL РЕЖИМ: изоляция от Legacy генератора и скриптов
	// ════════════════════════════════════════════════════════════════
	const bool legacy_script_was_waiting = script.waiting;
	const bool legacy_gen_en = Gen.CH1.CH_EN;

	// Приостанавливаем скрипт и активируем Howl синтез
	script.waiting = true;
	Howl.Start();

	Font_Smooth_Load(Roboto_Medium_en_ru_18);

	int8_t index = 1;
	int window_start = 0;
	const int window_size = 4;
	bool editing_power = false;
	uint32_t last_render_time = 0;

	char str[48];

	while (1) {
		button.tick();

		// Обработка длительного нажатия: быстрый MUTE / UNMUTE (безопасность!)
		if (button.isHolded()) {
			Howl.ToggleMute();
		}

		// Опрос энкодера
		if (editing_power) {
			int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
			Encoder.Left = 0;
			Encoder.Right = 0;
			if (delta != 0) {
				int next_p = (int)Howl.settings.power_a + delta * (int)Howl.settings.power_step;
				Howl.SetPower(next_p);
			}
		} else {
			if (Encoder.Left) {
				Encoder.Left = 0;
				index--;
				if (index < 0) index = HOWL_MAIN_NUM_ITEMS - 1;
				if (index < window_start) {
					window_start = index;
				} else if (index >= window_start + window_size) {
					window_start = index - window_size + 1;
				}
			}
			if (Encoder.Right) {
				Encoder.Right = 0;
				index++;
				if (index >= HOWL_MAIN_NUM_ITEMS) index = 0;
				if (index >= window_start + window_size) {
					window_start = index - window_size + 1;
				} else if (index < window_start) {
					window_start = index;
				}
			}
		}

		// Обработка одиночного клика
		if (button.isClick()) {
			button.isHolded();

			if (editing_power) {
				editing_power = false;
			} else {
				switch (index) {
					case 0: // Выход в Legacy режим
						Howl.Stop();
						script.waiting = legacy_script_was_waiting;
						Gen.CH1.CH_EN = legacy_gen_en;
						tft.driver.needUpdate = 1;
						return;

					case 1: // Переключение источника
						{
							uint8_t next_s = (Howl.source + 1) % 3;
							Howl.SetSource((HowlSource)next_s);
						}
						break;

					case 2: // Редактирование мощности
						editing_power = true;
						break;

					case 3: // Волновой генератор
						PAGE_Howl_Gen();
						Font_Smooth_Load(Roboto_Medium_en_ru_18);
						break;

					case 4: // Меню активностей
						PAGE_Howl_Act();
						Font_Smooth_Load(Roboto_Medium_en_ru_18);
						break;

					case 5: // Плеер файлов
						PAGE_Howl_Player();
						Font_Smooth_Load(Roboto_Medium_en_ru_18);
						break;

					case 6: // Настройки Howl
						PAGE_Howl_Settings();
						Font_Smooth_Load(Roboto_Medium_en_ru_18);
						break;
				}
			}
		}

		// Отрисовка кадра (каждые 40-50 мс для плавного отображения 40 Гц бара)
		uint32_t now = HAL_GetTick();
		if (now - last_render_time >= 40) {
			last_render_time = now;

			tft.Fill16(COLOR_BACKGROUND);

			// ── ВЕРХНЯЯ ШАПКА (HUD Bar) ──────────────────────────────
			tft.RectangleFilled(0, 0, 240, 24, RGB565(15, 25, 45));

			Font_Smooth_drawStr(&tft, 8, 4, (char*)"HOWL РЕЖИМ", RGB565(100, 200, 255));

			// Статус MUTE или Источник в правом углу шапки
			if (Howl.settings.is_muted) {
				tft.RectangleFilled(165, 3, 70, 18, RGB565(200, 20, 20));
				Font_Smooth_drawStr(&tft, 175, 4, (char*)"MUTE", RGB565(255, 255, 255));
			} else {
				snprintf(str, sizeof(str), "[%s]", Howl.telemetry.source_name);
				Font_Smooth_drawStr(&tft, 140, 4, str, RGB565(180, 220, 180));
			}

			// ── КАРТОЧКА МОЩНОСТИ И ПУЛЬСА (Y: 28..96) ─────────────
			tft.RectangleFilled(4, 28, 232, 68, RGB565(25, 30, 45));

			// Строка мощности
			if (editing_power) {
				snprintf(str, sizeof(str), "> Мощн: %u / %u <", Howl.settings.power_a, Howl.settings.power_limit);
				Font_Smooth_drawStr(&tft, 10, 32, str, RGB565(255, 220, 50));
			} else {
				snprintf(str, sizeof(str), "Мощн: %u / %u  %s",
					Howl.settings.power_a, Howl.settings.power_limit,
					Howl.settings.auto_inc_en ? "Auto+" : "");
				Font_Smooth_drawStr(&tft, 10, 32, str, RGB565(240, 240, 240));
			}

			// Строка частоты и типа
			snprintf(str, sizeof(str), "%.1f Hz  Amp:%u%%  %s",
				Howl.telemetry.current_freq_hz,
				(uint8_t)(Howl.telemetry.current_amp * 100.0f),
				Howl.telemetry.activity_name);
			Font_Smooth_drawStr(&tft, 10, 52, str, RGB565(80, 230, 130));

			// ── ДИНАМИЧЕСКИЙ 40 Гц БАР ИНТЕНСИВНОСТИ И ЧАСТОТЫ ────
			const int bar_x = 10;
			const int bar_y = 74;
			const int bar_w = 220;
			const int bar_h = 16;

			// Фон индикатора
			tft.RectangleFilled(bar_x, bar_y, bar_w, bar_h, RGB565(12, 16, 24));

			// Заполнение бара пропорционально мгновенному импульсу
			int fill_w = (Howl.telemetry.meter_bar * bar_w) / 100;
			if (fill_w > bar_w) fill_w = bar_w;

			// Цвет бара зависит от текущей частоты (как в Android приложении)
			uint16_t bar_color;
			if (Howl.telemetry.current_freq_hz < 25.0f) {
				bar_color = RGB565(0, 190, 240);   // Голубой (глубокие сокращения)
			} else if (Howl.telemetry.current_freq_hz < 60.0f) {
				bar_color = RGB565(40, 220, 90);   // Ярко-зеленый (рабочая стимуляция)
			} else {
				bar_color = RGB565(250, 130, 30);  // Оранжевый/красный (вибрация/щекотка)
			}

			if (fill_w > 0 && !Howl.settings.is_muted && Howl.settings.power_a > 0) {
				tft.RectangleFilled(bar_x, bar_y, fill_w, bar_h, bar_color);
			}
			tft.Rectangle(bar_x, bar_y, bar_w, bar_h, RGB565(70, 80, 100));

			// ── МЕНЮ ВЫБОРА (Y: 102..238) ─────────────────────────
			const int item_h = 34;
			const int menu_y_start = 102;

			for (int i = 0; i < window_size; i++) {
				int item_idx = window_start + i;
				if (item_idx >= HOWL_MAIN_NUM_ITEMS) break;

				int y = menu_y_start + i * item_h;

				// Подсветка выбранного пункта
				if (item_idx == index) {
					tft.RectangleFilled(0, y, 239, item_h - 2, COLOR_RECTAGLE);
				}

				// Текст пункта
				if (item_idx == 1) {
					snprintf(str, sizeof(str), "Источник: %s", Howl.telemetry.source_name);
				} else if (item_idx == 2) {
					snprintf(str, sizeof(str), "Мощность: %u", Howl.settings.power_a);
				} else {
					snprintf(str, sizeof(str), "%s", menu_item_titles[item_idx]);
				}

				uint16_t text_color = (item_idx == index) ? COLOR_TEXT_SELECT : COLOR_TEXT_DEFAULT;
				Font_Smooth_drawStr(&tft, 12, y + 6, str, text_color);
			}

			// Скролл-бар справа
			if (HOWL_MAIN_NUM_ITEMS > window_size) {
				tft.RectangleFilled(234, menu_y_start, 4, window_size * item_h - 2, RGB565(30, 35, 45));
				int scroll_h = (window_size * item_h - 2) * window_size / HOWL_MAIN_NUM_ITEMS;
				int scroll_y = menu_y_start + ((window_size * item_h - 2 - scroll_h) * window_start) / (HOWL_MAIN_NUM_ITEMS - window_size);
				tft.RectangleFilled(234, scroll_y, 4, scroll_h, RGB565(180, 190, 210));
			}

			tft.driver.ST7789_UpdateDMA16bitV3();
		}
	}
}

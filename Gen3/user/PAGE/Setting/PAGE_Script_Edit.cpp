#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "strings.h"

#include "menu_typedef.h"
#include "scripting.h"
#include "fatfs.h"

#include "global_typedef.h"
extern Encoder_typedef Encoder;

#include "Button.h"
extern button_typedef button;

extern Scripting script;

extern uint16_t view_line_color(const char * s);

//В шрифте 18 русская строчная с рисуется как ё, поэтому в надписях ниже стоит латинская c
#pragma GCC optimize ("Os") //Экранам скорость не нужна, а flash мало: с -Ofast файл занимает 19 КБ

//┌─────────────────────────────────────────────────────────┐
	#define EDIT_ROW_H      21                            //│
	#define EDIT_ROWS       10                            //│ Плюс строка заголовка
	#define EDIT_TEXT_DY    3                             //│ Как на рабочем экране
	#define EDIT_NUMBER_W   34                            //│ Ширина колонки номера
	#define EDIT_TEXT_MAX   (CMD_LEN - 2)                 //│ Без \n и нуля, движок добавит \n
	#define EDIT_GRID_W     60                            //│ Клетка сетки
	#define EDIT_GRID_H     54                            //│
	#define EDIT_GRID_COLS  4                             //│
	#define EDIT_DIGIT_W    28                            //│ Клетка разряда шрифтом 30
	#define EDIT_NAME_MAX   14                            //│ "FM1 MOD " + имя влезает в строку
	#define EDIT_FILES_MAX  32                            //│
	#define EDIT_WORDS      5                             //│ IF F1 <= 123 это 4 слова
	#define EDIT_NEW_NAME   "Новый"                         //│ Скрипт без файла
	#define EDIT_KB_W       40                            //│ Клетка клавиатуры имени
	#define EDIT_KB_H       30                            //│
	#define EDIT_KB_COLS    6                             //│
	#define EDIT_KB_CHARS   36                            //│ Буквы и цифры, за ними три широкие клавиши
	#define EDIT_KB_DEL     (EDIT_KB_CHARS + 0)           //│
	#define EDIT_KB_OK      (EDIT_KB_CHARS + 1)           //│
	#define EDIT_KB_BACK    (EDIT_KB_CHARS + 2)           //│
	#define EDIT_KB_TOTAL   (EDIT_KB_CHARS + 3)           //│
//└─────────────────────────────────────────────────────────┘

#define EDIT_WORD_SIZE (EDIT_NAME_MAX + 1)

enum { EDIT_KEY_NONE, EDIT_KEY_CLICK, EDIT_KEY_HOLD };

//Черновик: нумерация как в script.list, в [0] имя файла, пустое у нового скрипта. Строки без \n
static char edit_list[PC_MAX][CMD_LEN] CCMRAM;
static char edit_files[EDIT_FILES_MAX][EDIT_WORD_SIZE] CCMRAM;
static int  edit_lines CCMRAM;     //Сколько записей вместе с именем
static int  edit_cursor CCMRAM;    //edit_lines это строка "+ в конец"
static int  edit_first CCMRAM;     //Первая строка окна
static int  edit_menu_pos CCMRAM;  //Прошлое действие меню строки, чтобы вставлять подряд кликами
static bool edit_modified CCMRAM;
static bool edit_active CCMRAM;    //Редактор открыт, из него запущено окно просмотра

//────────────────────────── Ввод и рисование ──────────────────────────────

static void edit_font(void)
{
	Font_Smooth_Load(Roboto_Medium_en_ru_18);
	tft.uTFT.GetColor = 1;
}

//Один опрос. Скрипт продолжает работать, пока идет редактирование
static int edit_poll(int * delta)
{
	script.run();

	button.tick();

	*delta = 0;
	if (Encoder.Left)  { Encoder.Left = 0;  *delta = -1; }
	if (Encoder.Right) { Encoder.Right = 0; *delta = 1; }

	if (button.isHolded()) return EDIT_KEY_HOLD;
	if (button.isClick())  return EDIT_KEY_CLICK;
	return EDIT_KEY_NONE;
}

//Символов, а не байт: кириллица занимает два байта
static int edit_text_chars(const char * text)
{
	int chars = 0;
	for (; *text; text++)
		if ((*text & 0xC0) != 0x80) chars++;
	return chars;
}

static void edit_frame(const char * title, const char * right)
{
	tft.Fill16(COLOR_BACKGROUND);
	tft.RectangleFilled(0, 0, 240, EDIT_ROW_H, RGB565(40, 40, 70));
	Font_Smooth_drawStr(&tft, 4, EDIT_TEXT_DY, title, COLOR_TEXT_DEFAULT);
	if (right)
		Font_Smooth_drawStr(&tft, 236 - 10 * edit_text_chars(right), EDIT_TEXT_DY, right, COLOR_TEXT_DEFAULT);
}

//Вертикальный список. Индекс строки, по удержанию -1
static int edit_pick_list(const char * title, const char * const * rows, int count, int index)
{
	int first = 0;
	bool redraw = true;

	while (1) {
		int delta;
		const int key = edit_poll(&delta);

		if (delta) {
			index += delta;
			if (index < 0) index = 0;
			if (index >= count) index = count - 1;
			redraw = true;
		}

		if (key == EDIT_KEY_HOLD)  return -1;
		if (key == EDIT_KEY_CLICK) return index;

		if (redraw) {
			redraw = false;

			if (index < first) first = index;
			if (index >= first + EDIT_ROWS) first = index - EDIT_ROWS + 1;

			edit_frame(title, NULL);

			for (int row = 0; (row < EDIT_ROWS) && (first + row < count); row++) {
				const int y = EDIT_ROW_H * (row + 1);
				const bool current = (first + row == index);
				if (current)
					tft.RectangleFilled(0, y, 240, EDIT_ROW_H, COLOR_RECTAGLE);
				Font_Smooth_drawStr(&tft, 4, y + EDIT_TEXT_DY, rows[first + row], current ? COLOR_TEXT_SELECT : COLOR_TEXT_DEFAULT);
			}

			tft.driver.ST7789_UpdateDMA16bitV3();
		}
	}
}

//Сообщение, ждем любую кнопку
static void edit_message(const char * text)
{
	static const char * const rows[] = { "OK" };
	edit_pick_list(text, rows, 1, 0);
}

//Сетка как в Android, последняя клетка всегда Назад. Индекс клетки, назад и удержание -1
static int edit_pick_grid(const char * title, const char * const * cells, int count, int index)
{
	const int total = count + 1;
	bool redraw = true;

	while (1) {
		int delta;
		const int key = edit_poll(&delta);

		if (delta) {
			index += delta;
			if (index < 0) index = 0;
			if (index >= total) index = total - 1;
			redraw = true;
		}

		if (key == EDIT_KEY_HOLD)  return -1;
		if (key == EDIT_KEY_CLICK) return (index == count) ? -1 : index;

		if (redraw) {
			redraw = false;

			edit_frame(title, NULL);

			for (int i = 0; i < total; i++) {
				const char * text = (i == count) ? "Назад" : cells[i];
				const bool current = (i == index);
				const int x = (i % EDIT_GRID_COLS) * EDIT_GRID_W;
				const int y = EDIT_ROW_H + 1 + (i / EDIT_GRID_COLS) * EDIT_GRID_H;

				tft.RectangleFilled(x + 1, y + 1, EDIT_GRID_W - 2, EDIT_GRID_H - 2, current ? COLOR_RECTAGLE : RGB565(40, 40, 70));

				Font_Smooth_drawStr(&tft, x + (EDIT_GRID_W - edit_text_chars(text) * 10) / 2, y + 16, text, current ? COLOR_TEXT_SELECT : COLOR_TEXT_DEFAULT);
			}

			tft.driver.ST7789_UpdateDMA16bitV3();
		}
	}
}

//────────────────────────── Ввод числа ────────────────────────────────────

//Значения в единицах младшего разряда: при frac 1 это десятые, при frac 3 тысячные
typedef struct {
	int32_t min;
	int32_t max;
	int32_t def;
	uint8_t frac;
	const char * unit;
} edit_range;

static const edit_range range_cr_fr  = {  200,  10000,  1000, 0, "Гц" };
static const edit_range range_am_fr  = {    1,   1000,   100, 1, "Гц" };
static const edit_range range_fm_base= {  200,   5000,  2500, 0, "Гц" };
static const edit_range range_fm_dev = {    1,  50000,  2000, 1, "Гц" };
static const edit_range range_fm_fr  = {    1,   1000,   100, 1, "Гц" };
static const edit_range range_delay  = {    0,  99999,  1000, 0, "мc" };
static const edit_range range_float  = {    0, 999999,     0, 1, NULL };
static const edit_range range_mul    = {    1,  99999,  1000, 3, NULL };

static int32_t edit_scale(int frac)
{
	int32_t scale = 1;
	while (frac-- > 0) scale *= 10;
	return scale;
}

//Без лишних нулей после точки, как в готовых скриптах
static void edit_format(char * out, int size, int32_t value, int frac)
{
	const int32_t scale = edit_scale(frac);
	int len = snprintf(out, size, "%ld", (long) (value / scale));
	int32_t rest = value % scale;

	if (rest) {
		out[len++] = '.';
		for (int32_t div = scale / 10; rest && (len < size - 1); div /= 10) {
			out[len++] = '0' + rest / div;
			rest %= div;
		}
		out[len] = 0;
	}
}

static int32_t edit_parse(const char * text, int frac)
{
	return (int32_t) (atof(text) * edit_scale(frac) + 0.5);
}

static void edit_number_draw(const char * title, const edit_range * r, int32_t value, int digits, int cursor, int32_t step, bool preview)
{
	char str[32];

	edit_frame(title, r->unit);

	edit_format(str, sizeof(str), step, r->frac);
	char text[48];
	snprintf(text, sizeof(text), "Шаг %s", str);
	Font_Smooth_drawStr(&tft, 80, 100, text, COLOR_TEXT_DISABLE);

	if (preview) {
		snprintf(text, sizeof(text), "-> %s", ((value >= 1) && (value < edit_lines)) ? edit_list[value] : "");
		Font_Smooth_drawStr(&tft, 4, 128, text, view_line_color(edit_list[value]));
	}

	Font_Smooth_drawStr(&tft, 4, 156, "Вращение: значение", COLOR_TEXT_DISABLE);
	Font_Smooth_drawStr(&tft, 4, 177, "Клик: другой разряд", COLOR_TEXT_DISABLE);
	Font_Smooth_drawStr(&tft, 4, 198, "Удержание: готово", COLOR_TEXT_DISABLE);

	//Разряды шрифтом 30. Загрузка быстрая, а кадр рисуется только при изменении
	Font_Smooth_Load(Roboto_Medium_en_ru_30);

	int x = (240 - digits * EDIT_DIGIT_W - (r->frac ? 10 : 0)) / 2;
	int32_t div = 1;
	for (int i = 1; i < digits; i++) div *= 10;

	for (int i = 0; i < digits; i++) {
		if (r->frac && (i == digits - r->frac)) {
			Font_Smooth_drawStr(&tft, x, 50, ".", COLOR_TEXT_DEFAULT);
			x += 10;
		}

		const bool current = (i == cursor);
		if (current)
			tft.RectangleFilled(x, 44, EDIT_DIGIT_W, 40, COLOR_RECTAGLE);

		str[0] = '0' + (value / div) % 10;
		str[1] = 0;
		Font_Smooth_drawStr(&tft, x + 5, 50, str, current ? COLOR_TEXT_SELECT : COLOR_TEXT_DEFAULT);

		x += EDIT_DIGIT_W;
		div /= 10;
	}

	Font_Smooth_Load(Roboto_Medium_en_ru_18);

	tft.driver.ST7789_UpdateDMA16bitV3();
}

//Ввод по разрядам с переносом. Удержание подтверждает, отмены нет: строка правится заново
static int32_t edit_pick_number(const char * title, const edit_range * r, int32_t value, bool preview)
{
	if (value < r->min) value = r->min;
	if (value > r->max) value = r->max;

	int digits = 1;
	for (int32_t p = 10; p <= r->max; p *= 10) digits++;
	if (digits < r->frac + 1) digits = r->frac + 1;

	//Начинаем со старшего значащего разряда, но не ниже единиц
	int cursor = 0;
	int32_t step = 1;
	for (int i = 1; i < digits; i++) step *= 10;
	while ((cursor < digits - 1 - r->frac) && (value / step == 0)) {
		cursor++;
		step /= 10;
	}

	bool redraw = true;

	while (1) {
		int delta;
		const int key = edit_poll(&delta);

		if (delta) {
			value += delta * step;
			if (value < r->min) value = r->min;
			if (value > r->max) value = r->max;
			redraw = true;
		}

		if (key == EDIT_KEY_HOLD)
			return value;

		if (key == EDIT_KEY_CLICK) {
			cursor++;
			step /= 10;
			if (cursor >= digits) {
				cursor = 0;
				step = 1;
				for (int i = 1; i < digits; i++) step *= 10;
			}
			redraw = true;
		}

		if (redraw) {
			redraw = false;
			edit_number_draw(title, r, value, digits, cursor, step, preview);
		}
	}
}

//────────────────────────── Файлы ─────────────────────────────────────────

//Имена с расширением ext в edit_files без него. Длинные как короткое 8.3
static int edit_read_dir(const char * path, const char * ext)
{
	DIR dir;
	FILINFO fno;
	int count = 0;

	if (f_opendir(&dir, path) != FR_OK)
		return 0;

	while ((count < EDIT_FILES_MAX) && (f_readdir(&dir, &fno) == FR_OK) && fno.fname[0]) {
		if (fno.fattrib & AM_DIR)
			continue;

		const char * name = fno.fname;
		if ((strlen(name) > EDIT_NAME_MAX + strlen(ext)) && fno.altname[0])
			name = fno.altname;

		const char * dot = strrchr(name, '.');
		if ((dot == NULL) || (strcasecmp(dot, ext) != 0) || (dot - name > EDIT_NAME_MAX))
			continue;

		memcpy(edit_files[count], name, dot - name);
		edit_files[count][dot - name] = 0;
		count++;
	}

	f_closedir(&dir);
	return count;
}

//Выбор из папки. Имя в out, false если назад или файлов нет
static bool edit_pick_file(const char * title, const char * path, const char * ext, const char * hint, char * out)
{
	const char * rows[EDIT_FILES_MAX];
	const int count = edit_read_dir(path, ext);

	if (count == 0) {
		edit_message("Нет файлов");
		return false;
	}

	int index = 0;
	for (int i = 0; i < count; i++) {
		rows[i] = edit_files[i];
		if (strcmp(edit_files[i], hint) == 0)
			index = i;
	}

	index = edit_pick_list(title, rows, count, index);
	if (index < 0)
		return false;

	snprintf(out, EDIT_WORD_SIZE, "%s", edit_files[index]);
	return true;
}

//────────────────────────── Сборка команды ────────────────────────────────

//В сетке 16 клеток с Назад. Новые команды идут во вторую сетку, старые блоки остаются на местах
static const char * const edit_blocks[] = { "CH1", "CR1", "AM1", "FM1", "GOTO", "IF", "ELSE", "ENDIF", "LOAD", "PLUS", "MINUS", "DELAY", "YIELD", "END", "Еще" };
static const char * const edit_more[]   = { "MUL", "WAIT" };
static const char * const edit_ch_parts[] = { "CR", "AM", "FM" };
static const char * const edit_on_off[]   = { "ON", "OFF" };
static const char * const edit_cr_am[]    = { "FR", "MOD" };
static const char * const edit_fm[]       = { "BASE", "DEV", "FR", "MOD" };
static const char * const edit_compare[]  = { "<", "<=", ">", ">=", "==", "!=" };
static const char * const edit_value[]    = { "Чиcло", "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9" };

#define COUNT_OF(a) ((int) (sizeof(a) / sizeof((a)[0])))

typedef enum { STEP_DONE, STEP_BLOCK, STEP_WORDS, STEP_REG, STEP_NUMBER, STEP_GOTO, STEP_VALUE, STEP_FILE } edit_step_kind;

typedef struct {
	edit_step_kind kind;
	const char * const * words;
	int count;
	edit_range range;
	const char * dir;
} edit_step;

static edit_step step_words(const char * const * words, int count)
{
	edit_step s = {};
	s.kind = STEP_WORDS;
	s.words = words;
	s.count = count;
	return s;
}

static edit_step step_kind(edit_step_kind kind, const edit_range * range)
{
	edit_step s = {};
	s.kind = kind;
	if (range) s.range = *range;
	return s;
}

static edit_step step_file(const char * dir)
{
	edit_step s = {};
	s.kind = STEP_FILE;
	s.dir = dir;
	return s;
}

//Что спросить на позиции pos, когда слова до нее уже выбраны
static edit_step edit_step_at(char words[][EDIT_WORD_SIZE], int pos)
{
	const edit_step done = {};
	const char * cmd = words[0];
	const char * arg = words[1];

	if (pos == 0)
		return step_kind(STEP_BLOCK, NULL);

	if (strcmp(cmd, "CH1") == 0) {
		if (pos == 1) return step_words(edit_ch_parts, COUNT_OF(edit_ch_parts));
		if (pos == 2) return step_words(edit_on_off, COUNT_OF(edit_on_off));
		return done;
	}

	if ((strcmp(cmd, "CR1") == 0) || (strcmp(cmd, "AM1") == 0)) {
		const bool carrier = (cmd[0] == 'C');
		if (pos == 1) return step_words(edit_cr_am, COUNT_OF(edit_cr_am));
		if (pos == 2) {
			if (strcmp(arg, "MOD") == 0) return step_file(carrier ? "/Carrier" : "/Mod");
			return step_kind(STEP_VALUE, carrier ? &range_cr_fr : &range_am_fr);
		}
		return done;
	}

	if (strcmp(cmd, "FM1") == 0) {
		if (pos == 1) return step_words(edit_fm, COUNT_OF(edit_fm));
		if (pos == 2) {
			if (strcmp(arg, "MOD") == 0)  return step_file("/Mod");
			if (strcmp(arg, "BASE") == 0) return step_kind(STEP_VALUE, &range_fm_base);
			if (strcmp(arg, "DEV") == 0)  return step_kind(STEP_VALUE, &range_fm_dev);
			return step_kind(STEP_VALUE, &range_fm_fr);
		}
		return done;
	}

	if (strcmp(cmd, "GOTO") == 0) {
		const edit_range range = { 1, edit_lines - 1, 1, 0, "cтр" };
		return (pos == 1) ? step_kind(STEP_GOTO, &range) : done;
	}

	if (strcmp(cmd, "DELAY") == 0)
		return (pos == 1) ? step_kind(STEP_VALUE, &range_delay) : done;

	if (strcmp(cmd, "IF") == 0) {
		if (pos == 1) return step_kind(STEP_REG, NULL);
		if (pos == 2) return step_words(edit_compare, COUNT_OF(edit_compare));
		if (pos == 3) return step_kind(STEP_VALUE, &range_float);
		return done;
	}

	if ((strcmp(cmd, "PLUS") == 0) || (strcmp(cmd, "MINUS") == 0) || (strcmp(cmd, "LOAD") == 0)) {
		if (pos == 1) return step_kind(STEP_REG, NULL);
		if (pos == 2) return step_kind(STEP_VALUE, &range_float);
		return done;
	}

	if (strcmp(cmd, "MUL") == 0) {
		if (pos == 1) return step_kind(STEP_REG, NULL);
		if (pos == 2) return step_kind(STEP_VALUE, &range_mul);
		return done;
	}

	return done; //ELSE ENDIF YIELD END WAIT без операндов
}

static void edit_ask_number(const char * title, const edit_range * r, const char * hint, bool preview, char * out)
{
	const int32_t start = (hint[0] && (hint[0] != 'F')) ? edit_parse(hint, r->frac) : r->def;
	const int32_t value = edit_pick_number(title, r, start, preview);
	edit_format(out, EDIT_WORD_SIZE, value, r->frac);
}

static int edit_word_index(const char * const * words, int count, const char * hint)
{
	for (int i = 0; i < count; i++)
		if (strcmp(words[i], hint) == 0)
			return i;
	return 0;
}

//Спросить одно слово. Подсказка это слово старой строки на этой позиции. false значит шаг назад
static bool edit_ask(const edit_step * s, const char * title, const char * hint, char * out)
{
	int index;

	switch (s->kind) {
	case STEP_BLOCK: {
		//Последняя клетка открывает вторую сетку, Назад из нее возвращает в первую
		const int last = COUNT_OF(edit_blocks) - 1;
		const int more = edit_word_index(edit_more, COUNT_OF(edit_more), hint);

		index = (strcmp(edit_more[more], hint) == 0) ? last : edit_word_index(edit_blocks, COUNT_OF(edit_blocks), hint);

		while (1) {
			index = edit_pick_grid(title, edit_blocks, COUNT_OF(edit_blocks), index);
			if (index < 0) return false;

			const char * word = edit_blocks[index];
			if (index == last) {
				const int sub = edit_pick_grid(title, edit_more, COUNT_OF(edit_more), more);
				if (sub < 0) continue;
				word = edit_more[sub];
			}

			snprintf(out, EDIT_WORD_SIZE, "%s", word);
			return true;
		}
	}

	case STEP_WORDS:
		index = edit_pick_grid(title, s->words, s->count, edit_word_index(s->words, s->count, hint));
		if (index < 0) return false;
		snprintf(out, EDIT_WORD_SIZE, "%s", s->words[index]);
		return true;

	case STEP_REG:
		index = edit_pick_grid(title, edit_value + 1, 10, edit_word_index(edit_value + 1, 10, hint));
		if (index < 0) return false;
		snprintf(out, EDIT_WORD_SIZE, "%s", edit_value[index + 1]);
		return true;

	case STEP_NUMBER:
		edit_ask_number(title, &s->range, hint, false, out);
		return true;

	case STEP_GOTO:
		edit_ask_number(title, &s->range, hint, true, out);
		return true;

	case STEP_VALUE:
		index = edit_pick_grid(title, edit_value, COUNT_OF(edit_value), edit_word_index(edit_value, COUNT_OF(edit_value), hint));
		if (index < 0) return false;
		if (index == 0)
			edit_ask_number(title, &s->range, hint, false, out);
		else
			snprintf(out, EDIT_WORD_SIZE, "%s", edit_value[index]);
		return true;

	case STEP_FILE:
		return edit_pick_file(title, s->dir, ".dat", hint, out);

	default:
		return false;
	}
}

static void edit_join(char words[][EDIT_WORD_SIZE], int count, char * out, int size)
{
	int len = 0;
	out[0] = 0;
	for (int i = 0; (i < count) && (len < size); i++)
		len += snprintf(out + len, size - len, "%s%s", i ? " " : "", words[i]);
}

//Собрать команду, начиная со старой строки. false если вышли с первого шага
static bool edit_build(const char * old, char * out)
{
	char words[EDIT_WORDS][EDIT_WORD_SIZE] = {};
	char old_words[EDIT_WORDS][EDIT_WORD_SIZE] = {};

	char copy[CMD_LEN];
	snprintf(copy, sizeof(copy), "%s", old);
	int old_count = 0;
	for (char * w = strtok(copy, " "); w && (old_count < EDIT_WORDS); w = strtok(NULL, " "))
		snprintf(old_words[old_count++], EDIT_WORD_SIZE, "%s", w);

	int pos = 0;

	while (1) {
		const edit_step step = edit_step_at(words, pos);

		if (step.kind == STEP_DONE) {
			edit_join(words, pos, out, EDIT_TEXT_MAX + 1);
			return true;
		}

		//Пока выбор совпадает со старой строкой, курсор ставим на ее слова
		const char * hint = "";
		bool same = (pos < old_count);
		for (int i = 0; same && (i < pos); i++)
			same = (strcmp(words[i], old_words[i]) == 0);
		if (same)
			hint = old_words[pos];

		char title[CMD_LEN + 4];
		edit_join(words, pos, title, CMD_LEN);
		strcat(title, pos ? " ?" : "?");

		if (edit_ask(&step, title, hint, words[pos]))
			pos++;
		else if (pos == 0)
			return false;
		else
			pos--;
	}
}

//────────────────────────── Строки черновика ──────────────────────────────

//Записать строку без перевода и без половины символа UTF-8
static void edit_set_line(int i, const char * s)
{
	snprintf(edit_list[i], EDIT_TEXT_MAX + 1, "%s", s);
	edit_list[i][strcspn(edit_list[i], "\r\n")] = 0;

	int len = strlen(edit_list[i]);
	if ((s[len] & 0xC0) == 0x80) {
		while ((len > 0) && ((edit_list[i][--len] & 0xC0) == 0x80)) {}
		edit_list[i][len] = 0;
	}
}

//Номера в GOTO идут за своими строками.
//'I' вставка перед at, 'D' удаление at, 'S' обмен at и at+1
static void edit_fix_goto(char op, int at)
{
	for (int i = 1; i < edit_lines; i++) {
		if (strncmp(edit_list[i], "GOTO", 4) != 0)
			continue;

		int n = atoi(edit_list[i] + 4);

		if (op == 'I') { if (n >= at) n++; }
		if (op == 'D') { if (n > at)  n--; }
		if (op == 'S') {
			if (n == at) n = at + 1;
			else if (n == at + 1) n = at;
		}

		snprintf(edit_list[i], CMD_LEN, "GOTO %d", n);
	}
}

static void edit_insert(int at)
{
	for (int i = edit_lines; i > at; i--)
		memcpy(edit_list[i], edit_list[i - 1], CMD_LEN);
	edit_list[at][0] = 0;
	edit_lines++;
	edit_fix_goto('I', at);
}

static void edit_delete(int at)
{
	for (int i = at; i < edit_lines - 1; i++)
		memcpy(edit_list[i], edit_list[i + 1], CMD_LEN);
	edit_lines--;
	edit_fix_goto('D', at);
}

static bool edit_is_full(void)
{
	if (edit_lines < PC_MAX - 1) //Место для END при сохранении
		return false;
	edit_message("Скрипт полон");
	return true;
}

//Новая строка перед at. Номер цели GOTO выбирается уже с учетом вставки
static void edit_add_line(int at)
{
	if (edit_is_full())
		return;

	edit_insert(at);

	if (edit_build("", edit_list[at])) {
		edit_modified = true;
		edit_cursor = at + 1;
	}
	else
		edit_delete(at);
}

static void edit_list_draw(bool moving)
{
	char str[32];

	if (edit_cursor < edit_first) edit_first = edit_cursor;
	if (edit_cursor >= edit_first + EDIT_ROWS) edit_first = edit_cursor - EDIT_ROWS + 1;

	snprintf(str, sizeof(str), "%s%s", edit_list[0][0] ? edit_list[0] : EDIT_NEW_NAME, edit_modified ? "*" : "");
	char right[24];
	snprintf(right, sizeof(right), "%d/%d", edit_lines - 1, PC_MAX - 1);
	edit_frame(moving ? "Перемещение" : str, right);

	for (int row = 0; row < EDIT_ROWS; row++) {
		const int i = edit_first + row;
		if (i > edit_lines) break;

		const int y = EDIT_ROW_H * (row + 1);
		const bool current = (i == edit_cursor);

		if (current)
			tft.RectangleFilled(0, y, 240, EDIT_ROW_H, moving ? RGB565(110, 230, 130) : COLOR_RECTAGLE);

		if (i == edit_lines) {
			Font_Smooth_drawStr(&tft, EDIT_NUMBER_W, y + EDIT_TEXT_DY, "+ в конец", current ? COLOR_TEXT_SELECT : COLOR_TEXT_DISABLE);
			break;
		}

		snprintf(str, sizeof(str), "%d", i);
		Font_Smooth_drawStr(&tft, 2, y + EDIT_TEXT_DY, str, current ? COLOR_TEXT_SELECT : COLOR_TEXT_DISABLE);
		Font_Smooth_drawStr(&tft, EDIT_NUMBER_W, y + EDIT_TEXT_DY, edit_list[i], current ? COLOR_TEXT_SELECT : view_line_color(edit_list[i]));
	}

	tft.driver.ST7789_UpdateDMA16bitV3();
}

//Строка едет за энкодером, клик или удержание ставят на место
static void edit_move(void)
{
	bool redraw = true;

	while (1) {
		int delta;
		const int key = edit_poll(&delta);

		if (delta) {
			const int to = edit_cursor + delta;
			if ((to >= 1) && (to < edit_lines)) {
				char temp[CMD_LEN];
				memcpy(temp, edit_list[edit_cursor], CMD_LEN);
				memcpy(edit_list[edit_cursor], edit_list[to], CMD_LEN);
				memcpy(edit_list[to], temp, CMD_LEN);
				edit_fix_goto('S', (to < edit_cursor) ? to : edit_cursor);
				edit_cursor = to;
				edit_modified = true;
				redraw = true;
			}
		}

		if (key != EDIT_KEY_NONE)
			return;

		if (redraw) {
			redraw = false;
			edit_list_draw(true);
		}
	}
}

static void edit_line_menu(void)
{
	static const char * const rows[] = { "Вcтавить выше", "Изменить", "Копия", "Перемеcтить", "Удалить", "Назад" };

	const int at = edit_cursor;

	char title[CMD_LEN + 8];
	snprintf(title, sizeof(title), "%d: %s", at, edit_list[at]);

	const int action = edit_pick_list(title, rows, COUNT_OF(rows), edit_menu_pos);
	if ((action < 0) || (action == 5))
		return;

	edit_menu_pos = action;

	switch (action) {
	case 0:
		edit_add_line(at);
		break;

	case 1: {
		char text[CMD_LEN];
		if (edit_build(edit_list[at], text)) {
			memcpy(edit_list[at], text, CMD_LEN);
			edit_modified = true;
		}
		break;
	}

	case 2:
		if (edit_is_full())
			break;
		edit_insert(at + 1);
		memcpy(edit_list[at + 1], edit_list[at], CMD_LEN);
		edit_cursor = at + 1;
		edit_modified = true;
		break;

	case 3:
		edit_move();
		break;

	case 4:
		edit_delete(at);
		edit_modified = true;
		break;
	}
}

//────────────────────────── Файл скрипта ──────────────────────────────────

static void edit_new(void)
{
	edit_list[0][0] = 0;
	snprintf(edit_list[1], CMD_LEN, "END");
	edit_lines = 2;
	edit_cursor = 1;
	edit_first = 1;
	edit_modified = false;
}

//Черновик из загруженного скрипта
static void edit_from_script(void)
{
	if (script.lines < 2) {
		edit_new();
		return;
	}

	for (int i = 1; i < script.lines; i++)
		edit_set_line(i, script.list[i].buf);

	//Запущенный без сохранения черновик файла не имеет
	const bool unnamed = (strcmp(script.Name.buf, EDIT_NEW_NAME) == 0);
	snprintf(edit_list[0], CMD_LEN, "%s", unnamed ? "" : script.Name.buf);

	edit_lines = script.lines;
	edit_cursor = 1;
	edit_first = 1;
	edit_modified = unnamed;
}

static bool edit_load_file(const char * name)
{
	char path[48];
	snprintf(path, sizeof(path), "/script/%s.txt", name);

	if (f_open(&SDFile, path, FA_READ) != FR_OK)
		return false;

	snprintf(edit_list[0], CMD_LEN, "%s", name);

	char str[32];
	edit_lines = 1;
	while ((edit_lines < PC_MAX) && script.readLine(str, sizeof(str)))
		edit_set_line(edit_lines++, str);

	f_close(&SDFile);

	edit_cursor = 1;
	edit_first = 1;
	edit_modified = false;
	return true;
}

//Как GENERATOR::Save_File: на время записи генератор на паузе. f_puts сам пишет CRLF
static bool edit_write_file(const char * name)
{
	char path[48];
	snprintf(path, sizeof(path), "/script/%s.txt", name);

	Gen.pause();

	f_mkdir("/script"); //Если уже есть, вернет FR_EXIST

	bool ok = false;
	if (f_open(&SDFile, path, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
		ok = true;
		for (int i = 1; ok && (i < edit_lines); i++)
			ok = (f_puts(edit_list[i], &SDFile) >= 0) && (f_puts("\n", &SDFile) >= 0);
		ok = (f_close(&SDFile) == FR_OK) && ok;
	}

	Gen.start();

	return ok;
}

static bool edit_file_exists(const char * name)
{
	char path[48];
	FILINFO fno;
	snprintf(path, sizeof(path), "/script/%s.txt", name);
	return f_stat(path, &fno) == FR_OK;
}

//Первый свободный номер, подсказка для имени нового файла
static void edit_free_name(char * out, int size)
{
	for (int n = 1; n < 100; n++) {
		snprintf(out, size, "S%02d", n);
		if (!edit_file_exists(out))
			return;
	}
	out[0] = 0;
}

//Без END движок побежит дальше конца списка
static void edit_ensure_end(void)
{
	if ((edit_lines > 1) && (strcmp(edit_list[edit_lines - 1], "END") == 0))
		return;
	if (edit_lines >= PC_MAX)
		return;
	snprintf(edit_list[edit_lines++], CMD_LEN, "END");
	edit_modified = true;
}

//Первая ошибка структуры: номер строки или 0. Вложенные IF движок не умеет
static int edit_check(const char ** message)
{
	int open_if = 0;
	bool has_else = false;

	for (int i = 1; i < edit_lines; i++) {
		const char * s = edit_list[i];

		if (strncmp(s, "ENDIF", 5) == 0) {
			if (open_if == 0) { *message = "ENDIF без IF"; return i; }
			open_if = 0;
		}
		else if (strncmp(s, "ELSE", 4) == 0) {
			if ((open_if == 0) || has_else) { *message = "ELSE без IF"; return i; }
			has_else = true;
		}
		else if (strncmp(s, "IF", 2) == 0) {
			if (open_if) { *message = "IF внутри IF"; return i; }
			open_if = i;
			has_else = false;
		}
		else if (strncmp(s, "GOTO", 4) == 0) {
			const int n = atoi(s + 4);
			if ((n < 1) || (n >= edit_lines)) { *message = "GOTO мимо"; return i; }
		}
	}

	if (open_if) {
		*message = "IF без ENDIF";
		return open_if;
	}
	return 0;
}

//Предупреждение не запрещает сохранить. true если идем дальше
static bool edit_check_dialog(void)
{
	static const char * const rows[] = { "Иcправить", "Продолжить" };

	const char * message;
	const int line = edit_check(&message);
	if (line == 0)
		return true;

	edit_cursor = line;

	char title[48];
	snprintf(title, sizeof(title), "Строка %d: %s", line, message);
	return edit_pick_list(title, rows, COUNT_OF(rows), 0) == 1;
}

static bool edit_save(const char * name)
{
	edit_ensure_end();

	if (!edit_check_dialog())
		return false;

	char text[40];

	if (!edit_write_file(name)) {
		edit_message("Ошибка SD");
		return false;
	}

	snprintf(edit_list[0], CMD_LEN, "%s", name);
	edit_modified = false;

	snprintf(text, sizeof(text), "Сохранено %s", name);
	edit_message(text);
	return true;
}

static const char edit_kb_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static void edit_kb_draw(const char * name, int index)
{
	static const char * const wide[] = { "Стереть", "OK", "Назад" };

	char title[EDIT_WORD_SIZE + 16];
	snprintf(title, sizeof(title), "Имя: %s_", name);
	char right[16];
	snprintf(right, sizeof(right), "%d/%d", (int) strlen(name), EDIT_NAME_MAX);
	edit_frame(title, right);

	for (int i = 0; i < EDIT_KB_TOTAL; i++) {
		const bool current = (i == index);
		const uint16_t color = current ? COLOR_TEXT_SELECT : COLOR_TEXT_DEFAULT;
		const uint16_t back = current ? COLOR_RECTAGLE : RGB565(40, 40, 70);

		if (i < EDIT_KB_CHARS) {
			const int x = (i % EDIT_KB_COLS) * EDIT_KB_W;
			const int y = EDIT_ROW_H + 1 + (i / EDIT_KB_COLS) * EDIT_KB_H;
			const char str[2] = { edit_kb_chars[i], 0 };
			tft.RectangleFilled(x + 1, y + 1, EDIT_KB_W - 2, EDIT_KB_H - 2, back);
			Font_Smooth_drawStr(&tft, x + 14, y + 5, str, color);
		}
		else {
			const int x = (i - EDIT_KB_CHARS) * (240 / COUNT_OF(wide));
			const int y = EDIT_ROW_H + 1 + (EDIT_KB_CHARS / EDIT_KB_COLS) * EDIT_KB_H;
			tft.RectangleFilled(x + 1, y + 1, 240 / COUNT_OF(wide) - 2, EDIT_KB_H - 2, back);
			Font_Smooth_drawStr(&tft, x + (240 / COUNT_OF(wide) - edit_text_chars(wide[i - EDIT_KB_CHARS]) * 10) / 2, y + 5, wide[i - EDIT_KB_CHARS], color);
		}
	}

	tft.driver.ST7789_UpdateDMA16bitV3();
}

//Экранная клавиатура. Курсор идет по кругу и стоит на OK: подсказка сохраняется одним кликом,
//шаг влево это Del, шаг вправо через Back к букве A. Удержание как OK. false если отказались
static bool edit_pick_name(char * name)
{
	int index = EDIT_KB_OK;
	bool redraw = true;

	while (1) {
		int delta;
		const int key = edit_poll(&delta);

		if (delta) {
			index = (index + delta + EDIT_KB_TOTAL) % EDIT_KB_TOTAL;
			redraw = true;
		}

		if (key == EDIT_KEY_HOLD)
			return name[0] != 0;

		if (key == EDIT_KEY_CLICK) {
			const int len = strlen(name);

			if (index == EDIT_KB_BACK)
				return false;

			if ((index == EDIT_KB_OK) && len)
				return true;

			if ((index == EDIT_KB_DEL) && len)
				name[len - 1] = 0;

			if ((index < EDIT_KB_CHARS) && (len < EDIT_NAME_MAX)) {
				name[len] = edit_kb_chars[index];
				name[len + 1] = 0;
			}

			redraw = true;
		}

		if (redraw) {
			redraw = false;
			edit_kb_draw(name, index);
		}
	}
}

//Занятое другим скриптом имя только с подтверждением
static bool edit_overwrite_ok(const char * name)
{
	static const char * const rows[] = { "Перезапиcать", "Назад" };

	if ((strcasecmp(name, edit_list[0]) == 0) || !edit_file_exists(name))
		return true;

	char title[EDIT_WORD_SIZE + 24];
	snprintf(title, sizeof(title), "%s уже еcть", name);
	return edit_pick_list(title, rows, COUNT_OF(rows), 1) == 0;
}

static bool edit_save_as(void)
{
	char name[EDIT_WORD_SIZE];
	edit_free_name(name, sizeof(name));

	while (edit_pick_name(name))
		if (edit_overwrite_ok(name))
			return edit_save(name);

	return false;
}

static bool edit_save_current(void)
{
	char name[CMD_LEN];
	snprintf(name, sizeof(name), "%s", edit_list[0]);
	return name[0] ? edit_save(name) : edit_save_as();
}

//Можно ли бросить черновик
static bool edit_can_leave(void)
{
	static const char * const rows[] = { "Сохранить", "Не cохранять", "Отмена" };

	if (!edit_modified)
		return true;

	switch (edit_pick_list("Не cохранено", rows, COUNT_OF(rows), 0)) {
	case 0:  return edit_save_current();
	case 1:  return true;
	default: return false;
	}
}

static void edit_open(void)
{
	if (!edit_can_leave())
		return;

	char name[EDIT_WORD_SIZE];
	if (edit_pick_file("Открыть", "/script", ".txt", edit_list[0], name) && !edit_load_file(name))
		edit_message("Ошибка SD");
}

//Запуск из черновика без сохранения. В окне просмотра клик вернет сюда
static void edit_run(void)
{
	edit_ensure_end();

	if (!edit_check_dialog())
		return;

	script.pause();

	for (int i = 1; i < PC_MAX; i++) {
		if (i < edit_lines)
			snprintf(script.list[i].buf, CMD_LEN, "%s\n", edit_list[i]);
		else
			script.list[i].buf[0] = 0; //Остатки прошлого скрипта
	}

	script.Name = edit_list[0][0] ? edit_list[0] : EDIT_NEW_NAME;
	script.list[0] = script.Name.buf;
	script.lines = edit_lines;
	script.pc_exec = 0;
	script.start();

	PAGE_Script_View();

	edit_font();
}

//Меню файла. true значит выйти из редактора
static bool edit_file_menu(void)
{
	static const char * const rows[] = { "Запуcк", "Стоп", "Сохранить", "Сохранить как", "Новый", "Открыть", "Выход" };

	char title[CMD_LEN + 8];
	snprintf(title, sizeof(title), "%s%s", edit_list[0][0] ? edit_list[0] : EDIT_NEW_NAME, edit_modified ? "*" : "");

	switch (edit_pick_list(title, rows, COUNT_OF(rows), 0)) {
	case 0:
		edit_run();
		break;

	case 1:
		script.stop();
		break;

	case 2:
		edit_save_current();
		break;

	case 3:
		edit_save_as();
		break;

	case 4:
		if (edit_can_leave())
			edit_new();
		break;

	case 5:
		edit_open();
		break;

	case 6:
		return edit_can_leave();
	}

	return false;
}

//Редактор скрипта энкодером. Клик по строке: ее меню, удержание: меню файла
void PAGE_Script_Edit(void)
{
	if (edit_active) //Удержание в окне просмотра, открытом из редактора
		return;

	edit_active = true;
	edit_menu_pos = 0;

	edit_font();
	edit_from_script();

	bool redraw = true;

	while (1) {
		int delta;
		const int key = edit_poll(&delta);

		if (delta) {
			edit_cursor += delta;
			redraw = true;
		}

		if (key == EDIT_KEY_HOLD) {
			if (edit_file_menu())
				break;
			redraw = true;
		}

		if (key == EDIT_KEY_CLICK) {
			if (edit_cursor >= edit_lines)
				edit_add_line(edit_lines);
			else
				edit_line_menu();
			redraw = true;
		}

		if (redraw) {
			redraw = false;
			if (edit_cursor < 1) edit_cursor = 1;
			if (edit_cursor > edit_lines) edit_cursor = edit_lines;
			edit_list_draw(false);
		}
	}

	button.isClick();
	button.isHolded();
	button.isDouble();

	edit_active = false;

	Font_Smooth_Load(menu_setting.font);
}

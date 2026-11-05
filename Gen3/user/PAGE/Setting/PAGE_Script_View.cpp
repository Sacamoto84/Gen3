#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "menu_typedef.h"
#include "scripting.h"

#include "global_typedef.h"
extern Encoder_typedef Encoder;

#include "Button.h"
extern button_typedef button;

extern Scripting script;

//┌─────────────────────────────────────────────────────────┐
	#define VIEW_ROW_H      21                            //│
	#define VIEW_ROWS       10                            //│ Плюс строка заголовка
	#define VIEW_TEXT_DY    3                             //│ Как на рабочем экране
	#define VIEW_NUMBER_W   34                            //│ Ширина колонки номера
	#define VIEW_INDENT_STEP 8                            //│ Шаг отступа на уровень вложенности (~1 символ)
	#define VIEW_ABOVE      3                             //│ Строк над текущей
	#define VIEW_FRAME_MS   50                            //│ Не чаще 20 кадров в секунду
	#define VIEW_MANUAL_MS  3000                          //│ После энкодера снова за текущей строкой
	#define VIEW_REGS_MS    200                           //│ Новое значение регистра: кадр стоит скрипту 40 мс
//└─────────────────────────────────────────────────────────┘

//Цвет строки по команде
uint16_t view_line_color(const char * s)
{
	if ((strncmp(s, "IF", 2) == 0) || (strncmp(s, "ELSE", 4) == 0) || (strncmp(s, "ENDIF", 5) == 0))
		return RGB565(90, 200, 255);
	if (strncmp(s, "END", 3) == 0)
		return RGB565(255, 220, 60);
	if (strncmp(s, "DIV", 3) == 0)
		return COLOR_TEXT_DEFAULT;
	if ((strncmp(s, "LOOP", 4) == 0) || (strncmp(s, "RET", 3) == 0))
		return RGB565(110, 230, 130);

	switch (s[0])
	{
		case 'C': case 'A': case 'F': case 'V': return RGB565(255, 170, 90); //CH CR AM FM VOL
		case 'G':           return RGB565(110, 230, 130); //GOTO GOSUB
		case 'D': case 'Y': case 'W': return RGB565(200, 160, 255); //DELAY YIELD WAIT
		case 'T':           return COLOR_TEXT_DISABLE;    //Текст
		default:            return COLOR_TEXT_DEFAULT;    //LOAD PLUS MINUS MUL
	}
}

//Какие регистры упомянуты в скрипте: бит на каждый F0..F9
static uint16_t view_used_regs(void)
{
	uint16_t used = 0;

	for (int i = 1; i < script.lines; i++) {
		const char * s = script.list[i].buf;
		if (s[0] == 'T')
			continue;
		for (const char * p = s; (p = strchr(p, ' ')) != NULL; ) {
			p++;
			if ((p[0] == 'F') && (p[1] >= '0') && (p[1] <= '9') && ((p[2] == ' ') || (p[2] == '\r') || (p[2] == '\n') || (p[2] == 0)))
				used |= 1 << (p[1] - '0');
		}
	}
	return used;
}

//Строка значений внизу. В DELAY скрипт стоит почти все время, поэтому показываем регистры, а не текущую строку
static void view_draw_regs(uint16_t used, int y)
{
	char str[96];
	int len = 0;

	tft.RectangleFilled(0, y, 240, 240 - y, RGB565(40, 40, 70));

	for (int i = 0; (i < 10) && (len < (int) sizeof(str)); i++) {
		if (used & (1 << i)) {
			len += snprintf(str + len, sizeof(str) - len, "F%d=%.1f ", i, script.F[i]);
			if ((len >= 3) && (len < (int) sizeof(str)) && (strncmp(str + len - 3, ".0 ", 3) == 0)) { //Без лишнего .0
				len -= 3;
				str[len++] = ' ';
				str[len] = 0;
			}
		}
	}

	Font_Smooth_drawStr(&tft, 2, y + VIEW_TEXT_DY, str, COLOR_TEXT_DEFAULT);
}

//Первая строка окна, чтобы текущая была видна
static int view_first_for(int pc, int last_first)
{
	int first = pc - VIEW_ABOVE;
	if (first > last_first) first = last_first;
	if (first < 1) first = 1;
	return first;
}

//Вычисление отступов для строк скрипта (0..3 уровня, шаг VIEW_INDENT_STEP)
static void view_compute_indents(uint8_t * indents)
{
	bool is_sub[PC_MAX] = {false};

	for (int i = 1; i < script.lines; i++) {
		const char * s = script.list[i].buf;
		while (*s == ' ' || *s == '\t') s++;
		if ((strncmp(s, "GOSUB", 5) == 0) && (s[5] == ' ' || s[5] == '\t')) {
			int target = atoi(s + 5);
			if ((target >= 1) && (target < script.lines))
				is_sub[target] = true;
		}
	}

	int sub_depth = 0;
	int if_depth = 0;

	for (int i = 1; i < script.lines; i++) {
		const char * s = script.list[i].buf;
		while (*s == ' ' || *s == '\t') s++;

		if (is_sub[i])
			sub_depth = 1;

		int indent = sub_depth + if_depth;

		if ((strncmp(s, "IF", 2) == 0) && (s[2] == ' ' || s[2] == '\t')) {
			indent = sub_depth + if_depth;
			if (if_depth < 3) if_depth++;
		}
		else if ((strncmp(s, "ELSE", 4) == 0) && (s[4] == ' ' || s[4] == '\t' || s[4] == '\r' || s[4] == '\n' || s[4] == 0)) {
			indent = sub_depth + (if_depth > 0 ? if_depth - 1 : 0);
		}
		else if ((strncmp(s, "ENDIF", 5) == 0) && (s[5] == ' ' || s[5] == '\t' || s[5] == '\r' || s[5] == '\n' || s[5] == 0)) {
			if (if_depth > 0) if_depth--;
			indent = sub_depth + if_depth;
		}
		else if ((strncmp(s, "RET", 3) == 0) && (s[3] == ' ' || s[3] == '\t' || s[3] == '\r' || s[3] == '\n' || s[3] == 0)) {
			indent = sub_depth + if_depth;
			sub_depth = 0;
		}
		else if ((strncmp(s, "END", 3) == 0) && (s[3] == ' ' || s[3] == '\t' || s[3] == '\r' || s[3] == '\n' || s[3] == 0)) {
			indent = 0;
			sub_depth = 0;
			if_depth = 0;
		}

		if (indent > 3) indent = 3;
		if (indent < 0) indent = 0;
		indents[i] = (uint8_t) indent;
	}
}

static void view_draw(int first, int rows, uint16_t used)
{
	char str[32];
	uint8_t indents[PC_MAX] = {0};
	view_compute_indents(indents);

	tft.Fill16(COLOR_BACKGROUND);

	//Заголовок: имя скрипта и состояние
	tft.RectangleFilled(0, 0, 240, VIEW_ROW_H, RGB565(40, 40, 70));

	if (script.lines < 2) {
		Font_Smooth_drawStr(&tft, 4, VIEW_TEXT_DY, "No script", COLOR_TEXT_DEFAULT);
		tft.driver.ST7789_UpdateDMA16bitV3();
		return;
	}

	Font_Smooth_drawStr(&tft, 4, VIEW_TEXT_DY, script.Name.buf, COLOR_TEXT_DEFAULT);
	Font_Smooth_drawStr(&tft, 190, VIEW_TEXT_DY, script.end ? "STOP" : "RUN",
			script.end ? COLOR_TEXT_DISABLE : RGB565(110, 230, 130));

	//Строки скрипта, нумерация как в GOTO
	for (int row = 0; row < rows; row++) {
		const int i = first + row;
		if (i >= script.lines) break;

		const int y = VIEW_ROW_H * (row + 1);
		const bool current = (i == (int) script.pc_exec);

		if (current)
			tft.RectangleFilled(0, y, 240, VIEW_ROW_H, script.end ? COLOR_TEXT_DISABLE : COLOR_RECTAGLE);

		snprintf(str, sizeof(str), "%d", i);
		Font_Smooth_drawStr(&tft, 2, y + VIEW_TEXT_DY, str, current ? COLOR_TEXT_SELECT : COLOR_TEXT_DISABLE);

		snprintf(str, sizeof(str), "%s", script.list[i].buf);
		str[strcspn(str, "\r\n")] = 0;
		const int x = VIEW_NUMBER_W + indents[i] * VIEW_INDENT_STEP;
		Font_Smooth_drawStr(&tft, x, y + VIEW_TEXT_DY, str, current ? COLOR_TEXT_SELECT : view_line_color(str));
	}

	if (used)
		view_draw_regs(used, VIEW_ROW_H * (rows + 1));

	tft.driver.ST7789_UpdateDMA16bitV3();
}

//Просмотр загруженного скрипта и текущей строки. Энкодер листает, клик выход или продолжение WAIT, удержание редактор
void PAGE_Script_View(void)
{
	Font_Smooth_Load(menu_generator.font);
	tft.uTFT.GetColor = 1;

	int first = -1;           //Первая строка окна
	int drawn_first = -1;     //Что нарисовано сейчас
	int drawn_pc = -1;
	int drawn_end = -1;
	uint32_t manual_time = 0; //Когда крутили энкодер
	bool manual = false;
	uint32_t frame_time = 0;

	const uint16_t used = view_used_regs();
	const int rows = used ? VIEW_ROWS - 1 : VIEW_ROWS; //Место для строки регистров
	float drawn_F[10] = {};

	while (1) {

		script.run();

		button.tick();

		const int last_first = (script.lines > rows + 1) ? script.lines - rows : 1;

		if (Encoder.Left || Encoder.Right) {
			if (first < 1) first = view_first_for(script.pc_exec, last_first);
			if (Encoder.Left)  first--;
			if (Encoder.Right) first++;
			Encoder.Left = 0;
			Encoder.Right = 0;
			if (first < 1) first = 1;
			if (first > last_first) first = last_first;
			manual = true;
			manual_time = HAL_GetTick();
		}

		const uint32_t now = HAL_GetTick();

		if (manual && (now - manual_time >= VIEW_MANUAL_MS))
			manual = false;

		if (!manual)
			first = view_first_for(script.pc_exec, last_first);

		//Цикл возвращается в свой DELAY за один проход: номер строки тот же, а регистр уже другой
		const bool regs_changed = used && (memcmp(drawn_F, script.F, sizeof(drawn_F)) != 0) && (now - frame_time >= VIEW_REGS_MS);

		const bool changed = (first != drawn_first) || ((int) script.pc_exec != drawn_pc) || ((int) script.end != drawn_end) || regs_changed;

		if (changed && ((drawn_first < 0) || (now - frame_time >= VIEW_FRAME_MS))) {
			frame_time = now;
			drawn_first = first;
			drawn_pc = script.pc_exec;
			drawn_end = script.end;
			memcpy(drawn_F, script.F, sizeof(drawn_F));
			view_draw(first, rows, used);
		}

		if (button.isHolded()) {
			PAGE_Script_Edit(); //Если открыты из редактора, сразу вернется и мы выйдем к нему
			break;
		}

		if (button.isClick()) {
			if (!script.waiting)
				break;
			script.waiting = false;
		}
	}

	button.isClick();
	button.isHolded();
	button.isDouble();

	Font_Smooth_Load(menu_setting.font);
}

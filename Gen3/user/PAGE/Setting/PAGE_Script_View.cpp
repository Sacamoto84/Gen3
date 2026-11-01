#include "Page.h"
#include "global_define.h"
#include "stdio.h"
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
	#define VIEW_ABOVE      3                             //│ Строк над текущей
	#define VIEW_FRAME_MS   50                            //│ Не чаще 20 кадров в секунду
	#define VIEW_MANUAL_MS  3000                          //│ После энкодера снова за текущей строкой
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

	switch (s[0])
	{
		case 'C': case 'A': case 'F': return RGB565(255, 170, 90); //CH CR AM FM
		case 'G':           return RGB565(110, 230, 130); //GOTO
		case 'D': case 'Y': case 'W': return RGB565(200, 160, 255); //DELAY YIELD WAIT
		case 'T':           return COLOR_TEXT_DISABLE;    //Текст
		default:            return COLOR_TEXT_DEFAULT;    //LOAD PLUS MINUS MUL
	}
}

//Первая строка окна, чтобы текущая была видна
static int view_first_for(int pc, int last_first)
{
	int first = pc - VIEW_ABOVE;
	if (first > last_first) first = last_first;
	if (first < 1) first = 1;
	return first;
}

static void view_draw(int first)
{
	char str[32];

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
	for (int row = 0; row < VIEW_ROWS; row++) {
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
		Font_Smooth_drawStr(&tft, VIEW_NUMBER_W, y + VIEW_TEXT_DY, str, current ? COLOR_TEXT_SELECT : view_line_color(str));
	}

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

	while (1) {

		script.run();

		button.tick();

		const int last_first = (script.lines > VIEW_ROWS + 1) ? script.lines - VIEW_ROWS : 1;

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

		const bool changed = (first != drawn_first) || ((int) script.pc_exec != drawn_pc) || ((int) script.end != drawn_end);

		if (changed && ((drawn_first < 0) || (now - frame_time >= VIEW_FRAME_MS))) {
			frame_time = now;
			drawn_first = first;
			drawn_pc = script.pc_exec;
			drawn_end = script.end;
			view_draw(first);
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

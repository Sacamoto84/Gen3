#ifndef SCRIPTING_H_
#define SCRIPTING_H_

#include "mString.h"
#include "main.h"
#include "fatfs.h"

#include "tim.h"
#include "HiSpeedDWT.h"
#include "generator.h"

#include "timber.h"

extern uint8_t render;

extern HiSpeedDWT TimerT5;

#define PC_MAX 128
#define CMD_LEN 24
#define GOSUB_MAX 4 //Вложенность подпрограмм


//Команда максимум 24 байт [128]
//Скрипты лежат в папке script

/*
 * ----------------- Логика -----------------
 * IF F1 == 2
 * ...
 * ELSE
 * ...
 * ENDIF
 *
 * DELAY 1000 - Задержка работы
 *
 * IF можно вкладывать в IF.
 *
 * Неизвестная команда, чужой регистр, деление на ноль, IF без ENDIF, RET без GOSUB
 * и вложенность GOSUB больше GOSUB_MAX останавливают скрипт на этой строке.
 * Пустая строка занимает номер и ничего не делает, CH2 CR2 AM2 FM2 пропускаются
 */

/*
 *╭─ Выйти ─╮╭─ Переход ─╮╭─ Задержка ─╮╭─ Завершение ─╮╭─ Текст ────╮╭─ Ждать клик ─╮
 *│ YIELD   ││ GOTO 2    ││ DELAY 4000 ││ END          ││ TEXT Текст ││ WAIT         │
 *╰─────────╯╰───────────╯│ DELAY F1   │╰──────────────╯╰────────────╯╰──────────────╯
 *                        ╰────────────╯
 *╭─ Цикл ─────────┬──────────────────────────────╮╭─ Подпрограмма ─╮
 *│ LOOP F0 2      │ F0 - 1 -> F0, если > 0 GOTO 2 ││ GOSUB 7        │
 *╰────────────────┴──────────────────────────────╯│ RET            │
 *                                                 ╰────────────────╯
 *╭─ Прочитать с панели ─────────────────╮
 *│ READ F1 CR1 FR │ частота несущей     │
 *│ READ F1 AM1 FR │ частота AM          │
 *│ READ F1 FM1 FR │ BASE DEV FR         │
 *╰────────────────┴─────────────────────╯
 *╭─ Арифметика ────┬─────────────────────╮╭─ Загрузка в регистр ────────────╮
 *│ MINUS F1 5000.0 │ F1 - 5000.0 -> F1   ││ LOAD F1 2344.0  │ 2344.0 -> F1  │
 *│ MINUS F1 F2     │ F1 - F2 -> F1       ││ LOAD F1 F2      │ F2 -> F1      │
 *│ PLUS  F1 4555.5 │ F1 + 4555.5 -> F1   │╰─────────────────┴───────────────╯
 *│ PLUS  F1 F2     │ F1 + F2 -> F1       │╭─ Отобразить дамп регистров ──╮
 *│ MUL   F1 1.02   │ F1 * 1.02 -> F1     ││ PRINTF - Дамп регистров F    │
 *│ MUL   F1 F2     │ F1 * F2 -> F1       │╰──────────────────────────────╯
 *│ DIV   F1 1.02   │ F1 / 1.02 -> F1     │
 *│ DIV   F1 F2     │ F1 / F2 -> F1       │
 *╰─────────────────┴─────────────────────╯
 *╭─ Генератор ─────────────────╮╭─╮╭────────────┬──────────────╮
 *│ CH[1 2] [CR AM FM] [ON OFF] ││✓││IF F1 < 450 │ IF F1 < 450  │
 *│                             ││ ││...{true}   │ ...  {true}  │
 *│ CR[1 2] FR 1000.0     F[]   ││✓││ELSE        │ ENDIF        │
 *│ CR[1 2] MOD 02_HWawe        ││✓││...{false}  │ ...  {false} │
 *│                             ││ ││ENDIF       │              │
 *│ AM[1 2] FR 1000.3     F[]   ││✓│├───┬────┬───┼────┬────┬────┤
 *│ AM[1 2] MOD 02_HWawe        ││✓││ < │ <= │ > │ >= │ == │ != │
 *│                             ││ │╰───┴────┴───┴────┴────┴────╯
 *│ FM[1 2] BASE 1234.6   F[]   ││✓│
 *│ FM[1 2] DEV  123.8    F[]   ││✓│
 *│ FM[1 2] MOD  02_HWawe       ││✓│
 *│ FM[1 2] FR   3.5      F[]   ││✓│
 *╰─────────────────────────────╯╰─╯
 * ┌ ┐ └ ┘├ ┤ ┬ ┴ ┼ ─ │╭╮╯╰│ ─ ✓
 *
 * Переход если равно и не равно
 *
 *
 */

extern HiSpeedDWT TimerDWT;

//Структура ответа для выделения пары операндов из команды
typedef struct {
	mString<CMD_LEN> operand0;
	mString<CMD_LEN> operand1;
}pair_operand;

//Структура ответа для выделения пары операндов из команды
typedef struct {
	mString<CMD_LEN> operand0;
	mString<CMD_LEN> operand1;
	mString<CMD_LEN> operand2;
}triple_operand;

class Scripting{
public:

//╭─ Генератор ─────────────────╮
	mString<CMD_LEN> Name; //Имя текущего скрипта

	pair_operand   pair;
	triple_operand triple;

	bool end = true;
	bool yield = false;
	bool waiting = false;        //WAIT: ждем клик в окне просмотра
	uint32_t pc;

	uint32_t endTime;            //Время > которого можно продолжать работу

	uint8_t gosub_stack[GOSUB_MAX]; //Куда вернется RET
	uint8_t gosub_depth;

	float   F[10];               //10 FLOAT регистров

	char str[32];                //Временная строка
	mString<CMD_LEN> list[PC_MAX];    //Список команд
	uint8_t line = 0;            //Текучая строка
	uint8_t lines = 0;           //Сколько записей в list вместе с именем в [0]
	uint32_t pc_exec = 0;        //Строка, которая выполнялась последней (pc уже на следующей, например в DELAY)
	GENERATOR * G;

	bool returnToMenu2 = false;

//╰─────────────────────────────╯


	//Новый запуск не наследует паузу и регистры прошлого скрипта
	void start(void)  { pc = 1; end = false; waiting = false; endTime = 0; gosub_depth = 0; memset(F, 0, sizeof(F)); if (G) G->CH1.Volume_Script = 1.0F; }
	void stop(void)   { pc = 1; end = true; waiting = false; timber.print("Script Stop\n");}

	void pause(void)  { end = true;  }
	void resume(void) { end = false; }

	void printF(void);

	void run(void);

	triple_operand excretionTripleOperand(mString<CMD_LEN> str);

	void   comandError(void);
	float* reg(mString<CMD_LEN> & s);
	float  opValue(mString<CMD_LEN> & s);

	bool  readLine(char * s, int size);

	void  comandPlusMinus(void);
	void  generatorComand(void);

	void load(char *name);
	void CMD_EXE(void);

	void ifComand(void);
	uint32_t ifBlockEnd(bool stop_at_else);
	void readComand(void);

	void command(char * str);

 void Unit5Load(void)
 {
	list[0] = "Unit test";
	list[1] = "T Юнит тест\r\n";
	list[2] = "LOAD F1 1000\r\n";
	list[3] = "IF F1 < 10000\r\n";
	list[4] = "CR1 FR F1\r\n";
	list[5] = "PLUS F1 100\r\n";
	list[6] = "DELAY 1000\r\n";
    list[7] = "GOTO 3\r\n";
    list[8] = "ENDIF\r\n";
    list[9] = "T Выход\r\n";
    list[10] = "END\r\n";
 }

};











#endif

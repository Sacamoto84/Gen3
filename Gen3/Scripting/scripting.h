#ifndef SCRIPTING_H_
#define SCRIPTING_H_

#include "mString.h"
#include "main.h"
#include "fatfs.h"

#include "tim.h"
#include "HiSpeedDWT.h"
#include "generator.h"

#include "logUART.h"
extern classLog rtt;

extern uint8_t render;

extern HiSpeedDWT TimerT5;

#define PC_MAX 128
#define CMD_LEN 24


//Команда максимум 24 байт [128]
//Скрипты лежат в папке script

/*
 * ----------------- Логика -----------------
 * IF R1 = 2
 * ...
 * ELSE
 * ...
 * ENDIF
 *
 * DELAY 1000 - Задержка работы
 */

/*
 *╭─ Выйти ─╮╭─ Переход ─╮╭─ Задержка ─╮╭─ Завершение ─╮╭─ Текст ────╮
 *│ YIELD   ││ GOTO 2    ││ DELAY 4000 ││ END          ││ TEXT Текст │
 *╰─────────╯╰───────────╯╰────────────╯╰──────────────╯╰────────────╯
 *╭─ Арифметика ────┬─────────────────────╮╭─ Загрузка константы в регистр ─╮
 *│ MINUS R1 5000   │ R1 - 5000-> R1      ││ LOAD R1 2344    │ 2344 -> R1   │
 *│ MINUS F1 5000.0 │ F1 - 5.1 -> F1      ││ LOAD F1 2344.0  │ 2344 -> F1   │
 *│                 │                     │╰─────────────────┴──────────────╯
 *│ MINUS R1 R2     │ R1-R2->R1           │╭─ Отобразить дамп регистров ──╮
 *│ MINUS F1 F2     │ F1-F2->F1           ││ PRINTF - Дамп регистров F    │
 *│                 │                     │╰──────────────────────────────╯
 *│ PLUS R1 4555    │ R1 + 4555 -> 4555   │
 *│ PLUS F1 4555.5  │ F1 + 4555.5 -> 4555 │
 *│                 │                     │
 *│ PLUS R1 R2      │ R1+R2->R1           │
 *│ PLUS F1 F2      │ F1+F2->F1           │
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
 *╭─ Копирование регистров ───╮ Не готово
 *│ COPY R1 R2       │ R2->R1 │
 *│ COPY F1 F2       │ F2->F1 │
 *╰──────────────────┴────────┘
 *
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
	uint32_t pc;

	uint32_t endTime;            //Время > которого можно продолжать работу

	float   F[10];               //10 FLOAT регистров

	char str[32];                //Временная строка
	mString<CMD_LEN> list[PC_MAX];    //Список команд
	uint8_t line = 0;            //Текучая строка
	GENERATOR * G;

	bool returnToMenu2 = false;

//╰─────────────────────────────╯


	void start(void)  { pc = 1; end = false; }
	void stop(void)   { pc = 1; end = true; rtt.print("Script Stop\n");}

	void pause(void)  { end = true;  }
	void resume(void) { end = false; }

	void printF(void);

	void run(void);

	triple_operand excretionTripleOperand(mString<CMD_LEN> str);

	void  comandPlusMinus(void);
	void  generatorComand(void);

	void load(char *name);
	void CMD_EXE(void);

	void ifComand(void);

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

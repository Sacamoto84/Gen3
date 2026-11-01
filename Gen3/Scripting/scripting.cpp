#include "scripting.h"
#include "HiSpeedDWT.h"

#include "timber.h"

#pragma GCC optimize ("Os") //Скорость тут не нужна, синтез в generator.cpp, а flash мало: с -Ofast файл на 900 байт больше

Scripting script CCMRAM;

extern HiSpeedDWT TimerDWT;
extern HiSpeedDWT TimerT5;

int i;

//Каждые 10ms
extern "C" void TIM7_IRQHandler(void) {
	//script.run();
	HAL_TIM_IRQHandler(&htim7);
}

void Scripting::run(void) {
	if (end)
		return;
	end = false;
	yield = false;
	if (waiting)
		return;
	if (uwTick < endTime)
		return;
	endTime = 0;

	//Цикл без DELAY и YIELD не должен вешать устройство: остальное на следующем проходе
	for (int steps = 0; (steps < PC_MAX) && (yield == false) && (end == false); steps++) {
		CMD_EXE();
		if (uwTick <= endTime)
			return;
	}
}

void Scripting::printF(void)
{
	char s[127];
	sprintf(s, "\x1B[0mF 0:%.1f 1:%.1f 2:%.1f 3:%.1f 4:%.1f 5:%.1f 6:%.1f 7:%.1f 8:%.1f 9:%.1f\n",
			F[0],F[1],F[2],F[3],F[4],F[5],F[6],F[7],F[8],F[9]);
	timber.print(s);
}

//Первые три слова строки, недостающие пустые. Перевод строки и все, что после третьего слова, отбрасываем
triple_operand Scripting::excretionTripleOperand(mString<CMD_LEN> str){
	triple_operand out;
	const char * strings[3] = { "", "", "" };
	char * p = str.buf;
	int n = 0;

	strings[0] = p;
	for (; *p; p++) {
		if ((*p == '\r') || (*p == '\n')) {
			*p = 0;
			break;
		}
		if (*p == ' ') {
			*p = 0;
			if (strings[n][0] == 0) { //Несколько пробелов подряд
				strings[n] = p + 1;
				continue;
			}
			if (n == 2)
				break;
			strings[++n] = p + 1;
		}
	}

	out.operand0 = strings[0];
	out.operand1 = strings[1];
	out.operand2 = strings[2];
	return out;
}

//Ошибка в строке: скрипт встает, окно просмотра показывает ее подсвеченной и STOP
void Scripting::comandError(void){
	timber.print("Script:? pc:%d:%s\n", pc, list[pc].buf);
	end = true;
}

//Регистр F0..F9 по имени. Чужое имя это ошибка в строке
float * Scripting::reg(mString<CMD_LEN> & s){
	const int index = s.buf[1] - '0';
	if ((s.buf[0] == 'F') && (index >= 0) && (index <= 9) && (s.buf[2] == 0))
		return &F[index];
	comandError();
	return NULL;
}

//Значение операнда: регистр или число
float Scripting::opValue(mString<CMD_LEN> & s){
	if (s.buf[0] == 'F') {
		const float * r = reg(s);
		return r ? *r : 0.0F;
	}
	if (s.buf[0] == 0) //Нет операнда
		comandError();
	return s.toFloat();
}

//Частота в поле uint16_t: без ограничения 70000 Гц превратились бы в 4464
static uint16_t toFreq(float value){
	if (value < 0.0F) return 0;
	if (value > 65535.0F) return 65535;
	return (uint16_t) value;
}

void Scripting::comandPlusMinus(){

	mString<CMD_LEN> comand;
	comand = list[pc];

	triple = excretionTripleOperand(comand);

	float * r = reg(triple.operand1);
	if (r == NULL)
		return;

	const float value = opValue(triple.operand2);

	if      (triple.operand0 == (char*)"MINUS") *r -= value;
	else if (triple.operand0 == (char*)"PLUS")  *r += value;
	else if (triple.operand0 == (char*)"MUL")   *r *= value;
	else if ((triple.operand0 == (char*)"DIV") && (value != 0.0F)) *r /= value;
	else comandError(); //В том числе деление на ноль
}

void Scripting::generatorComand(void){
		mString<CMD_LEN> comand;
		comand = list[pc];

		triple = excretionTripleOperand(comand);

		uint8_t onoff = 0;

	//╭─ CH1 ───────────────────────────────────────────────────────────────────────╮
	    if (triple.operand0 == (char*)"CH1")                                      //│
	    {                                                                         //│
	      if (triple.operand2 == (char*)"ON") onoff = 1; else onoff = 0;          //│
	                                                                              //│
	      if (triple.operand1 == (char*)"CR")                                     //│
	      {                                                                       //│
	    		  G->CH1.CH_EN = onoff;                                           //│
	      }                                                                       //│
	                                                                              //│
	      if (triple.operand1 == (char*)"AM")                                     //│
	      {                                                                       //│
	    		  G->CH1.AM_EN = onoff;                                           //│
	      }                                                                       //│
	                                                                              //│
	      if (triple.operand1 == (char*)"FM")                                     //│
	      {                                                                       //│
	    		  G->CH1.FM_EN = onoff;                                           //│
	      }                                                                       //│
	                                                                              //│
	      render = 1;                                                             //│
	                                                                              //│
	      return;                                                                 //│
	    }                                                                         //│
	//╰─────────────────────────────────────────────────────────────────────────────╯

	//╭─ CR1 ───────────────────────────────────────────────────────────────────────╮
		if (triple.operand0 == (char*)"CR1")                                      //│
		{                                                                         //│
			//CR1 MOD 01_Sine_12b                                                 //│
		    if (triple.operand1 == (char*)"MOD")                                  //│
		    {                                                                     //│
		      	sprintf(G->CH1.Carrier_mod,"%s.dat",triple.operand2.buf);         //│
	              G->Create_Carrier1();                                           //│
		      render = 1;                                                         //│
		    }                                                                     //│
		                                                                          //│
			//CR1 FR 1000.3                                                       //│
		    if (triple.operand1 == (char*)"FR")                                   //│
			{
		       G->CH1.Carrier_fr = toFreq(opValue(triple.operand2));              //│
			   render = 1;                                                        //│
			}                                                                     //│
                                                                                  //│
		  return;                                                                 //│
		}                                                                         //│
	//╰─────────────────────────────────────────────────────────────────────────────╯

	//╭─ AM1 ───────────────────────────────────────────────────────────────────────╮
	    if (triple.operand0 == (char*)"AM1")                                      //│
	    {                                                                         //│
	      //SEGGER_RTT_printf(0, "╭─ AM1 ─────╮\n");                              //│
	    	                                                                      //│
	      //AM1 FR 1000.3                                                         //│
	      if (triple.operand1 == (char*)"FR")                                     //│
	      {                                                                       //│
               G->CH1.AM_fr = opValue(triple.operand2);                           //│
             render = 1;                                                          //│
	      }                                                                       //│
	                                                                              //│
	      //AM1 MOD 02_HWawe { 1.9ms }                                            //│
	      if (triple.operand1 == (char*)"MOD")                                    //│
	      {                                                                       //│
	        	sprintf(G->CH1.AM_mod,"%s.dat",triple.operand2.buf);              //│
                G->Create_AM_Modulation1();                                       //│
	        render = 1;                                                           //│
	      }                                                                       //│
	      return;                                                                 //│
	    }                                                                         //│
	//╰─────────────────────────────────────────────────────────────────────────────╯
	//╭─ FM1 ───────────────────────────────────────────────────────────────────────╮
	    if (triple.operand0 == (char*)"FM1")                                      //│
	    {                                                                         //│
	      //SEGGER_RTT_printf(0, "╭─ FM1 ─────╮\n");                              //│
	                                                                              //│
	      //FM1 BASE 1234.6                                                       //│
	      if (triple.operand1 == (char*)"BASE")                                   //│
	      {
	    	    G->CH1.FM_Base = toFreq(opValue(triple.operand2));                //│
	    	    G->Refresh_FM_Modulation1();                                      //│
	    	  render = 1;                                                         //│
	      }                                                                       //│
                                                                                  //│
	      //FM1 DEV  123.8                                                        //│
	      if (triple.operand1 == (char*)"DEV")                                    //│
	      {
	    	    G->CH1.FM_Dev = opValue(triple.operand2);                         //│
	    	    G->Refresh_FM_Modulation1();                                      //│
	    	  render = 1;                                                         //│
	      }                                                                       //│
	                                                                              //│
	      //FM1 MOD 02_HWawe                                                      //│
	      if (triple.operand1 == (char*)"MOD")                                    //│
	      {                                                                       //│
	    	 	 sprintf(G->CH1.FM_mod,"%s.dat",triple.operand2.buf);             //│
	    	     G->Create_FM_Modulation1();                                      //│
	    	  render = 1;                                                         //│
	      }                                                                       //│
                                                                                  //│
	      //FM1 FR   3.5                                                          //│
	      if (triple.operand1 == (char*)"FR")                                     //│
	      {                                                                       //│
	    	    G->CH1.FM_mod_fr = opValue(triple.operand2);                      //│
	    	  render = 1;                                                         //│
	      }                                                                       //│
                                                                                  //│
	      return;                                                                 //│
	    }                                                                         //│
	//╰─────────────────────────────────────────────────────────────────────────────╯

	    //Второго канала больше нет, старые скрипты еще упоминают CH2 CR2 AM2 FM2
	    if ((triple.operand0.length() != 3) || (triple.operand0.buf[2] != '2'))
	      comandError();
	}

//Строка файла для list, с переводом и не длиннее CMD_LEN. Хвост длинной строки отбрасываем,
//иначе он станет отдельной строкой и собьет номера в GOTO
bool Scripting::readLine(char * s, int size){
	if (f_gets(s, size, &SDFile) == NULL)
		return false;

	if (strchr(s, '\n') == NULL) {
		char c;
		UINT count;
		while ((f_read(&SDFile, &c, 1, &count) == FR_OK) && count && (c != '\n')) {}
	}

	if (strncmp(s, "\xEF\xBB\xBF", 3) == 0) //Метка UTF-8 от блокнота в начале файла
		memmove(s, s + 3, strlen(s + 3) + 1);

	int len = strcspn(s, "\r\n");
	if (len > CMD_LEN - 2) {
		len = CMD_LEN - 2;
		while ((len > 0) && ((s[len] & 0xC0) == 0x80)) //Без половины символа UTF-8
			len--;
	}
	s[len] = '\n';
	s[len + 1] = 0;
	return true;
}

//Загрузить скрипт в список
void Scripting::load(char *name) {
	char patch[32]; //Полный путь к файлу
	sprintf(patch, "/script/%s.txt", name); //Собираем полный путь в файлу
	int res = f_open(&SDFile, patch, FA_READ);
	//Ошибка открытия картинки с microSD
	if (res != FR_OK) {
		timber.print("\033[01;38;05;51mScripting>\033[01;38;05;196mERROR open>\033[01;38;05;46m%s\n", patch);
		f_close(&SDFile);
		return;
	}

	line = 0;

    timber.print("*** Скрипт %s***\n", patch);
	list[line] = name; //Для того чтобы строки начинались с 1
	timber.print("%d:%s\n", line, list[line].buf);

	line++;

	while ((line < PC_MAX) && readLine(patch, sizeof(patch))) {
		list[line] = patch;
		timber.print("%d:%s", line, list[line].buf);
		line++;
	}
	f_close(&SDFile);

	timber.print("\r\n*** Скрипт Конец ***\r\n");

	lines = line;

	//Без этого скрипт без END выполнит остатки предыдущего
	for (int i = line; i < PC_MAX; i++)
		list[i].clear();

	line = 0;
	pc_exec = 0;
	Name = name;
}

//Выполнить команду по строке pc
void Scripting::CMD_EXE(void){

	if (pc >= PC_MAX) { //Конец списка или GOTO мимо него
		end = true;
		return;
	}

	pc_exec = pc;

	mString<CMD_LEN> comand;
	mString<CMD_LEN> com;

	comand = list[pc];
	com = comand;
	com.truncate(1);

	timber.print("\x1B[01;38;05;208;48;05;234mScript \x1B[01;38;05;7;48;05;234m:");
	timber.print("%d \x1B[01;38;05;10;48;05;234m%s \x1B[0m \n", pc, com.buf);

	char c = comand.buf[0];

	switch (c) {
	//╭─ ENDIF ELSE ───────────────────────────────────────────────────╮
	case 'E':                                                      //  │
		//╭── ELSE ──────────────────────────────────────────────────╮ │
		if (comand.indexOf((char*) "ELSE", 0) == 0)                //│ │
		{                                                          //│ │
		  //Ищем первое ENDIF                                      //│ │
		  uint16_t currentPC = pc;                                 //│ │
		                                                           //│ │
		  while(true)                                              //│ │
		  {                                                        //│ │
		    if (list[currentPC].indexOf((char*)"ENDIF", 0) == 0)   //│ │
		    {                                                      //│ │
		      pc = currentPC; break;                               //│ │
		    }                                                      //│ │
                                                                   //│ │
		    currentPC++;                                           //│ │
		    if (currentPC > (PC_MAX - 1))                          //│ │
		    {                                                      //│ │
		      comandError(); break;                                //│ │
		    }                                                      //│ │
		  }                                                        //│ │
		  break;                                                   //│ │
		}                                                          //│ │
		//╰──────────────────────────────────────────────────────────╯ │
		//╭── ENDIF ─────────────────────────────────────────────────╮ │
		if (comand.indexOf((char*) "ENDIF", 0) == 0){pc++; break;} //│ │
		//╰──────────────────────────────────────────────────────────╯ │
		//╭── END ───────────────────────────────────────────────────╮ │
		if (comand.indexOf((char*) "END", 0) == 0) {               //│ │
			timber.println("Скрипт окончен");                         //│ │
			end = true; return; }                                  //│ │
		//╰──────────────────────────────────────────────────────────╯ │
		comandError();                                             //  │
	    break;                                                     //  │
	//╰────────────────────────────────────────────────────────────────╯

	//╭─ IF ───────────╮
	case 'I':        //│
	    ifComand();  //│
	    break;       //│
	//╰────────────────╯
	//╭── Генератор ──────────╮
	case 'C':               //│
		generatorComand();  //│
		pc++;               //│
		break;              //│
	                        //│
	case 'A':               //│
		generatorComand();  //│
		pc++;               //│
		break;              //│
	                        //│
	case 'F':               //│
		generatorComand();  //│
		pc++;               //│
		break;              //│
	//╰───────────────────────╯
	//╭── MINUS ─────────────╮
	case 'M':              //│
		comandPlusMinus(); //│
		pc++;              //│
		break;             //│
	//╰──────────────────────╯
	//╭── PLUS ─ PRINTR ─ PRINTF ───────────────────────╮
	case 'P':                                         //│
                                                      //│
		if (comand.indexOf((char*) "PLUS", 0) == 0)   //│
		  comandPlusMinus();                          //│
		else if (comand.indexOf((char*) "PRINTF", 0) == 0) //│
		  printF();                                   //│
		else                                          //│
		  comandError();                              //│
                                                      //│
		pc++;                                         //│
		break;                                        //│
	//╰─────────────────────────────────────────────────╯
	//┌── GOTO ────────────────────────────────────┐
	case 'G':
		if (comand.indexOf((char*) "GOTO", 0) == 0) {
			pc = comand.toUint(4);
		}
		else
			comandError();
		break;
	//└────────────────────────────────────────────┘
	//┌── YELD ────────────────────────────────────┐
	case 'Y':
		if (comand.indexOf((char*) "YIELD", 0) == 0) {
			yield = true;
			pc++;
		}
		else
			comandError();
		break;
	//└────────────────────────────────────────────┘
	//┌── WAIT ────────────────────────────────────┐
	case 'W':
		if (comand.indexOf((char*) "WAIT", 0) == 0) {
			waiting = true; //Снимет клик в окне просмотра
			yield = true;
			pc++;
		}
		else
			comandError();
		break;
	//└────────────────────────────────────────────┘
	//┌── DELAY ───────────────────────────────────┐
	case 'D':
		if (comand.indexOf((char*) "DELAY", 0) == 0) {
			triple = excretionTripleOperand(comand);
			const float d = opValue(triple.operand1);
			endTime = uwTick + ((d > 0.0F) ? (uint32_t) d : 0);
			pc++;
		}
		else if (comand.indexOf((char*) "DIV", 0) == 0) {
			comandPlusMinus();
			pc++;
		}
		else
			comandError();
		break;
	//└────────────────────────────────────────────┘
	//┌── TEXT ────────────────────────────────────┐
	case 'T': {
		pc++;
		break;
	}
	//└────────────────────────────────────────────┘

	//┌── LOAD ────────────────────────────────────┐
	case 'L': {
		triple = excretionTripleOperand(comand);
		if (triple.operand0 == (char*)"LOAD")
		{
		   float * r = reg(triple.operand1);
		   if (r)
		     *r = opValue(triple.operand2);
		}
		else
		   comandError();
		pc++;
		break;
	}
	//└────────────────────────────────────────────┘

	default:
		//Пустая строка занимает номер и ничего не делает
		if (comand.buf[strspn(comand.buf, " \t\r\n")] == 0)
			pc++;
		else
			comandError();
		break;
	};

}

// IF R1 < 4500
void Scripting::ifComand(void){
	  mString<CMD_LEN> comand;
	  comand = list[pc];
      if (comand.indexOf((char*) "IF", 0) != 0){comandError();return;}
      comand.remove(0, 3); //Убрали IF
      triple = excretionTripleOperand(comand);

      const float * left = reg(triple.operand0); //Слева только регистр
      if (left == NULL)
    	  return;

      const float f1value = *left;
      const float f2value = opValue(triple.operand2);

      // имеем f1value и f2value
      bool boolResult = false; //Результат сравнения true or false чтобы решить куда дальше

      if      (triple.operand1 == "<" ) boolResult = (f1value <  f2value);
      else if (triple.operand1 == ">" ) boolResult = (f1value >  f2value);
      else if (triple.operand1 == "<=") boolResult = (f1value <= f2value);
      else if (triple.operand1 == ">=") boolResult = (f1value >= f2value);
      else if (triple.operand1 == "!=") boolResult = (f1value != f2value);
      else if (triple.operand1 == "==") boolResult = (f1value == f2value);
      else {comandError();return;}

      if (boolResult)
      {
    	  pc++; //Переход на следующую строку
      }
      else
      {
    	  //Ищем первое ELSE или ENDIF
          uint16_t currentPC = pc;
          while(true)
          {
        	  if (list[currentPC].indexOf((char*)"ELSE", 0) == 0) //+1 к ELSE
        	  {
        		  pc = currentPC + 1 ; break;
        	  }

        	  if (list[currentPC].indexOf((char*)"ENDIF", 0) == 0)
        	  {
        		  pc = currentPC; break;
        	  }

        		  currentPC++;

        	  if (currentPC > (PC_MAX - 1))
        	  {
        		  comandError();
        		  break;
        	  }
          }
      }
	}



//Выполнить команду
void Scripting::command(char * str){

	mString<CMD_LEN> comand;
	comand = str;

	timber.print("\x1B[01;38;05;208;48;05;234mComand \x1B[01;38;05;7;48;05;234m:");
	timber.print(" \x1B[01;38;05;10;48;05;234m%s \x1B[0m \n", comand.buf);


	char c = comand.buf[0];

	switch (c) {
	//╭── Генератор ──────────╮
	case 'C':               //│
		generatorComand();  //│
		break;              //│
	                        //│
	case 'A':               //│
		generatorComand();  //│
		break;              //│
	                        //│
	case 'F':               //│
		generatorComand();  //│
		break;              //│
	//╰───────────────────────╯
	//╭── PLUS ─ PRINTR ─ PRINTF ───────────────────────╮
	case 'P':                                         //│                                                   //│
		if (comand.indexOf((char*) "PRINTF", 0) == 0) //│
		  printF();                                   //│
		break;                                        //│
	//╰─────────────────────────────────────────────────╯
	default:
		timber.print("EXE:? %s\n", pc, comand.buf);
		break;
	};

}








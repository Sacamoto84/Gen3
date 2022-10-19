#include "scripting.h"
#include "HiSpeedDWT.h"

#include "logUART.h"
extern classLog rtt;

Scripting script;

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
	if (uwTick < endTime)
		return;
	endTime = 0;

	while ((yield == false) && (end == false)) {
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
	rtt.print(s);
}

triple_operand Scripting::excretionTripleOperand(mString<CMD_LEN> str){
	triple_operand out;
	char* strings[3];
	str.split(strings,' ');
	out.operand0 = strings[0];
	out.operand1 = strings[1];
	out.operand2 = strings[2];
	out.operand2.truncate(1);
	return out;
}

void Scripting::comandPlusMinus(){

	mString<CMD_LEN> comand;
	comand = list[pc];

	triple = excretionTripleOperand(comand);

	int index = triple.operand1.buf[1] - 0x30;//Индекс первой ячейки 0..9

	if ( triple.operand2.buf[0] == 'F')
	{
	  //Втокой операнд это регистор
	  int secondIndex = triple.operand2.buf[1]-0x30; //Индекс второго регистра

	  //┌── MINUS ────────────────────────────────────┐
	  if (triple.operand0 == (char*)"MINUS") {
		 F[index] = F[index]-F[secondIndex];  //MINUS F* F*
	  }
	  //└────────────────────────────────────────────┘
	  //┌── PLUS ────────────────────────────────────┐
	  if (triple.operand0 == (char*)"PLUS") {
	     F[index] = F[index]+F[secondIndex];          //MINUS F* F*
	  }
	  //└────────────────────────────────────────────┘
	}
    else
	{
	  //Второй операнд это константа
      float fvalue = 0.0F;
	  fvalue = triple.operand2.toFloat();
	  //┌── MINUS ───────────────────────────────────┐
	  if (triple.operand0 == (char*)"MINUS") {
		  F[index] = F[index]-fvalue; //MINUS F* F*
	  }
	  //└────────────────────────────────────────────┘
	  //┌── PLUS ────────────────────────────────────┐
	  if (triple.operand0 == (char*)"PLUS") {
	      F[index] = F[index]+fvalue; //MINUS F* F*
	  }
	  //└────────────────────────────────────────────┘
	}
}

void Scripting::generatorComand(void){
		mString<CMD_LEN> comand;
		comand = list[pc];

		triple = excretionTripleOperand(comand);

	    float value;

		uint8_t chanel = 1;
		chanel = triple.operand0.buf[2] - 0x30; //Номер канала

		uint8_t onoff = 0;

	//╭─ CH1 CH2 ───────────────────────────────────────────────────────────────────╮
	    if ((triple.operand0 == (char*)"CH1")||(triple.operand0 == (char*)"CH2")) //│
	    {                                                                         //│
	      if (triple.operand2 == (char*)"ON") onoff = 1; else onoff = 0;          //│
	                                                                              //│
	      if (triple.operand1 == (char*)"CR")                                     //│
	      {                                                                       //│
	    	  if (chanel == 1)                                                    //│
	    		  G->CH1.CH_EN = onoff;                                           //│
	    	  else                                                                //│
	    		  G->CH2.CH_EN = onoff;                                           //│
	      }                                                                       //│
	                                                                              //│
	      if (triple.operand1 == (char*)"AM")                                     //│
	      {                                                                       //│
	    	  if (chanel == 1)                                                    //│
	    		  G->CH1.AM_EN = onoff;                                           //│
	    	  else                                                                //│
	    		  G->CH2.AM_EN = onoff;                                           //│
	      }                                                                       //│
	                                                                              //│
	      if (triple.operand1 == (char*)"FM")                                     //│
	      {                                                                       //│
	    	  if (chanel == 1)                                                    //│
	    		  G->CH1.FM_EN = onoff;                                           //│
	    	  else                                                                //│
	    		  G->CH2.FM_EN = onoff;                                           //│
	      }                                                                       //│
	                                                                              //│
	      render = 1;                                                             //│
	                                                                              //│
	      return;                                                                 //│
	    }                                                                         //│
	//╰─────────────────────────────────────────────────────────────────────────────╯

	//╭─ CR1 CR2 ───────────────────────────────────────────────────────────────────╮
		if ((triple.operand0 == (char*)"CR1")||(triple.operand0 == (char*)"CR2")) //│
		{                                                                         //│
			//CR[1 2] MOD 01_Sine_12b                                             //│
		    if (triple.operand1 == (char*)"MOD")                                  //│
		    {                                                                     //│
		       if (chanel == 1){                                                  //│
		      	sprintf(G->CH1.Carrier_mod,"%s.dat",triple.operand2.buf);         //│
	              G->Create_Carrier1();                                           //│
		       }                                                                  //│
		       else                                                               //│
		       {                                                                  //│
		      	sprintf(G->CH2.Carrier_mod,"%s.dat",triple.operand2.buf);         //│
		      	G->Create_Carrier2();                                             //│
		       }                                                                  //│
		      render = 1;                                                         //│
		    }                                                                     //│
		                                                                          //│
			//CR[1 2] FR 1000.3                                                   //│
		    if (triple.operand1 == (char*)"FR")                                   //│
			{
		       if (triple.operand2.buf[0] == 'F')
		       {
		    	 value = F[triple.operand2.buf[1]-0x30];
		       }
		       else
			     value = triple.operand2.toFloat();

			   if (chanel == 1)                                                   //│
			     G->CH1.Carrier_fr = (uint16_t)value;                             //│
			   else                                                               //│
			     G->CH2.Carrier_fr = (uint16_t)value;                             //│
			   render = 1;                                                        //│
			}                                                                     //│
                                                                                  //│
		  return;                                                                 //│
		}                                                                         //│
	//╰─────────────────────────────────────────────────────────────────────────────╯

	//╭─ AM1 AM2 ───────────────────────────────────────────────────────────────────╮
	    if ((triple.operand0 == (char*)"AM1")||(triple.operand0 == (char*)"AM2")) //│
	    {                                                                         //│
	      //SEGGER_RTT_printf(0, "╭─ AM1 AM2 ─╮\n");                              //│
	    	                                                                      //│
	      //AM[1 2] FR 1000.3                                                     //│
	      if (triple.operand1 == (char*)"FR")                                     //│
	      {                                                                       //│
		     if (triple.operand2.buf[0] == 'F')
		     {
		       value = F[triple.operand2.buf[1]-0x30];
		     }
		     else
			   value = triple.operand2.toFloat();

             if (chanel == 1)                                                     //│
               G->CH1.AM_fr = value;                                              //│
             else                                                                 //│
               G->CH2.AM_fr = value;                                              //│
             render = 1;                                                          //│
	      }                                                                       //│
	                                                                              //│
	      //AM[1 2] MOD 02_HWawe { 1.9ms }                                        //│
	      if (triple.operand1 == (char*)"MOD")                                    //│
	      {                                                                       //│
	         if (chanel == 1){                                                    //│
	        	sprintf(G->CH1.AM_mod,"%s.dat",triple.operand2.buf);              //│
                G->Create_AM_Modulation1();                                       //│
	         }                                                                    //│
	         else                                                                 //│
	         {                                                                    //│
	        	sprintf(G->CH2.AM_mod,"%s.dat",triple.operand2.buf);              //│
	        	G->Create_AM_Modulation2();                                       //│
	         }                                                                    //│
	        render = 1;                                                           //│
	      }                                                                       //│
	      return;                                                                 //│
	    }                                                                         //│
	//╰─────────────────────────────────────────────────────────────────────────────╯
	//╭─ FM1 FM2 ───────────────────────────────────────────────────────────────────╮
	    if ((triple.operand0 == (char*)"FM1")||(triple.operand0 == (char*)"FM2")) //│
	    {                                                                         //│
	      //SEGGER_RTT_printf(0, "╭─ FM1 FM2 ─╮\n");                              //│
	                                                                              //│
	      //FM[1 2] BASE 1234.6                                                   //│
	      if (triple.operand1 == (char*)"BASE")                                   //│
	      {
		      if (triple.operand2.buf[0] == 'F')
		      {
		    	value = F[triple.operand2.buf[1]-0x30];
		      }
		      else
			    value = triple.operand2.toFloat();

	    	  if (chanel == 1){                                                   //│
	    	    G->CH1.FM_Base = (uint16_t)value;                                 //│
	    	    G->Refresh_FM_Modulation1();                                      //│
	    	  }                                                                   //│
	    	  else{                                                               //│
	    	    G->CH2.FM_Base = (uint16_t)value;                                 //│
	    	    G->Refresh_FM_Modulation2();                                      //│
	    	  }                                                                   //│
	    	  render = 1;                                                         //│
	      }                                                                       //│
                                                                                  //│
	      //FM[1 2] DEV  123.8                                                    //│
	      if (triple.operand1 == (char*)"DEV")                                    //│
	      {
		      if (triple.operand2.buf[0] == 'F')
		      {
		    	value = F[triple.operand2.buf[1]-0x30];
		      }
		      else
			    value = triple.operand2.toFloat();

	    	  if (chanel == 1){                                                   //│
	    	    G->CH1.FM_Dev = value;                                            //│
	    	    G->Refresh_FM_Modulation1();                                      //│
	    	  }                                                                   //│
	    	  else{                                                               //│
	    	    G->CH2.FM_Dev = value;                                            //│
	    	    G->Refresh_FM_Modulation2();                                      //│
	    	  }                                                                   //│
	    	  render = 1;                                                         //│
	      }                                                                       //│
	                                                                              //│
	      //FM[1 2] MOD 02_HWawe                                                  //│
	      if (triple.operand1 == (char*)"MOD")                                    //│
	      {                                                                       //│
	    	  if (chanel == 1){                                                   //│
	    	 	 sprintf(G->CH1.FM_mod,"%s.dat",triple.operand2.buf);             //│
	    	     G->Create_FM_Modulation1();                                      //│
	    	  }                                                                   //│
	    	  else                                                                //│
	    	  {                                                                   //│
	    	 	 sprintf(G->CH2.FM_mod,"%s.dat",triple.operand2.buf);             //│
	    	 	 G->Create_FM_Modulation2();                                      //│
	    	  }                                                                   //│                                                              //│
	    	  render = 1;                                                         //│
	      }                                                                       //│
                                                                                  //│
	      //FM[1 2] FR   3.5                                                      //│
	      if (triple.operand1 == (char*)"FR")                                     //│
	      {                                                                       //│
		      if (triple.operand2.buf[0] == 'F')
		      {
		        value = F[triple.operand2.buf[1]-0x30];
		      }
		      else
			    value = triple.operand2.toFloat();

	    	  if (chanel == 1)                                                    //│
	    	    G->CH1.FM_mod_fr = value;                                         //│
	    	  else                                                                //│
	    	    G->CH2.FM_mod_fr = value;                                         //│
	    	  render = 1;                                                         //│
	      }                                                                       //│
                                                                                  //│
	      return;                                                                 //│
	    }                                                                         //│
	//╰─────────────────────────────────────────────────────────────────────────────╯
	}

//Загрузить скрипт в список
void Scripting::load(char *name) {
	char patch[32]; //Полный путь к файлу
	sprintf(patch, "/script/%s.txt", name); //Собираем полный путь в файлу
	int res = f_open(&SDFile, patch, FA_READ);
	//Ошибка открытия картинки с microSD
	if (res != FR_OK) {
		rtt.print("\033[01;38;05;51mScripting>\033[01;38;05;196mERROR open>\033[01;38;05;46m%s\n", patch);
		f_close(&SDFile);
		return;
	}

	line = 0;

    rtt.print("*** Скрипт %s***\n", patch);
	list[line] = name; //Для того чтобы строки начинались с 1
	rtt.print("%d:%s\n", line, list[line].buf);

	line++;

	while (f_gets(patch, 32, &SDFile)) {
		list[line] = patch;
		rtt.print("%d:%s", line, list[line].buf);
		line++;
	}
	f_close(&SDFile);

	rtt.print("\r\n*** Скрипт Конец ***\r\n");

	line = 0;
	Name = name;
}

//Выполнить команду по строке pc
void Scripting::CMD_EXE(void){

	mString<CMD_LEN> comand;
	mString<CMD_LEN> com;

	comand = list[pc];
	com = comand;
	com.truncate(1);

	rtt.print("\x1B[01;38;05;208;48;05;234mScript \x1B[01;38;05;7;48;05;234m:");
	rtt.print("%d \x1B[01;38;05;10;48;05;234m%s \x1B[0m \n", pc, com.buf);

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
		    if (currentPC > (PC_MAX - 1)) break;                   //│ │
		  }                                                        //│ │
		  break;                                                   //│ │
		}                                                          //│ │
		//╰──────────────────────────────────────────────────────────╯ │
		//╭── ENDIF ─────────────────────────────────────────────────╮ │
		if (comand.indexOf((char*) "ENDIF", 0) == 0){pc++; break;} //│ │
		//╰──────────────────────────────────────────────────────────╯ │
		//╭── END ───────────────────────────────────────────────────╮ │
		if (comand.indexOf((char*) "END", 0) == 0) {               //│ │
			rtt.println("Скрипт окончен");                         //│ │
			end = true; return; }                                  //│ │
		//╰──────────────────────────────────────────────────────────╯ │
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
                                                      //│                                                          //│
		if (comand.indexOf((char*) "PRINTF", 0) == 0) //│
		  printF();                                   //│
                                                      //│
		pc++;                                         //│
		break;                                        //│
	//╰─────────────────────────────────────────────────╯
	//┌── GOTO ────────────────────────────────────┐
	case 'G':
		if (comand.indexOf((char*) "GOTO", 0) == 0) {
			pc = comand.toUint(4);
		}
		break;
	//└────────────────────────────────────────────┘
	//┌── YELD ────────────────────────────────────┐
	case 'Y':
		if (comand.indexOf((char*) "YIELD", 0) == 0) {
			yield = true;
			pc++;
		}
		break;
	//└────────────────────────────────────────────┘
	//┌── DELAY ───────────────────────────────────┐
	case 'D':
		if (comand.indexOf((char*) "DELAY", 0) == 0) {
			uint32_t d = comand.toUint(5);
			endTime = uwTick + d;
			pc++;
		}
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
		   int index = triple.operand1.buf[1]-0x30;
		   F[index] = triple.operand2.toFloat();
		}
		pc++;
		break;
	}
	//└────────────────────────────────────────────┘

	default:
		rtt.print("Script:? pc:%d:%s\n", pc, comand.buf);
		pc++;
		if (pc >= PC_MAX)
		  end = true;
		break;
	};

}

// IF R1 < 4500
void Scripting::ifComand(void){
	  mString<CMD_LEN> comand;
	  comand = list[pc];
	  comand.truncate(1);
      if (comand.indexOf((char*) "IF", 0) != 0){pc++;return;}
      comand.remove(0, 3); //Убрали IF
      triple = excretionTripleOperand(comand);

      float f1value = 0.0F;
      float f2value = 0.0F;

      f1value = F[triple.operand0.buf[1] - 0x30];

      if ((triple.operand2.buf[0] == 'F'))
      {
          f2value = F[triple.operand2.buf[1] - 0x30];
      }
      else
    	  f2value = triple.operand2.toFloat();


      // имеем f1value и f2value
      bool boolResult = false; //Результат сравнения true or false чтобы решить куда дальше

      if ((triple.operand1 == "<" )&&(f1value <  f2value)) boolResult = true;
      if ((triple.operand1 == ">" )&&(f1value >  f2value)) boolResult = true;
      if ((triple.operand1 == "<=")&&(f1value <= f2value)) boolResult = true;
      if ((triple.operand1 == ">=")&&(f1value >= f2value)) boolResult = true;
      if ((triple.operand1 == "!=")&&(f1value != f2value)) boolResult = true;
      if ((triple.operand1 == "==" )&&(f1value == f2value)) boolResult = true;

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
        		  break;
          }
      }
	}



//Выполнить команду
void Scripting::command(char * str){

	mString<CMD_LEN> comand;
	comand = str;

	rtt.print("\x1B[01;38;05;208;48;05;234mComand \x1B[01;38;05;7;48;05;234m:");
	rtt.print(0, " \x1B[01;38;05;10;48;05;234m%s \x1B[0m \n", comand.buf);


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
		rtt.print("EXE:? %s\n", pc, comand.buf);
		break;
	};

}








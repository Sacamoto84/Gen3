#include "Page.h"
#include "global_define.h"
#include "stdio.h"

	 typedef struct
	  {
	    uint32_t r0;
	    uint32_t r1;
	    uint32_t r2;
	    uint32_t r3;
	    uint32_t r12;
	    uint32_t lr;
	    uint32_t pc;
	    uint32_t psr;
	  } stack_ptr; //Указатель на текущее значение стека(SP)

extern "C" void PAGE_HardFault (stack_ptr * SP)
{
	//char str[32];
	tft.Fill16(tft.RGB565(255,0,0));

	//gfxfont.Puts(20, 20, (char *)"HardFault", tft.RGB565(0,0,0));

	//sprintf(str,"R0:0x%08X",(unsigned int)SP->r0);
	//gfxfont.Puts(0, 40, str, WHITE);

	//sprintf(str,"R1:0x%08X",(unsigned int)SP->r1);
	//gfxfont.Puts(0, 60, str, WHITE);

	//sprintf(str,"R2:0x%08X",(unsigned int)SP->r2);
	//gfxfont.Puts(0, 80, str, WHITE);

	//sprintf(str,"R3:0x%08X",(unsigned int)SP->r3);
	//gfxfont.Puts(0, 100, str, WHITE);

	//sprintf(str,"R12:0x%08X",(unsigned int)SP->r12);
	//gfxfont.Puts(0, 120, str, WHITE);

	//sprintf(str,"lr:0x%08X",(unsigned int)SP->lr);
	//gfxfont.Puts(0, 140, str, WHITE);

	//sprintf(str,"pc:0x%08X",(unsigned int)SP->pc);
	//gfxfont.Puts(0, 160, str, WHITE);

	//sprintf(str,"psr:0x%08X",(unsigned int)SP->psr);
	//gfxfont.Puts(0, 180, str, WHITE);

	tft.ST7789_Update();
}



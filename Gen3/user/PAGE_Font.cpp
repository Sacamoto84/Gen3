#include "Page.h"
#include "stdio.h"
#include "Button.h"
#include "TFT_color.h"
#include "global_define.h"
#include "TFT.h"

extern button_typedef button;
extern TFT tft;

void PAGE_Font(void)
{
	while(1)
	{
		button.tick();
		tft.Fill16(COLOR_BACKGROUND);

		if (button.isRelease())
					return;

				tft.driver.ST7789_UpdateDMA16bitV2();

	}
}

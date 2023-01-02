#include <TFT_Wiget_Animated_Rectagle.h>
#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "TFT_gif.h"


TFT_gif gifw;

void PAGE_Volume(void)
{
	int i = 0;

    rtt.print("Wiget\n");

    TFT_gif gif;
    gif.init(&tft);
    gif.setName((char *)"tA");

    gif.setDelay(0);

    gif.setXY(10, 10);

    gif.trigger = HOVER;

    TFT_gif gif2;
    gif2.init(&tft);
    gif2.setName((char *)"tA");

    gif2.setDelay(0);


    gif2.setXY(100, 10);

    gif2.trigger = LOOP;


    TFT_gif gif3;
    gif3.init(&tft);
    gif3.setName((char *)"tA");

    gif3.setDelay(0);


    gif3.setXY(10, 150);

    gif3.trigger = MORPH;

    TFT_gif gif4;
    gif4.init(&tft);
    gif4.setName((char *)"tA");

    gif4.setDelay(0);


    gif4.setXY(100, 150);

    gif4.trigger = BOOMERANG;


    tft.Fill16(COLOR_DARKBLUE);

    tft.driver.ST7789_UpdateDMA16bitV3();

	while (1) {
		button.tick();

		if (Encoder.Left) {
			Encoder.Left = 0;
			i+=1;
			if (i>2) i=2;

			gif .command(STOP);
			gif2.command(STOP);
			gif3.command(STOP);
			gif4.command(STOP);

		}

		if (Encoder.Right) {
			Encoder.Right = 0;
			i-=1;
			if(i<0) i=0;

			gif .command(PLAY);
			gif2.command(PLAY);
			gif3.command(PLAY);
			gif4.command(PLAY);
		}

		tft.Fill16(COLOR_DARKBLUE);
		//tft.Font_Smooth_drawStr(30, 30, "Wiget");


		gif.run();
		gif2.run();
		gif3.run();
		gif4.run();

		TimerT5.Start();
		tft.driver.ST7789_Update(gif.info());
		TimerT5.Loger((char*)"tft.ST7789_Update");

		tft.driver.ST7789_Update(gif2.info());
		tft.driver.ST7789_Update(gif3.info());
		tft.driver.ST7789_Update(gif4.info());








		//tft.ST7789_UpdateDMA16bitV3();

	    if (button.isClick())
					return;	
		}
}

#include "main.h"
#include "TFT.h"
extern TFT tft CCMRAM;

extern "C" void TFT_DMA2_Stream5_Complete(void)
{
	if (DMA2->HISR & DMA_HISR_TCIF5)
		tft.driver.DMA_TX_Complete = 1;
}

#include "main.h"
#include "TFT.h"
#include "dma.h"

extern DMA_HandleTypeDef hdma_spi1_tx;
extern TFT tft CCMRAM;

//DMA SPI1 TX
extern "C" void DMA2_Stream3_IRQHandler(void)
{
	if (DMA2->LISR & DMA_LISR_TCIF3)
		      tft.DMA_TX_Complete = 1;

  HAL_DMA_IRQHandler(&hdma_spi1_tx);
}


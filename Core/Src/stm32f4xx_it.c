#include "main.h"
#include "stm32f4xx_it.h"

extern int * U3_DMA_TX_Complete;

 struct
 {
   uint32_t r0;
   uint32_t r1;
   uint32_t r2;
   uint32_t r3;
   uint32_t r12;
   uint32_t lr;
   uint32_t pc;
   uint32_t psr;
 }*stack_ptr; //Указатель на текущее значение стека(SP)

extern void LOG(char * tag, char TYPE, char * text);
extern void PAGE_HardFault ();

extern DMA_HandleTypeDef hdma_usart3_tx;
extern UART_HandleTypeDef huart3;

void NMI_Handler(void){while (1){}}

void HardFault_Handler(void)
{ while (1){PAGE_HardFault(stack_ptr);} }

void MemManage_Handler(void){while (1){}}
void BusFault_Handler(void){while (1){}}
void UsageFault_Handler(void){while (1){}}
void SVC_Handler(void){}
void DebugMon_Handler(void){}
void PendSV_Handler(void){}
void SysTick_Handler(void){ HAL_IncTick(); }

void DMA1_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart3_tx);
}

void USART3_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart3);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  { *U3_DMA_TX_Complete = 1; }
}



#include "main.h"
#include "crc.h"
#include "dac.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "sdio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

extern void main_cpp(void);

void SystemClock_Config(void);

uint16_t res;


#define APPLICATION_ADDRESS    0x08040000
static void Go_To_User_App(void)
{
    uint32_t app_jump_address;

    typedef void(*pFunction)(void);
    pFunction Jump_To_Application;

     __disable_irq();

    app_jump_address = *( uint32_t*) (APPLICATION_ADDRESS + 4);
    Jump_To_Application = (pFunction)app_jump_address;
	SCB->VTOR = APPLICATION_ADDRESS;
    __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
    Jump_To_Application();
}


int main(void)
{
	__disable_irq(); //????????? ??????????
	//SCB->VTOR = 0x8060000;
	SCB->VTOR = 0x80C0000;

	uint32_t *p;
	p = (uint32_t *)(0x10000000);
	int count = 16384;
	while(count--)
		*p++ = 0;

	__enable_irq(); //????????? ??????????

	//Go_To_User_App();

	//while(1){}



  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  SystemClock_Config();

  MX_DMA_Init();

  MX_GPIO_Init();
  MX_CRC_Init();
  MX_DAC_Init();
  MX_I2C1_Init();
  MX_SDIO_SD_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  MX_FATFS_Init();
  MX_TIM5_Init();

  MX_TIM7_Init();
  BSP_SD_Init();

  HAL_UART_MspInit(&huart3);

  //Encoder
  TIM2->SR &= ~TIM_SR_UIF; // clear UIF flag
  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_Base_Start_IT(&htim2);
  TIM2->DIER |= TIM_DIER_UIE;

  //HAL_TIM_Base_Start(&htim2);
  //HAL_TIM_Base_Start_IT(&htim2);

  TIM1->CCR1 = 75;
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

  res = f_mount(&SDFatFS, SDPath, 1); //Mount MicroSd

  main_cpp();

  //USART3->CR1 |= USART_CR1_OVER8;
  //USART3->BRR = 2 <<4 | 2;

  //SEGGER_RTT_WriteString(0, "\033[01;03;38;05;226;48;05;24m");
  //SEGGER_RTT_WriteString(0, " Генератор V84 \x1B[0m\r\n");

  //if (f_open(&SDFile, "b1.bmp", FA_OPEN_EXISTING | FA_READ)
  //		== FR_OK) {
  //	SEGGER_RTT_WriteString(0, "Тест f_open == OK\r\n");
  //	f_close(&SDFile);
  //} else {
  //	SEGGER_RTT_WriteString(0, "Тест f_open == ERROR\r\n");
  //}

	while (1) {
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

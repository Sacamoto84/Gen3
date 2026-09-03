/* Includes ------------------------------------------------------------------*/

#include "stm32f4xx_hal.h"

#define APPLICATION_ADDRESS    0x080C0000 //0x08060000

void Go_To_User_App(void)
{
    uint32_t app_jump_address;
	
    typedef void(*pFunction)(void);//объявляем пользовательский тип
    pFunction Jump_To_Application; //и создаём переменную этого типа

     __disable_irq();//запрещаем прерывания
		
    app_jump_address = *( uint32_t*) (APPLICATION_ADDRESS + 4);    //извлекаем адрес перехода из вектора Reset
    Jump_To_Application = (pFunction)app_jump_address;            //приводим его к пользовательскому типу
	     SCB->VTOR = APPLICATION_ADDRESS;//переносим начало вектора прерываний по указанному адресу
      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);          //устанавливаем SP приложения                                           
    Jump_To_Application();		                        //запускаем приложение	
}


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

	Go_To_User_App();//прыгаем в основную программу

  while (1)
  {

  }
}

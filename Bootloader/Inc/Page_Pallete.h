#ifndef __PAGE_PALLETE_H
#define __PAGE_PALLETE_H

#ifdef __cplusplus
 extern "C" {
#endif 
	 
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"	 
	 
#define pColor_0 RGB565(158,158,158)    // Фон заливки, окантовки
#define pColor_1 RGB565(77,85,74)       // Фон основной задний фон	 
#define pColor_2 RGB565(148,165,142)    // Светлый фон выбранной кнопки
#define pColor_3 RGB565(208,231,199)    // Светлая часть окантовки светлой кнопки
#define pColor_4 RGB565(89,99,85)       // Темная часть окантовки светлой кнопки
#define pColor_5 RGB565(166,170,165)    // Светлая часть окантовки темной кнопки
#define pColor_6 RGB565(46,51,44)       // Темная часть окантовки темной кнопки
	 	 
#ifdef __cplusplus
}
#endif

#endif
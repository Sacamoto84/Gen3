#ifndef PAGE_BATTERY_H_
#define PAGE_BATTERY_H_

#include <stdint.h>
#include <stdbool.h>

void Battery_Init(void);
void Battery_Load(void);
bool Battery_Save(void);

float Battery_GetAdcVoltage(void);
float Battery_GetVoltage(void);
float Battery_GetCellVoltage(void);

float Battery_GetCoef(void);
void Battery_SetCoef(float k);

uint8_t Battery_GetCells(void);
void Battery_SetCells(uint8_t cells);

void PAGE_Battery(void);

#endif /* PAGE_BATTERY_H_ */

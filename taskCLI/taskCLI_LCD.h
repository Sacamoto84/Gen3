/*
 * taskCLI_LCD.h
 *
 *  Created on: 4 дек. 2021 г.
 *      Author: Ivan
 */

#ifndef TASKCLI_LCD_H_
#define TASKCLI_LCD_H_

void LCD_Invers_Callback(cmd *c);
void LCD_Gamma_P_Callback(cmd *c);
void LCD_Gamma_N_Callback(cmd *c);
void LCD_Block_Callback(cmd *c);
void LCD_SoftwareReset_Callback(cmd *c);
void LCD_VMCTR1_Callback(cmd *c);
void LCD_Frame_Rate_Control_Callback(cmd *c);
void LCD_Read_vcom1_Callback(cmd *c);
void LCD_Gamma_BMP_Callback(cmd *c);
void LCD_GAMSET_Callback(cmd *c);
void LCD_Init_Callback(cmd *c);
void LCD_Init_Custom_Callback(cmd *c);
void LCD_Read_backlight_Callback(cmd *c);
void LCD_Read_fps_Callback(cmd *c);
void LCD_Read_gammaset_Callback(cmd *c) ;
void LCD_Save_backlight_Callback(cmd *c);
void LCD_Read_gn_Callback(cmd *c);
void LCD_Save_fps_Callback(cmd *c);
void LCD_Save_gammaset_Callback(cmd *c);
void LCD_Save_vcom1_Callback(cmd *c);
void LCD_Save_gp_Callback(cmd *c);
void LCD_Save_gn_Callback(cmd *c);
void LCD_Read_gp_Callback(cmd *c);

#endif /* TASKCLI_LCD_H_ */



#include "taskCLI.h"
#include "global_define.h"

#ifdef USE_CLI

extern TFT tft;

#define LCD_D 1
#define LCD_C 0

struct cmdBuf {
  uint8_t command;   // ST7735 command byte
  uint8_t delay;     // ms delay after
  uint8_t len;       // length of parameter data
  uint8_t data[16];  // parameter data
};

Command LCD_Invers;  //Команда инверсии экрана 0 1
Command LCD_Gamma_P; //Команда положительной гаммы // Готово
Command LCD_Gamma_N; //Команда положительной гаммы // Готово
Command LCD_Block;   //Блокировка обновления экрана
Command LCD_SoftwareReset;   //Програмный брос экрана
Command LCD_Gamma_BMP; //Показ картинки GAMMA

Command LCD_GAMSET; //GAMSET (Gamma Set)

Command LCD_Frame_Rate_Control; //B1 FRMCTR1 (Frame Rate Control)

Command LCD_VMCTR1;
Command LCD_Init;

Command LCD_Init_Custom;

//Команды чтение настроек
Command LCD_Read_backlight;
Command LCD_Read_fps;
Command LCD_Read_gammaset;
Command LCD_Read_vcom1;
Command LCD_Read_gp;
Command LCD_Read_gn;

//Команды сохранения настроек
Command LCD_Save_backlight;    //Ok
Command LCD_Save_fps;
Command LCD_Save_gammaset;
Command LCD_Save_vcom1;
Command LCD_Save_gp;
Command LCD_Save_gn;

//Команда инверсии экрана
void LCD_Invers_Callback(cmd *c) {
	Command cmd(c); // Create wrapper object
	Argument invertArg = cmd.getArgument(0); //("invert");
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);

	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_Select();

	//if (argIntValue)
	//	ST7735_Display_Invert(&LCD, 1);
	//else
	//	ST7735_Display_Invert(&LCD, 0);

	if (tft.LCD->GPIO_CS != NULL)
		tft.SPI.LCD_UnSelect();

	SEGGER_RTT_printf(0, ">LCD_Invers>%d\n", argIntValue);
}

void LCD_Gamma_P_Callback(cmd *c) {

	struct cmdBuf initializers = { 0xE0, 0, 16, { 0x02, 0x1C, 0x07, 0x12, 0x37,
			0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10 } };

	Command cmd(c); // Create wrapper object

	int argNum = cmd.countArgs(); // Get number of arguments

	for (int i = 0; i < argNum; i++) {
		Argument arg = cmd.getArg(i);
		String argValue = arg.getValue();
		int argIntValue = StrToInt(argValue);

		initializers.data[i] = argIntValue;
	}

	if (tft.LCD->GPIO_CS != NULL) {
		  tft.SPI.LCD_Select();
		}

	tft.ST7789_Transmit_Array(LCD_C, &initializers.command, 1);
	tft.ST7789_Transmit_Array(LCD_D, initializers.data, 16);

	if (tft.LCD->GPIO_CS != NULL)
		tft.SPI.LCD_UnSelect();

	SEGGER_RTT_printf(0, ">LCD_Gamma_P>\n");
}

void LCD_Gamma_N_Callback(cmd *c) {

	struct cmdBuf initializers = { 0xE1, 0, 16, { 0x03, 0x1d, 0x07, 0x06, 0x2E,
			0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10 } };

	Command cmd(c); // Create wrapper object

	int argNum = cmd.countArgs(); // Get number of arguments

	for (int i = 0; i < argNum; i++) {
		Argument arg = cmd.getArg(i);
		String argValue = arg.getValue();
		int argIntValue = StrToInt(argValue);

		initializers.data[i] = argIntValue;
	}
	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_Select();


	tft.ST7789_Transmit_Array(LCD_C, &initializers.command, 1);
	tft.ST7789_Transmit_Array(LCD_D, initializers.data, 16);

	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_UnSelect();

	SEGGER_RTT_printf(0, ">LCD_Gamma_N>\n");
}










//VMCTR1 (VCOM Control 1) 0xC5
void LCD_VMCTR1_Callback(cmd *c) {
	Command cmd(c); // Create wrapper object

	// Get arguments
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);

	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_Select();

	tft.SPI.SendCmd(0xC5);
	tft.SPI.SendData(argIntValue);

	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_UnSelect();

	SEGGER_RTT_printf(0, ">LCD_VMCTR1\n");
}

void LCD_Frame_Rate_Control_Callback(cmd *c) {
	struct cmdBuf initializers = { 0xB1, 0, 3, { 0x01, 0x2C, 0x2D } };
	Command cmd(c); // Create wrapper object
	int argNum = cmd.countArgs(); // Get number of arguments
	for (int i = 0; i < argNum; i++) {
		Argument arg = cmd.getArg(i);
		String argValue = arg.getValue();
		int argIntValue = StrToInt(argValue);

		initializers.data[i] = argIntValue;
	}

	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_Select();

	tft.ST7789_Transmit_Array(LCD_C, &initializers.command, 1);
	tft.ST7789_Transmit_Array(LCD_D, initializers.data, 3);

	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_UnSelect();

	SEGGER_RTT_printf(0, ">LCD_Frame_Rate_Control_Callback>\n");
}

//Чтение из файла настройки VCOM1
void LCD_Read_vcom1_Callback(cmd *c) {

	int value;
	char str1[16] = { 0 };
	char str2[16] = { 0 };

	if (f_open(&SDFile, "/Config/lcd_vcom.txt", FA_OPEN_EXISTING | FA_READ)
			== FR_OK) {
		SEGGER_RTT_printf(0, ">Read Ok /Config/lcd_vcom.txt\n");
		f_gets((char*) str1, sizeof(str1), &SDFile);
		f_close(&SDFile);
		value = atoi(str1);
		sprintf(str2, "%d", value);
		BT_Send_String_to_Host(98, str2);
	} else {
		SEGGER_RTT_printf(0, ">Read Error /Config/lcd_vcom.txt\n");
	}

	SEGGER_RTT_printf(0, ">LCD_Read_vcom1 %d\n", value);
}

void LCD_Gamma_BMP_Callback(cmd *c) {

	Command cmd(c); // Create wrapper object

	// Get arguments
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);

	switch (argIntValue) {
	case 1:
		tft.BMP_From_File( 0, 0, (char*)"/Gamma/Gamma1.bmp");
		break;
	case 2:
		tft.BMP_From_File( 0, 0, (char*)"/Gamma/Gamma2.bmp");
		break;
	case 3:
		tft.BMP_From_File( 0, 0, (char*)"/Gamma/Gamma3.bmp");
		break;
	case 4:
		tft.BMP_From_File( 0, 0, (char*)"/Gamma/Gamma4.bmp");
		break;
	case 5:
		tft.BMP_From_File( 0, 0, (char*)"/Gamma/Gamma5.bmp");
		break;
	}

	tft.ST7789_Update();

	SEGGER_RTT_printf(0, ">LCD_Gamma_BMP>\n");
}

void LCD_GAMSET_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0); //("invert");
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);
	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_Select();
	tft.SPI.SendCmd(0x26);
	tft.SPI.SendData(argIntValue);
	if (tft.LCD->GPIO_CS != NULL)  tft.SPI.LCD_UnSelect();
	SEGGER_RTT_printf(0, ">LCD_GAMSET\n");
}



void LCD_Init_Custom_Callback(cmd *c) {
	//ST7735_custom_init();
}

//Чтение из файла настройки яркости
void LCD_Read_backlight_Callback(cmd *c) {

	int value = -1;
	char str1[16] = { 0 };
	char str2[16] = { 0 };

	if (f_open(&SDFile, "/Config/lcd_bl.txt", FA_OPEN_EXISTING | FA_READ)
			== FR_OK) {
		SEGGER_RTT_printf(0, ">Read Ok /Config/lcd_bl.txt\n");
		f_gets((char*) str1, sizeof(str1), &SDFile);
		f_close(&SDFile);
		value = atoi(str1);
		sprintf(str2, "%d", value);
		BT_Send_String_to_Host(98, str2);
	} else {
		SEGGER_RTT_printf(0, ">Read Error /Config/lcd_bl.txt\n");
	}

	SEGGER_RTT_printf(0, ">LCD_Read_backlight %d\n", value);
}

void LCD_Read_fps_Callback(cmd *c) {
	int value[3];
	char str1[16] = { 0 };
	char str2[16] = { 0 };

	if (f_open(&SDFile, "/Config/lcd_fps.txt", FA_OPEN_EXISTING | FA_READ)
			== FR_OK) {
		SEGGER_RTT_printf(0, ">Read Ok\n");

		f_gets((char*) str1, sizeof(str1), &SDFile);
		value[0] = atoi(str1);

		f_gets((char*) str1, sizeof(str1), &SDFile);
		value[1] = atoi(str1);

		f_gets((char*) str1, sizeof(str1), &SDFile);
		value[2] = atoi(str1);

		sprintf(str2, "%d %d %d", value[0], value[1], value[2]);
		BT_Send_String_to_Host(98, str2);
		f_close(&SDFile);
	} else {
		SEGGER_RTT_printf(0, ">Read Error /Config/lcd_fps.txt\n");
	}

	SEGGER_RTT_printf(0, ">LCD_Read_fps %s\n", str2);
}

//Чтение из файла настройки gammaset
void LCD_Read_gammaset_Callback(cmd *c) {

	int value;
	char str1[16] = { 0 };
	char str2[16] = { 0 };

	if (f_open(&SDFile, "/Config/lcd_gs.txt", FA_OPEN_EXISTING | FA_READ)
			== FR_OK) {
		SEGGER_RTT_printf(0, ">Read Ok /Config/lcd_gs.txt\n");
		f_gets((char*) str1, sizeof(str1), &SDFile);
		f_close(&SDFile);
		value = atoi(str1);
		sprintf(str2, "%d", value);
		BT_Send_String_to_Host(98, str2);
	} else {
		SEGGER_RTT_printf(0, ">Read Error /Config/lcd_gs.txt\n");
	}

	SEGGER_RTT_printf(0, ">LCD_Read_gammaset %d\n", value);
}

//Сохранение в файл настройки яркости
void LCD_Save_backlight_Callback(cmd *c) {
	UINT testByte;
	char str1[32] = { 0 };
	Command cmd(c); // Create wrapper object
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);

	if (f_open(&SDFile, "/Config/lcd_bl.txt", FA_CREATE_ALWAYS | FA_WRITE)== FR_OK) {
		sprintf(str1, "%d\n", argIntValue);
		f_write(&SDFile, &str1, strlen(str1), &testByte);
		SEGGER_RTT_printf(0, ">Save Ok /Config/lcd_bl.txt\n");
		f_close(&SDFile);
	} else {
		SEGGER_RTT_printf(0, ">Save Error\n");
	}
	SEGGER_RTT_printf(0, ">LCD_Save_backlight %d\n", argIntValue);
}

void LCD_Read_gn_Callback(cmd *c) {
	int value[32];
	char str1[64] = { 0 };
	char str2[64] = { 0 };

	if (f_open(&SDFile, "/Config/lcd_gn.txt", FA_OPEN_EXISTING | FA_READ)== FR_OK) {
		SEGGER_RTT_printf(0, ">Read Ok /Config/lcd_gn.txt\n");

		for (uint8_t i = 0; i < 16; i++) {
			f_gets((char*) str1, sizeof(str1), &SDFile);
			value[i] = atoi(str1);
		}

		sprintf(str2, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				value[0], value[1], value[2], value[3], value[4], value[5],
				value[6], value[7], value[8], value[9], value[10], value[11],
				value[12], value[13], value[14], value[15]);
		BT_Send_String_to_Host(98, str2);
		f_close(&SDFile);
	} else {
		SEGGER_RTT_printf(0, ">Read Error /Config/lcd_gn.txt\n");
	}

	SEGGER_RTT_printf(0, ">LCD_Read_gn %s\n", str2);
}
//
//
//
void LCD_Save_fps_Callback(cmd *c) {

	char str1[32] = { 0 };
	char fps[3] = { 1, 1, 1 };
	Command cmd(c); // Create wrapper object

	int argNum = cmd.countArgs(); // Get number of arguments

	for (int i = 0; i < argNum; i++) {
		Argument arg = cmd.getArg(i);
		String argValue = arg.getValue();
		int argIntValue = StrToInt(argValue);

		fps[i] = argIntValue;
	}

	if (f_open(&SDFile, "/Config/lcd_fps.txt", FA_CREATE_ALWAYS | FA_WRITE)
			== FR_OK) {
		sprintf(str1, "%d\n", fps[0]);
		f_puts(str1, &SDFile);
		sprintf(str1, "%d\n", fps[1]);
		f_puts(str1, &SDFile);
		sprintf(str1, "%d\n", fps[2]);
		f_puts(str1, &SDFile);
		f_close(&SDFile);
		SEGGER_RTT_printf(0, ">Save Ok /Config/lcd_fps.txt\n");

	} else {
		SEGGER_RTT_printf(0, ">Save Error /Config/lcd_fps.txt\n");

	}

	SEGGER_RTT_printf(0, ">LCD_Save_fps %d %d %d\n", fps[0], fps[1], fps[2]);
}

void LCD_Save_gammaset_Callback(cmd *c) {
	UINT testByte;
	char str1[32] = { 0 };
	Command cmd(c); // Create wrapper object

	// Get arguments
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);

	if (f_open(&SDFile, "/Config/lcd_gs.txt", FA_CREATE_ALWAYS | FA_WRITE)
			== FR_OK) {
		sprintf(str1, "%d\n", argIntValue);
		f_write(&SDFile, &str1, strlen(str1), &testByte);
		f_close(&SDFile);
		SEGGER_RTT_printf(0, ">Save Ok /Config/lcd_gs.txt\n");

	} else {
		SEGGER_RTT_printf(0, ">Save Error /Config/lcd_gs.txt\n");

	}

	SEGGER_RTT_printf(0, ">LCD_Save_gammaset %d\n", argIntValue);

}

void LCD_Save_vcom1_Callback(cmd *c) {
	UINT testByte;
	char str1[32] = { 0 };
	Command cmd(c); // Create wrapper object

	// Get arguments
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);

	if (f_open(&SDFile, "/Config/lcd_vcom.txt", FA_CREATE_ALWAYS | FA_WRITE)
			== FR_OK) {
		sprintf(str1, "%d\n", argIntValue);
		f_write(&SDFile, &str1, strlen(str1), &testByte);
		f_close(&SDFile);
		SEGGER_RTT_printf(0, ">Save Ok /Config/lcd_vcom.txt\n");

	} else {
		SEGGER_RTT_printf(0, ">Save Error\n");
	}

	SEGGER_RTT_printf(0, ">LCD_Save_vcom1 %d\n", argIntValue);
}

void LCD_Save_gp_Callback(cmd *c) {

	char str1[32] = { 0 };
	char gp[32] = { 0 };
	Command cmd(c); // Create wrapper object

	int argNum = cmd.countArgs(); // Get number of arguments

	for (int i = 0; i < argNum; i++) {
		Argument arg = cmd.getArg(i);
		String argValue = arg.getValue();
		int argIntValue = StrToInt(argValue);

		gp[i] = argIntValue;
	}

	if (f_open(&SDFile, "/Config/lcd_gp.txt", FA_CREATE_ALWAYS | FA_WRITE)
			== FR_OK) {

		for (uint8_t i = 0; i < 16; i++) {
			sprintf(str1, "%d\n", gp[i]);
			f_puts(str1, &SDFile);
		}
		f_close(&SDFile);
		SEGGER_RTT_printf(0, ">Save Ok /Config/lcd_gp.txt \n");
	} else {
		SEGGER_RTT_printf(0, ">Save Error /Config/lcd_gp.txt\n");
	}
	SEGGER_RTT_printf(0, ">LCD_Save_gp\n");
}

//
//
//
void LCD_Save_gn_Callback(cmd *c) {
	char str1[32] = { 0 };
	char gn[32] = { 0 };
	Command cmd(c); // Create wrapper object

	int argNum = cmd.countArgs(); // Get number of arguments

	for (int i = 0; i < argNum; i++) {
		Argument arg = cmd.getArg(i);
		String argValue = arg.getValue();
		int argIntValue = StrToInt(argValue);

		gn[i] = argIntValue;
	}

	if (f_open(&SDFile, "/Config/lcd_gn.txt", FA_CREATE_ALWAYS | FA_WRITE)
			== FR_OK) {

		for (uint8_t i = 0; i < 16; i++) {
			sprintf(str1, "%d\n", gn[i]);
			f_puts(str1, &SDFile);
		}
		f_close(&SDFile);
		SEGGER_RTT_printf(0, ">Save Ok /Config/lcd_gn.txt \n");
	} else {
		SEGGER_RTT_printf(0, ">Save Error /Config/lcd_gn.txt\n");
	}
	SEGGER_RTT_printf(0, ">LCD_Save_gn\n");
}

void LCD_Read_gp_Callback(cmd *c) {
	int value[32];
	char str1[64] = { 0 };
	char str2[64] = { 0 };

	if (f_open(&SDFile, "/Config/lcd_gp.txt", FA_OPEN_EXISTING | FA_READ)
			== FR_OK) {
		SEGGER_RTT_printf(0, ">Read Ok /Config/lcd_gp.txt\n");

		for (uint8_t i = 0; i < 16; i++) {
			f_gets((char*) str1, sizeof(str1), &SDFile);
			value[i] = atoi(str1);
		}

		sprintf(str2, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				value[0], value[1], value[2], value[3], value[4], value[5],
				value[6], value[7], value[8], value[9], value[10], value[11],
				value[12], value[13], value[14], value[15]);
		BT_Send_String_to_Host(98, str2);
		f_close(&SDFile);
	} else {
		SEGGER_RTT_printf(0, ">Read Error /Config/lcd_gp.txt\n");
	}

	SEGGER_RTT_printf(0, ">LCD_Read_gp %s\n", str2);
}

















///////////////////////////////////////////////////////////////////////////////////////////////////
//Работает

//Команда блокировки обовления
void LCD_Block_Callback(cmd *c) {
	Command cmd(c); // Create wrapper object
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);
	if (argIntValue) {
		tft.blockUpdate = 1;
	} else {
		tft.blockUpdate = 0;
	}
	SEGGER_RTT_printf(0, ">LCD_Block>%d\n", tft.blockUpdate);
}

//Програмный сброс экрана
void LCD_SoftwareReset_Callback(cmd *c) {
	tft.SPI.LCD_Reset();
	SEGGER_RTT_printf(0, ">LCD_SoftwareReset\n");
}

//Инициализация экрана по умолчанию
void LCD_Init_Callback(cmd *c) {
	tft.ST7789_Init();
	tft.ST77XX_Update_MADCTL();
	SEGGER_RTT_printf(0, ">LCD_Init\n");
}

#endif




















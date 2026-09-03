#include "taskCLI.h"
#include "global_define.h"

#ifdef USE_CLI

Command RGB_DEF;  //Text по умолчанию
Command RGB_TS;   //Text Select
Command RGB_TD;   //Текст Disable
Command RGB_REC;  //Установить прямоугольник
Command RGB_BG;   //Установить фон


void RGB_callback(cmd *c) {
	Command cmd(c); // Create wrapper object
	int argNum = cmd.countArgs(); // Get number of arguments
    if (argNum != 4) SEGGER_RTT_WriteString(0,"ERROR>RGB>argNum != 4\n");
	uint8_t RGB[3];

	Argument arg = cmd.getArg(0);
	String argValue = arg.getValue();

	for (int i = 1; i < 4; i++) {
		Argument arg = cmd.getArg(i);
		String Value = arg.getValue();
		int argIntValue = StrToInt(Value);
		RGB[i-1] = (uint8_t)(argIntValue & 0xFF);
	}

	if (argValue.compare("DEF") == 0)
	{
	  palitra_COLOR_TEXT_DEFAULT = tft.RGB565(RGB[0], RGB[1], RGB[2]);
	  SEGGER_RTT_printf(0, ">RGB DEF\n");
	}

	if (argValue.compare("TS") == 0)
	{
	  palitra_COLOR_TEXT_SELECT = tft.RGB565(RGB[0], RGB[1], RGB[2]);
	  SEGGER_RTT_printf(0, ">RGB TS\n");
	}

	if (argValue.compare("TD") == 0)
	{
	  palitra_COLOR_TEXT_DISABLE = tft.RGB565(RGB[0], RGB[1], RGB[2]);
	  SEGGER_RTT_printf(0, ">RGB TD\n");
	}

	if (argValue.compare("REC") == 0)
	{
	  palitra_COLOR_RECTAGLE= tft.RGB565(RGB[0], RGB[1], RGB[2]);
	  SEGGER_RTT_printf(0, ">RGB REC\n");
	}

	if (argValue.compare("BG") == 0)
	{
	  palitra_COLOR_BACKGROUND = tft.RGB565(RGB[0], RGB[1], RGB[2]);
	  menu_generator.ColorBackground = palitra_COLOR_BACKGROUND;
	  menu_setting.ColorBackground = palitra_COLOR_BACKGROUND;
	  SEGGER_RTT_printf(0, ">RGB BG\n");
	}
	tft.needUpdate = 1;
}







#endif

#include "taskCLI.h"
#include "global_define.h"

#ifdef USE_CLI

Command comandCH1_EN;
Command comandCH1_CARRIER_FR;
Command comandCH1_CARRIER_MOD;

Command comandCH1_AM_EN;
Command comandCH1_AM_MOD;
Command comandCH1_AM_FR;

Command comandCH1_FM_EN;
Command comandCH1_FM_BASE;
Command comandCH1_FM_DEV;
Command comandCH1_FM_FR;
Command comandCH1_FM_MOD;

//CH1_EN
void comandCH1_EN_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);
	Gen.CH1.CH_EN = argIntValue;
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.CH_EN %d\n", Gen.CH1.CH_EN);
}

/// --- Частота несущей
void comandCH1_CARRIER_FR_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);
	Gen.CH1.Carrier_fr = argIntValue;
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.Carrier_fr %d\n", Gen.CH1.Carrier_fr);
}
//
// ---
void comandCH1_CARRIER_MOD_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	strcpy(Gen.CH1.Carrier_mod, invertValue.c_str());
	Gen.Create_Carrier(&Gen.CH1);
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.Carrier_mod %s\n", Gen.CH1.Carrier_mod);
}

void comandCH1_AM_EN_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);
	Gen.CH1.AM_EN = argIntValue;
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.AM_EN %d\n", Gen.CH1.AM_EN);
}

void comandCH1_AM_MOD_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	strcpy(Gen.CH1.AM_mod, invertValue.c_str());
	Gen.Create_AM_Modulation(&Gen.CH1);
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.AM_mod %s\n", Gen.CH1.AM_mod);
}

void comandCH1_AM_FR_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	float argIntValue = StrToFloat(invertValue);
	Gen.CH1.AM_fr = argIntValue;
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.AM_fr %f\n", Gen.CH1.AM_fr);

}

void comandCH1_FM_EN_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);
	Gen.CH1.FM_EN = argIntValue;
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.FM_EN %d\n", Gen.CH1.FM_EN);
}

void comandCH1_FM_BASE_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);
	Gen.CH1.FM_Base = argIntValue;
	Gen.Create_FM_Modulation(&Gen.CH1);
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.FM_Base %d\n", Gen.CH1.FM_Base);
}

void comandCH1_FM_DEV_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);
	Gen.CH1.FM_Dev = argIntValue;
	Gen.Create_FM_Modulation(&Gen.CH1);
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.FM_Dev %d\n", Gen.CH1.FM_Dev);
}

void comandCH1_FM_FR_Callback(cmd *c) {
	Command cmd(c);
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	float argIntValue = StrToFloat(invertValue);
	Gen.CH1.FM_mod_fr = argIntValue;
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.FM_mod_fr %f\n", Gen.CH1.FM_mod_fr);
}

void comandCH1_FM_MODCallback(cmd *c) {
	Command cmd(c); // Create wrapper object
	Argument invertArg = cmd.getArgument(0);
	String invertValue = invertArg.getValue();
	strcpy(Gen.CH1.FM_mod, invertValue.c_str());
	Gen.Create_FM_Modulation(&Gen.CH1);
	tft.needUpdate = 1;
	SEGGER_RTT_printf(0, ">sCH1.FM_mod %s\n", Gen.CH1.FM_mod);
}

#endif

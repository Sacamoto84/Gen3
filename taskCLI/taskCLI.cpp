#include "taskCLI.h"
#include "HiSpeedDWT.h"
#include "global_define.h"

#ifdef USE_CLI
void BT_Send_toCLI(char * str);
extern uint8_t CRC8(char *pcBlock, unsigned int len);

void SendToCli(char * str, int len);


extern volatile uint8_t block_uTFTUpdate; //Признак блокировки обновления экрана

extern uint8_t retSD;    /* Return value for SD */
extern char SDPath[4];   /* SD logical drive path */
extern FATFS SDFatFS;    /* File system object for SD logical drive */
extern FIL SDFile;       /* File object for SD */

extern FRESULT fr;

#define Debug_Socket 99 //Сокет для отправки текстовых сообщений на комп

extern char big_buffer[];

uint16_t hex_table[256+4] CCMRAM;

struct cmdBuf {
  uint8_t command;   // ST7735 command byte
  uint8_t delay;     // ms delay after
  uint8_t len;       // length of parameter data
  uint8_t data[16];  // parameter data
};
//
FRESULT scan_files ( char* path );

void SendToCli(char * str, int len);
String readStringUntil(char terminator);

String input;

SimpleCLI cli;

Command MCU_Reset;//Сброс контроллера

Command File_Read_Dir; //Получить список файлов в папке
Command Command_Read_File;

Command Generator_Pause;
Command Generator_Start;

Command Generator_Read_Config; //Чтение текущих настроек

//String to Int
int StrToInt(String s)
{
	char str[16]={0};
	strcpy(str, s.c_str());
	return atoi(str);
}

float StrToFloat(String s)
{
  char str[32]={0};

  for(unsigned int i = 0; i < s.size(); i++)
  {
    if(s[i] == ',')
    {
        s.replace(i, 1, ".");
    }
  }
	
	strcpy(str, s.c_str());
	
	return atof(str);
}

//Конвертирование DOS866 в W1251
char DosToChar(unsigned char Byte)
{
   char Code;
   Code = Byte;
   if (Byte >= 128 && Byte <= 175)
   Code = Byte + 64;
   if (Byte >= 224 && Byte <= 239)
   Code = Byte + 64;
   return Code;
}

// Конвертирование строки из DOS(866) в Win1251
void ConvertStringDosTo1251 ( char *str )
{
	static const uint8_t table[128] = {
			0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,	// 00 - 0F
			0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, // 10 - 1F
			0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, // 20 - 2F
			0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, // 30 - 3F
			0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, // 40 - 4F
			0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, // 50 - 5F
			0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, // 60 - 6F
			0xA8, 0xB8, 0xAA, 0xBA, 0xAF, 0xDF, 0xA1, 0xA2, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF  // 70 - 7F
	};

	unsigned int i;

	for ( i = 0; i < strlen ( str ); i ++ )
		if ( str [ i ] > 127 )
			str [ i ] = table [ (int) (str[i] - 128) ];
} // ConvertStringDosTo1251

// Конвертирование строки из Win1251 в DOS(866)
void ConvertString1251ToDos ( char *str )
{
	static const uint8_t table[128] = {
			0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, // 00 - 0F
			0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, // 10 - 1F
			0xD0, 0xF6, 0xF7, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xF0, 0xD9, 0xF2, 0xDB, 0xDC, 0xDD, 0xDE, 0xF4, // 20 - 2F
			0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF1, 0xF9, 0xF3, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, // 30 - 3F
			0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, // 40 - 4F
			0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0xF5, // 50 - 5F
			0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, // 60 - 6F
			0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF  // 70 - 7F
	};
	unsigned int i;

	for ( i = 0; i < strlen ( str ); i ++ )
		if ( str [ i ] > 127 )
			str [ i ] = table [ (int)(str[i] - 128) ];
} // ConvertStringDosTo1251

//Сформировать и отправить текущие настройки
void sendStructCHtoHost(unsigned int ch)
{

	char buffer[256]  __attribute__((aligned (4)));
	char str_temp[32] __attribute__((aligned (4)));
	uint8_t crc;

	memset(buffer, 0 , 256);
	memset(str_temp, 0 , 32);

	LOG( (char*)"F", 'I',(char*)"sendStructCHtoHost");

	strcat(buffer, (ch) ? "!51#" : "!50#");

	sprintf(str_temp, "%d,", (ch)? Gen.CH2.CH_EN : Gen.CH1.CH_EN);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%d,", (ch)? Gen.CH2.Carrier_fr : Gen.CH1.Carrier_fr);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%s,", (ch)? Gen.CH2.Carrier_mod: Gen.CH1.Carrier_mod);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%d,", (ch)? Gen.CH2.AM_EN : Gen.CH1.AM_EN);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%.1f,", (ch)? Gen.CH2.AM_fr : Gen.CH1.AM_fr);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%s,", (ch)? Gen.CH2.AM_mod : Gen.CH1.AM_mod);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%d,", (ch)? Gen.CH2.FM_EN : Gen.CH1.FM_EN);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%d,", (ch)? Gen.CH2.FM_Base : Gen.CH1.FM_Base);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%d,", (ch)? Gen.CH2.FM_Dev : Gen.CH1.FM_Dev);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%.1f,", (ch)? Gen.CH2.FM_mod_fr : Gen.CH1.FM_mod_fr);
	strcat (buffer,str_temp);

	sprintf(str_temp, "%s", (ch)? Gen.CH2.FM_mod : Gen.CH1.FM_mod);
	strcat (buffer,str_temp);

	crc = CRC8(&buffer[1], strlen(buffer)-1);
	sprintf(str_temp, ";%d$", crc);
	strcat (buffer,str_temp);

	BT_Send_RAW(&buffer[0]);
}

//Получить список файлов в папке
void File_Read_Dir_Callback(cmd* c) {
	
	SEGGER_RTT_printf(0,"CLI>File_Read_Dir Start...\n");

	char str[64];
	char str2[70];
	
    Command cmd(c); // Create wrapper object
	
	Argument invertArg = cmd.getArgument(0);//("invert");
	String invertValue = invertArg.getValue();
	
	strcpy(str, invertValue.c_str());  
	sprintf(str2,"/%s",str);
	
	scan_files(str2); //Сканируем все файлы в папке

	char str_temp[32];
    uint8_t crc;
	
	//Cоздадим паровозик в big_buffer
	//Очиистка буфера
	memset(big_buffer,0,sizeof(SIZE_big_buffer));
	
	//98 сокет
	sprintf(big_buffer, "!98#" ); 
	
	for(uint8_t i=0; i<Dir_File_Info[0].maxFileCount;i++)
    {
	 ConvertStringDosTo1251 ( Dir_File_Info[i].Name);
     SEGGER_RTT_printf(0,"%s\n", Dir_File_Info[i].Name);
	 sprintf(str_temp, "%s,", Dir_File_Info[i].Name);
	 strcat (big_buffer,str_temp);
    }
	
	crc = CRC8(&big_buffer[1], strlen(big_buffer)-1);
	sprintf(str_temp, ";%d$", crc);
	strcat (big_buffer,str_temp);
	
	BT_Send_RAW(&big_buffer[0]);
	
	SEGGER_RTT_printf(0,"%s\n", big_buffer);
    SEGGER_RTT_printf(0,">File_Read_Dir End\n");
}

void comandRead_ConfCallback(cmd* c)
{
	LOG("CLI", 'I', "Read_Conf");
	Command cmd(c); // Create wrapper object

	Argument invertArg = cmd.getArgument(0);//("invert");
	String invertValue = invertArg.getValue();
	int argIntValue = StrToInt(invertValue);

    if (argIntValue == 0) 	sendStructCHtoHost(0);
    if (argIntValue == 1) 	sendStructCHtoHost(1);
    if (argIntValue == 2) 	{sendStructCHtoHost(0); sendStructCHtoHost(1);}

}

//Получить список файлов в папке
void Read_File_Callback(cmd* c) {
	SEGGER_RTT_printf(0,"CLI>Read_File Start...\n");
	SEGGER_SYSVIEW_Error("CLI>Read_File Start...\n");
	char str[64];
	UINT filesize = 0;
    Command cmd(c); // Create wrapper object
	Argument invertArg = cmd.getArgument(0);//("invert");
	String invertValue = invertArg.getValue();
	strcpy(str, invertValue.c_str());
	SEGGER_RTT_printf(0, ">%s\n", str);
	if (f_open(&SDFile, str, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
		filesize = f_size(&SDFile);
		SEGGER_RTT_printf(0, ">Open OK %s %d bytes\n", str, filesize);
		UINT testByte;
		f_read(&SDFile, &Gen.buffer_temp, filesize, &testByte);
		f_close(&SDFile);
	} else
	{
		SEGGER_RTT_printf(0, ">Open Error %s\n", str);
	    return;
	}

	memset(big_buffer,0, SIZE_big_buffer);

	big_buffer[0] = '!';big_buffer[1] = '9';big_buffer[2] = '8';big_buffer[3] = '#';

	char *in;
	in = (char*)&Gen.buffer_temp[0];

	char * p;
	p = (char*)&big_buffer[4];

	for(unsigned int i = 0; i < filesize; i++)
	{

		*p++ = hex_table[*in] >> 8 ;
		*p++ = hex_table[*in++] & 0xFF;

		//sprintf(str, "%02X", *p++);
		//strcat (big_buffer,str);


	}

	SEGGER_SYSVIEW_Warn("6");
	uint8_t crc;
	crc = CRC8((char*)&big_buffer[1], 2048*2+3);
	sprintf(str, ";%d$", crc);
	strcat ((char*)big_buffer,str);
	SEGGER_SYSVIEW_Warn("7");
	SEGGER_SYSVIEW_Error("BT_Send_RAW\n");
	BT_Send_RAW((char*)&big_buffer[0]);
	SEGGER_SYSVIEW_Error("CLI>Read_File End\n");
}

//Сброс контроллера
void MCU_Reset_Callback(cmd* c) {
  SEGGER_RTT_printf(0,">MCU_Reset\n");
	HAL_NVIC_SystemReset();
}

// Callback in case of an error
void errorCallback(cmd_error* e) {
    CommandError cmdError(e); // Create wrapper object

	  String s;
	  char str[64];
  
	  s = cmdError.toString();
	  strcpy(str, s.c_str());
    SEGGER_RTT_printf(0,"CLI->ERROR: %s\n",str);

    //if (cmdError.hasCommand()) {
        //SEGGER_RTT_printf(0,"\nDid you mean \"");
		  	//s = cmdError.getCommand().toString();
				//strcpy(str, s.c_str());
        //SEGGER_RTT_printf(0,str);
        //SEGGER_RTT_printf(0,"\"?");
    //}
}

void Generator_Pause_Callback(cmd* c)
{
	SEGGER_RTT_printf(0,"Generator_Pause_Callback!\n");
	Gen.pause();
}

void Generator_Start_Callback(cmd* c)
{
	SEGGER_RTT_printf(0,"Generator_Start_Callback!\n");
	Gen.start();
}

//////////////////////////////
//Настройка
//////////////////////////////
void taskCLI_setup(void)
{
	SEGGER_RTT_printf(0,"!Start CLI!\n");
	
	cli.setOnError(errorCallback); // Set error Callback

	MCU_Reset  = cli.addSingleArgCmd("MCU_Reset", MCU_Reset_Callback);
	
#ifdef USE_CLI_GEN

	File_Read_Dir = cli.addSingleArgCmd("File_Read_Dir", File_Read_Dir_Callback); //Получить список файлов в папке
	Command_Read_File = cli.addSingleArgCmd("Read_File", Read_File_Callback);

	Generator_Pause = cli.addSingleArgCmd("Generator_Pause", Generator_Pause_Callback);
	Generator_Start = cli.addSingleArgCmd("Generator_Start", Generator_Start_Callback);


	comandCH1_EN          = cli.addSingleArgCmd("CH1_EN",          comandCH1_EN_Callback);
	comandCH1_CARRIER_FR  = cli.addSingleArgCmd("CH1_CARRIER_FR",  comandCH1_CARRIER_FR_Callback);
	comandCH1_CARRIER_MOD = cli.addSingleArgCmd("CH1_CARRIER_MOD", comandCH1_CARRIER_MOD_Callback);

	comandCH1_AM_EN       = cli.addSingleArgCmd("CH1_AM_EN",   comandCH1_AM_EN_Callback);
	comandCH1_AM_MOD      = cli.addSingleArgCmd("CH1_AM_MOD",  comandCH1_AM_MOD_Callback);
	comandCH1_AM_FR       = cli.addSingleArgCmd("CH1_AM_FR",   comandCH1_AM_FR_Callback);

	comandCH1_FM_EN       = cli.addSingleArgCmd("CH1_FM_EN",   comandCH1_FM_EN_Callback);
	comandCH1_FM_BASE     = cli.addSingleArgCmd("CH1_FM_BASE", comandCH1_FM_BASE_Callback);
	comandCH1_FM_DEV      = cli.addSingleArgCmd("CH1_FM_DEV",  comandCH1_FM_DEV_Callback);
	comandCH1_FM_FR       = cli.addSingleArgCmd("CH1_FM_FR",   comandCH1_FM_FR_Callback);
	comandCH1_FM_MOD      = cli.addSingleArgCmd("CH1_FM_MOD",  comandCH1_FM_MODCallback);

	Generator_Read_Config = cli.addSingleArgCmd("Read_Conf",  comandRead_ConfCallback);

#endif





#ifdef USE_CLI_LCD

	LCD_Invers = cli.addSingleArgCmd("LCD_Invers", LCD_Invers_Callback);

	LCD_Gamma_P = cli.addBoundlessCommand("LCD_Gamma_P", LCD_Gamma_P_Callback);

	LCD_Gamma_N = cli.addBoundlessCommand("LCD_Gamma_N", LCD_Gamma_N_Callback);
	//ConfigLCD.addArgument("invert", "0");

	LCD_Block = cli.addSingleArgCmd("LCD_Block", LCD_Block_Callback);


	LCD_Gamma_BMP = cli.addSingleArgCmd("LCD_Gamma_BMP", LCD_Gamma_BMP_Callback);

	LCD_SoftwareReset  = cli.addSingleArgCmd("LCD_SoftwareReset", LCD_SoftwareReset_Callback); //Програмный сброс
	LCD_GAMSET         = cli.addSingleArgCmd("LCD_GAMSET", LCD_GAMSET_Callback);  //GAMSET (Gamma Set) 26H
	LCD_Frame_Rate_Control = cli.addBoundlessCommand("LCD_Frame_Rate_Control", LCD_Frame_Rate_Control_Callback);
	LCD_VMCTR1 = cli.addSingleArgCmd("LCD_VMCTR1", LCD_VMCTR1_Callback);
	LCD_Init   = cli.addSingleArgCmd("LCD_Init", LCD_Init_Callback);
	MCU_Reset  = cli.addSingleArgCmd("MCU_Reset", MCU_Reset_Callback);


	LCD_Save_backlight = cli.addSingleArgCmd("LCD_Save_backlight", LCD_Save_backlight_Callback);   //Работает
	LCD_Save_fps       = cli.addBoundlessCommand("LCD_Save_fps", LCD_Save_fps_Callback);
	LCD_Save_gammaset  = cli.addSingleArgCmd("LCD_Save_gammaset", LCD_Save_gammaset_Callback);
	LCD_Save_vcom1     = cli.addSingleArgCmd("LCD_Save_vcom1", LCD_Save_vcom1_Callback);
	LCD_Save_gp        = cli.addBoundlessCommand("LCD_Save_gp", LCD_Save_gp_Callback);
	LCD_Save_gn        = cli.addBoundlessCommand("LCD_Save_gn", LCD_Save_gn_Callback);


	LCD_Read_backlight = cli.addSingleArgCmd("LCD_Read_backlight", LCD_Read_backlight_Callback); //Работает
	LCD_Read_vcom1     = cli.addSingleArgCmd("LCD_Read_vcom1", LCD_Read_vcom1_Callback);
	LCD_Read_fps       = cli.addSingleArgCmd("LCD_Read_fps", LCD_Read_fps_Callback);
	LCD_Read_gammaset  = cli.addSingleArgCmd("LCD_Read_gammaset", LCD_Read_gammaset_Callback);
    LCD_Read_gp        = cli.addSingleArgCmd("LCD_Read_gp", LCD_Read_gp_Callback);
	LCD_Read_gn        = cli.addSingleArgCmd("LCD_Read_gn", LCD_Read_gn_Callback);

	LCD_Init_Custom = cli.addSingleArgCmd("LCD_Init_Custom", LCD_Init_Custom_Callback);
#endif



	RGB_DEF = cli.addBoundlessCommand("RGB", RGB_callback);  //Text по умолчанию
	//RGB_TS;   //Text Select
	//RGB_TD;   //Текст Disable
	//RGB_REC;  //Установить прямоугольник
	//RGB_BG;   //Установить фон


	char *p;
	char str[16];
	p = (char *)&hex_table[0];
    for(uint i = 0; i<256;i++)
    {
      sprintf(str, "%02X", i);
      *p++=str[1];
      *p++=str[0];
    }
    p=NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
String readStringUntil(char terminator)
{
  String ret;
  int c = SEGGER_RTT_GetKey();
  while (c >= 0 && c != terminator) {
    ret += (char)c;   
		c = SEGGER_RTT_WaitKey();
  }
  return ret;
}

//Функция для Си чтобы вставить ее в parser
void SendToCli(char * str, int len)
{
	String ret;
	char * p;
	p = str;
	for (int i=0;i<len; i++)ret += *p++;
	SEGGER_SYSVIEW_Print("cli.parse");
	cli.parse(ret);
}


FRESULT scan_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
	char str1[64];
    static FRESULT res;
    DIR dir;
    static FILINFO fno;
    Dir_File_Info[0].maxFileCount = 0;

    res = f_opendir(&dir, path);                       /* Open the directory */

    if (res == FR_OK) {
        for (;;) {
					res = f_readdir(&dir, &fno);                   /* Read a directory item */
					if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */                               
					    sprintf(str1,"%s", fno.fname);										
						sprintf(Dir_File_Info[Dir_File_Info[0].maxFileCount++].Name,"%s", str1);
        }
        f_closedir(&dir);
    }
    
    return res;
}

#endif



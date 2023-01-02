#include <TFT_color.h>
#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "FontSmooth.h"
#include "menu_typedef.h"
#include "global_typedef.h"
#include "stdio.h"

#include "generator.h"
extern GENERATOR Gen;

extern void PAGE_Menu(menu_typedef * menu , item_typedef * item, int NUM);

//static int temp;
static item_typedef   * temp_item;
static menu_typedef   * temp_menu;

static char * convert_item_modulation(char * s)
{
	static char res_str[20];
	memset(res_str, 0, 20);
	int len = strlen(s);
	if (len > 0)
	  memcpy(res_str, &s[0], len-4);

	return &res_str[0];
}

//Событие по isClick в списке выбора названия несущей
void PageSelectModulation_onClick(void)
{
	int index = temp_menu->index; //Получем индекс в списке
	//char filename[32]={0};
	//sprintf(filename,"%s", temp_item[index].text); //Получаем название несущей по индексу

	uint32_t i = menu_generator.index;

	if (i == INDEX_CH1_CR) //Carrier
	{
		sprintf(Gen.CH1.Carrier_mod,"%s", temp_item[index].text);
        Gen.Create_Carrier(&Gen.CH1);
        temp_item[index].field.exit = 1; //Для выхода
        Font_Smooth_Load(menu_generator.font);
	}

	if (i == INDEX_CH1_AM_MOD) //AM mod
	{
		sprintf(Gen.CH1.AM_mod,"%s", temp_item[index].text);
        Gen.Create_AM_Modulation(&Gen.CH1);
        temp_item[index].field.exit = 1; //Для выхода
        Font_Smooth_Load(menu_generator.font);
	}

	if (i == INDEX_CH1_FM_MOD) //AM mod
	{
		sprintf(Gen.CH1.FM_mod,"%s", temp_item[index].text);
        Gen.Create_FM_Modulation(&Gen.CH1);
        temp_item[index].field.exit = 1; //Для выхода
        Font_Smooth_Load(menu_generator.font);
	}

	if (i == INDEX_CH2_CR) //Carrier
	{
		sprintf(Gen.CH2.Carrier_mod,"%s", temp_item[index].text);
        Gen.Create_Carrier(&Gen.CH2);
        temp_item[index].field.exit = 1; //Для выхода
        Font_Smooth_Load(menu_generator.font);
	}

	if (i == INDEX_CH2_AM_MOD) //AM mod
	{
		sprintf(Gen.CH2.AM_mod,"%s", temp_item[index].text);
        Gen.Create_AM_Modulation(&Gen.CH2);
        temp_item[index].field.exit = 1; //Для выхода
        Font_Smooth_Load(menu_generator.font);
	}

	if (i == INDEX_CH2_FM_MOD) //AM mod
	{
		sprintf(Gen.CH2.FM_mod,"%s", temp_item[index].text);
        Gen.Create_FM_Modulation(&Gen.CH2);
        temp_item[index].field.exit = 1; //Для выхода
        Font_Smooth_Load(menu_generator.font);
	}
}

//Пре рисуем по index
void postPageSelectModulation(void){
	tft.RectangleFilled(0, 0, 239, 119, BLACK);
    int index = temp_menu->index; //Текущий выбранный индекс
    char filename[32]={0};
    UINT testByte;

    uint32_t i = menu_generator.index;

   if (index)
   {

    //Получаем имя файла для отображения на экране
	if ((i==INDEX_CH1_CR)||(i==INDEX_CH2_CR))
		sprintf(filename,"/Carrier/%s", temp_item[index].text);
	else
		sprintf(filename,"/Mod/%s", temp_item[index].text);

	//Открывам файл
	if (f_open(&SDFile, filename, FA_OPEN_EXISTING | FA_READ) == FR_OK) {
			rtt.print(">Open OK %s\n", filename);
			f_read(&SDFile, &Gen.buffer_temp, 2048, &testByte);
			f_close(&SDFile);
		} else
		{
			rtt.print(">Open Error %s\n", filename);
			return;
		}

		uint16_t *p;
	    p = (uint16_t*)&Gen.buffer_temp[0];
	    Gen.buffer_temp[1024] = Gen.buffer_temp[1023];
	    Gen.buffer_temp[1025] = Gen.buffer_temp[1023];

	    //Carrier
	   if ((i == INDEX_CH1_CR)||(i == INDEX_CH2_CR))
	   {
	    tft.LineHW(0, 59, 239, BLUE);

	    tft.LineMoveXY(17, (4095-*p)/40 + 8);

		for (int i = 0; i < 1024; i+=5){
			tft.LineMoveTo(17+i/10, (4095-*p)/40 + 8, GREEN);
		    p+=5;
		}
		p = (uint16_t*)&Gen.buffer_temp[0];
	    //tft.LineMoveXY(17, *p/40 + 8);
		for (int i = 0; i < 1024; i+=5){
			tft.LineMoveTo(119+i/10, (4095-*p)/40 + 8, GREEN);
		    p+=5;
		}
	   }

	    //AM Mod
	   if ((i==INDEX_CH1_AM_MOD)||(i==INDEX_CH1_FM_MOD)||(i==INDEX_CH2_AM_MOD)||(i==INDEX_CH2_FM_MOD))
	   {
	    tft.LineHW(0, 59, 239, BLUE);
	    tft.LineMoveXY(17, *p/40 + 8);
		for (int i = 0; i < 1024; i+=5){
			tft.LineV(17+i/5, (4095-*p)/80 + 8, (*p)/80 + 59, GREEN);
		    p+=5;
		}
	   }

    } //if (index)

   tft.driver.needUpdate = 1;
}


//Создаем списки и по ним рендерим графику и рисуем списки
void PAGE_generator_select_modulation(void)
{
	uint32_t i = menu_generator.index;

	//Поиск файлов в папке
	char * path;

	if ((i==INDEX_CH1_CR)||(i==INDEX_CH2_CR))
	  path = (char*)"/Carrier";
	else
	  path = (char*)"/Mod";

	//temp = i; //Определяет тип модуляции несущая или модуляция

	Dir_File_Info_Array Dir_File_Info[32]; //Массив всех файлов в папке

    static FRESULT res;
    DIR dir;
    FILINFO fno;
    Dir_File_Info[0].maxFileCount = 0;



    res = f_opendir(&dir, path);                       /* Open the directory */

    if (res == FR_OK) {
        for (;;) {
					res = f_readdir(&dir, &fno);                   /* Read a directory item */
					if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
					sprintf(Dir_File_Info[Dir_File_Info[0].maxFileCount++].Name,"%s", fno.fname);
        }
        f_closedir(&dir);
    }

    item_typedef    item[Dir_File_Info[0].maxFileCount+1] = {0};
    menu_typedef    menu = {0};

    sprintf(item[0].text, "%s","<-Назад");
    item[0].field.exit = 1; //Для выхода

    for(int i = 1; i<Dir_File_Info[0].maxFileCount+1;i++){
    	sprintf(item[i].text, "%s",Dir_File_Info[i-1].Name);
    	item[i].callBackFunc_isClick = &PageSelectModulation_onClick;
    }


	//Описание как рендерить меню
    menu.item_count   = 4;
    menu.item_height  = 30;
    menu.item_start_y = 120;
    menu.font = Roboto_Medium_en_ru_18; //&_acRoboto_Medium_en_ru_18[0];
    menu.field.encoder_block = 0;
    menu.postCallBackFunc = NULL;
    menu.preCallBackFunc  = NULL;
    menu.temp = 0;
    //menu.title = NULL;
    menu.postCallBackFunc = &postPageSelectModulation; //Рисовалка
    menu.count_item = Dir_File_Info[0].maxFileCount+1;
    menu.ColorBackground = RGB565(8, 8, 8);

    menu.field.verticalScroll = 1;


    temp_item = &item[0];
    temp_menu = &menu;

    menu.items = item;

    PAGE_Menu( &menu,  &item[0], Dir_File_Info[0].maxFileCount+1);

    Font_Smooth_Load(menu_generator.font);

}


//Пред функция для картинки генератор, обрабатывем крутилки
void prePageGenerator(void)
{
	if (menu_generator.field.encoder_block){

	}
	else
	{
		//187 us
		sprintf(item_generator[INDEX_CH1_CR].text," %s", convert_item_modulation(Gen.CH1.Carrier_mod));
		sprintf(item_generator[INDEX_CH1_AM_MOD].text," %s", convert_item_modulation(Gen.CH1.AM_mod));
		sprintf(item_generator[INDEX_CH1_FM_MOD].text," %s", convert_item_modulation(Gen.CH1.FM_mod));
		sprintf(item_generator[INDEX_CH2_CR].text," %s", convert_item_modulation(Gen.CH2.Carrier_mod));
		sprintf(item_generator[INDEX_CH2_AM_MOD].text," %s", convert_item_modulation(Gen.CH2.AM_mod));
		sprintf(item_generator[INDEX_CH2_FM_MOD].text," %s", convert_item_modulation(Gen.CH2.FM_mod));
	}
}












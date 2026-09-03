
#ifndef INC_MENU_TYPEDEF_H_
#define INC_MENU_TYPEDEF_H_

#include "TFT.h"
#include "bitmap.h"
#include "resource.h"
#include "FontSmooth.h"

#include "Gif.h"

extern TFT tft CCMRAM;

#include "generator.h"
extern GENERATOR Gen;

//───────────────────────────────────────────────┬───────────────────────────┐
 typedef struct                                //│ Item                      │
 {                                             //├───────────────────────────┤
   char text [20];        					   //│ Текст                     │
                                 	 	 	   //│                           │
   void (*callBackFunc_isClick)  (void); 	   //│ Обработчик нажатий        │
   void (*callBackFunc_isHolded) (void); 	   //│                           │
   void (*callBackFunc_isDouble) (void); 	   //│                           │
   void (*preCallBackFunc)(uint32_t);  		   //│ Формирование строк, всегда вызывается перед рисованием строки
   	   	   	   	   	   	   	   	   	   	   	   //│                           │
   //──── gif ───────────────────────┬───────────┴───────────────────────────┤
   char *     nameGif  = 0;        //│ Имя гифки                             │  ┌ В двустроковом варианте gif_init_state
   Gif *  gif  = 0;                //│ Указатель на гифку                    │  │ если 1 то отображаем resid_first
   int8_t  resid_first =-1;        //│ Номер картинки ресурса первый кадр    │<-┤ если 0 то отображаем resid_last
   int8_t  resid_last  =-1;        //│ Номер картинки ресурса последний кадр │<-┤
   ANIMATION_TRIGGERS gif_trigger; //│ Тип анимации                          │  │  item_setting[1].nameGif = (char*)"save";
   uint8_t  gif_x;                 //│ Координата X гифки                    │  │  item_setting[1].gif_trigger = ONCE;
   uint8_t* gif_init_state;        //│ Источник данных                       │<-┘  item_setting[1].gif_x = 170;
   //────────────────────────────────┴───────────────────────────────────────┤
                                                                           //│
	struct fieldbite {                                                     //│
	   unsigned  bitmap_always_on : 1 ;                                    //│
	   unsigned  exit :1;                      //│ If 1 то по Click Выход  //│
	   unsigned  disable:1;                    //│ Если 1 то отключен      //│
	} field;                                                               //│
	                                                                       //│
   int32_t text_color = -1;                    //│ Цвет текста             //│
                                                                           //│
 } item_typedef;                                                           //│
//───────────────────────────────────────────────┴───────────────────────────┘

 typedef struct                                                                   //│
 {                                                                                //│
   //Блок инициализации                                                             │
   uint8_t item_count;    //Количество отображаемых строк                           │
   uint8_t item_height;   //Высота одной строки                                     │
   uint8_t item_start_y;  //Координата Y начала списка                              │
   uint8_t count_item;    //Максимальное количество элементов                       │
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  //│
   int8_t  item_text_delta_x;  //Смещение текста по X                               │
   int8_t  item_text_delta_y;  //Смещение текста по Y                               │
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  //│
   const unsigned char * font; //Используеммый шрифт                                │
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  //│
   //Callback                                                                       │
   void (*preCallBackFunc)  (void);												  //│
   void (*postCallBackFunc) (void);												  //│
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  //│
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  //│
   item_typedef * items;														  //│
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  //│
   uint32_t temp;																  //│
   uint32_t tempf;																  //│
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  //│
   struct fieldbite																  //│
   {																			  //│
		unsigned verticalScroll :1; //1-использовать вертикальный скролл            │
		unsigned encoder_block :1;  //Блокировка энкодера                           │
		unsigned needUpdate :1;     //Требование полной перерисовки кадра           │
		unsigned needRender :1;                                                   //│
   } field;																		  //│
                                                                                  //│
   uint8_t index;																  //│
   uint8_t window_start;														  //│
   uint8_t window_end;															  //│
   uint8_t max_item;															  //│
                                                                                  //│
   uint8_t ii = 0;																  //│
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  //│
   uint16_t ColorBackground; //Цвет фона										    │

   void run(uint8_t i)
   {
	   //Выполняем пре для этого елемента
	   //Создание текста
	   if (items[i].preCallBackFunc) {
		   void (*fcnPtr)(uint32_t) = items[i].preCallBackFunc;
		   fcnPtr(index);
		}

	   if (items[i].field.disable == 0)
	   {
		   //1500us
		   if (items[i].text_color != -1)
			   Font_Smooth_drawStr(&tft, 9 - 1 + item_text_delta_x, 8 + item_height * (ii % item_count) - 1 + item_start_y + item_text_delta_y, items[i].text, (uint16_t) items[i].text_color);
		   else
			   Font_Smooth_drawStr(&tft, 9 - 1 + item_text_delta_x, 8 + item_height * (ii % item_count) - 1 + item_start_y + item_text_delta_y, items[i].text, (i == index) ? COLOR_TEXT_SELECT : COLOR_TEXT_DEFAULT);
	   }
	   else
		   Font_Smooth_drawStr(&tft, 9 - 1 + item_text_delta_x, 8 + item_height * (ii % item_count) - 1 + item_start_y + item_text_delta_y, items[i].text, COLOR_TEXT_DISABLE); //COLOR_TEXT_DISABLE

   }

   //Обновить биты Disable
   void refreshDisable()
   {
	       //┌────────────────────────────────────────────────────┐
	   	   //│ Делаем блокировки                                  │
	   	   //└────────────────────────────────────────────────────┤
	   	   if (Gen.CH1.CH_EN)                                   //│
	   	   {                                                    //│
	   		   items[INDEX_CH1_CR].field.disable      = 0;      //│
	   		   items[INDEX_CH1_AM_EN].field.disable   = 0;      //│
	   		   if (Gen.CH1.AM_EN)                               //│
	   		   {                                                //│
	   		     items[INDEX_CH1_AM_MOD].field.disable  = 0;    //│
	   		     items[INDEX_CH1_AM_FR].field.disable   = 0;    //│
	   		   }                                                //│
	   		   else                                             //│
	   		   {                                                //│
	   			 items[INDEX_CH1_AM_MOD].field.disable  = 1;    //│
	   			 items[INDEX_CH1_AM_FR].field.disable   = 1;    //│
	   		   }                                                //│
	   		   items[INDEX_CH1_FM_EN].field.disable   = 0;      //│
	   		   if (Gen.CH1.FM_EN)                               //│
	   	        {                                               //│
	   			   items[INDEX_CH1_FM_BASE].field.disable = 0;  //│
	   			   items[INDEX_CH1_FM_DEV].field.disable  = 0;  //│
	   			   items[INDEX_CH1_FM_MOD].field.disable  = 0;  //│
	   			   items[INDEX_CH1_FM_FR].field.disable   = 0;  //│
	   			   items[INDEX_CH1_FR].field.disable  = 1;      //│
	   			}                                               //│
	   		   else                                             //│
	   		   {                                                //│
	   			   items[INDEX_CH1_FM_BASE].field.disable = 1;  //│
	   			   items[INDEX_CH1_FM_DEV].field.disable  = 1;  //│
	   			   items[INDEX_CH1_FM_MOD].field.disable  = 1;  //│
	   			   items[INDEX_CH1_FM_FR].field.disable   = 1;  //│
	   			   items[INDEX_CH1_FR].field.disable      = 0;  //│
	   		   }                                                //│
	   	   }                                                    //│
	   	   else                                                 //│
	   	   {                                                    //│
	   		   items[INDEX_CH1_FR].field.disable      = 1;      //│
	   		   items[INDEX_CH1_CR].field.disable      = 1;      //│
	   		   items[INDEX_CH1_AM_EN].field.disable   = 1;      //│
	   		   items[INDEX_CH1_AM_MOD].field.disable  = 1;      //│
	   		   items[INDEX_CH1_AM_FR].field.disable   = 1;      //│
	   		   items[INDEX_CH1_FM_EN].field.disable   = 1;      //│
	   		   items[INDEX_CH1_FM_BASE].field.disable = 1;      //│
	   		   items[INDEX_CH1_FM_DEV].field.disable  = 1;      //│
	   		   items[INDEX_CH1_FM_MOD].field.disable  = 1;      //│
	   		   items[INDEX_CH1_FM_FR].field.disable   = 1;      //│
	   	   }                                                    //│
	   	                                                        //│
	   	   if (Gen.CH2.CH_EN)                                   //│
	   	   {                                                    //│
	   		   items[INDEX_CH2_CR].field.disable      = 0;      //│
	   		   items[INDEX_CH2_AM_EN].field.disable   = 0;      //│
	   		   if (Gen.CH2.AM_EN)                               //│
	   		   {                                                //│
	   		     items[INDEX_CH2_AM_MOD].field.disable  = 0;    //│
	   		     items[INDEX_CH2_AM_FR].field.disable   = 0;    //│
	   		   }                                                //│
	   		   else                                             //│
	   		   {                                                //│
	   			 items[INDEX_CH2_AM_MOD].field.disable  = 1;    //│
	   			 items[INDEX_CH2_AM_FR].field.disable   = 1;    //│
	   		   }                                                //│
	   		   items[INDEX_CH2_FM_EN].field.disable   = 0;      //│
	   		   if (Gen.CH2.FM_EN)                               //│
	   	        {                                               //│
	   			   items[INDEX_CH2_FM_BASE].field.disable = 0;  //│
	   			   items[INDEX_CH2_FM_DEV].field.disable  = 0;  //│
	   			   items[INDEX_CH2_FM_MOD].field.disable  = 0;  //│
	   			   items[INDEX_CH2_FM_FR].field.disable   = 0;  //│
	   			   items[INDEX_CH2_FR].field.disable      = 1;  //│
	   			}                                               //│
	   		   else                                             //│
	   		   {                                                //│
	   			   items[INDEX_CH2_FM_BASE].field.disable = 1;  //│
	   			   items[INDEX_CH2_FM_DEV].field.disable  = 1;  //│
	   			   items[INDEX_CH2_FM_MOD].field.disable  = 1;  //│
	   			   items[INDEX_CH2_FM_FR].field.disable   = 1;  //│
	   			   items[INDEX_CH2_FR].field.disable      = 0;  //│
	   		   }                                                //│
	   	   }                                                    //│
	   	   else                                                 //│
	   	   {                                                    //│
	   		   items[INDEX_CH2_FR].field.disable      = 1;      //│
	   		   items[INDEX_CH2_CR].field.disable      = 1;      //│
	   		   items[INDEX_CH2_AM_EN].field.disable   = 1;      //│
	   		   items[INDEX_CH2_AM_MOD].field.disable  = 1;      //│
	   		   items[INDEX_CH2_AM_FR].field.disable   = 1;      //│
	   		   items[INDEX_CH2_FM_EN].field.disable   = 1;      //│
	   		   items[INDEX_CH2_FM_BASE].field.disable = 1;      //│
	   		   items[INDEX_CH2_FM_DEV].field.disable  = 1;      //│
	   		   items[INDEX_CH2_FM_MOD].field.disable  = 1;      //│
	   		   items[INDEX_CH2_FM_FR].field.disable   = 1;      //│
	   	   }                                                    //│
	   	   //─────────────────────────────────────────────────────┘
   }

   //┌───────────────────────────────────────────────────┐
   //│ Рендер в две колонки                              │
   //└───────────────────────────────────────────────────┤
   void run2(uint8_t i)
   {
	   //┌─────────────────────────────────────────────────────────────────────────────────────────────────────┐
	   //│ Рисуем выбранную строку                                                33us -Of GenOn На выделенном │
	   //└─────────────────────────────────────────────────────────────────────────────────────────────────────┤
	   if (i == index)                                                                                       //│
	   {                                                                                                     //│
	      tft.RectangleFilled( (i < ((max_item+1)/2)) ? 0 : 119,                                             //│
	    			item_start_y + item_height * (i % ((max_item+1)/2)), 119, item_height, COLOR_RECTAGLE); //│
	      tft.uTFT.GetColor = 1;                                                                             //│
	   }                                                                                                     //│
	   else                                                                                                  //│
	      tft.uTFT.GetColor = 0;                                                                             //│
	   //──────────────────────────────────────────────────────────────────────────────────────────────────────┘

	   //Выполняем пре для этого елемента
	   //Создание текста
	   if (items[i].preCallBackFunc) {
		   void (*fcnPtr)(uint32_t) = items[i].preCallBackFunc;
		   fcnPtr(index);
		}
	   field.needRender = field.encoder_block;

	   //┌────────────────────────────────────────────────────┐
	   //│ Рисуем текст                                       │
	   //└────────────────────────────────────────────────────┤
	   if (items[i].field.disable == 0)
	   {
		   //1500us
		   if (items[i].text_color != -1)
			   Font_Smooth_drawStr(&tft, (i < ((max_item+1)/2)) ? 0 : 119  + item_text_delta_x, 8 + item_height * (i % item_count) - 1 + item_start_y + item_text_delta_y, items[i].text, (uint16_t) items[i].text_color);
		   else
			   Font_Smooth_drawStr(&tft, (i < ((max_item+1)/2)) ? 0 : 119  + item_text_delta_x, 8 + item_height * (i % item_count) - 1 + item_start_y + item_text_delta_y, items[i].text, (i == index) ? COLOR_TEXT_SELECT : COLOR_TEXT_DEFAULT);
	   }
	   else
		   Font_Smooth_drawStr(&tft, (i < ((max_item+1)/2)) ? 0 : 119  + item_text_delta_x, 8 + item_height * (i % item_count) - 1 + item_start_y + item_text_delta_y, items[i].text, COLOR_TEXT_DISABLE);
	   //─────────────────────────────────────────────────────┘

	   //┌────────────────────────────────────────────────────┐
	   //│ Рисуем Bitmap                                      │
	   //└────────────────────────────────────────────────────┤
	    static Bitmap bmp = {0};

	    if (items[i].gif_init_state) //Если есть источник данных
	    {
	    	uint8_t* p;
			p = items[i].gif_init_state;

	    	if (*p)  //Если 1
	    	{
	    		if (items[i].resid_first) //Если id не 0
	    		{
                   bmp = getResBitmapID(items[i].resid_first);

                   switch (bmp.bit)
                   {
                   	  case 32: BitmapFromFlash32b(&tft,(i < ((max_item+1)/2)) ? 0 : 119, item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);	break;
                   	  case 24: BitmapFromFlash24b (&tft,(i < ((max_item+1)/2)) ? 0 : 119, item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);	break;
                      case 16: BitmapFromFlash16b (&tft,(i < ((max_item+1)/2)) ? 0 : 119, item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);	break;
                   }
	    		}
	    	}
	    	else
	    	{
	    		if (items[i].resid_last) //Если id не 0
	    		{
	    			bmp = getResBitmapID(items[i].resid_last);

	    			switch (bmp.bit)
	                {
	                   case 32: BitmapFromFlash32b(&tft, (i < ((max_item+1)/2)) ? 0 : 119, item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);	break;
	                   case 24: BitmapFromFlash24b(&tft, (i < ((max_item+1)/2)) ? 0 : 119, item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);	break;
	                   case 16: BitmapFromFlash16b(&tft, (i < ((max_item+1)/2)) ? 0 : 119, item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);	break;
	                }

	    		}
	    	}
	    }

        //Рисуем Disable картинки с учетом того были они ON или OFF 16бит
	    if   ( (i == INDEX_CH1_AM_EN) && (Gen.CH1.CH_EN == 0))
	    {
	    	if (Gen.CH1.AM_EN)
	    	{
	    	  bmp = getResBitmapID(id_ON_DISABLE);
              BitmapFromFlash16b(&tft, (i < ((max_item+1)/2)) ? 0 : 119,
            		  item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);
	    	}
	    	else
	    	{
	    	  bmp = getResBitmapID(id_OFF_DISABLE);
              BitmapFromFlash16b(&tft, (i < ((max_item+1)/2)) ? 0 : 119,
            		  item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);
	    	}
	    }

	    if   ( (i == INDEX_CH1_FM_EN) && (Gen.CH1.CH_EN == 0))
	    {
	    	if (Gen.CH1.FM_EN)
	    	{
	    	  bmp = getResBitmapID(id_ON_DISABLE);
              BitmapFromFlash16b(&tft, (i < ((max_item+1)/2)) ? 0 : 119,
            		  item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);
	    	}
	    	else
	    	{
	    	  bmp = getResBitmapID(id_OFF_DISABLE);
              BitmapFromFlash16b(&tft, (i < ((max_item+1)/2)) ? 0 : 119,
            		  item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);
	    	}
	    }

	    if   ( (i == INDEX_CH2_AM_EN) && (Gen.CH2.CH_EN == 0))
	    {
	    	if (Gen.CH2.AM_EN)
	    	{
	    	  bmp = getResBitmapID(id_ON_DISABLE);
              BitmapFromFlash16b(&tft,(i < ((max_item+1)/2)) ? 0 : 119,
            		  item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);
	    	}
	    	else
	    	{
	    	  bmp = getResBitmapID(id_OFF_DISABLE);
              BitmapFromFlash16b(&tft, (i < ((max_item+1)/2)) ? 0 : 119,
            		  item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);
	    	}
	    }

	    if   ( (i == INDEX_CH2_FM_EN) && (Gen.CH2.CH_EN == 0))
	    {
	    	if (Gen.CH2.FM_EN)
	    	{
	    	  bmp = getResBitmapID(id_ON_DISABLE);
              BitmapFromFlash16b(&tft, (i < ((max_item+1)/2)) ? 0 : 119,
            		  item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);
	    	}
	    	else
	    	{
	    	  bmp = getResBitmapID(id_OFF_DISABLE);
              BitmapFromFlash16b(&tft, (i < ((max_item+1)/2)) ? 0 : 119,
            		  item_start_y + item_height * (i % ((max_item+1)/2)), &bmp);
	    	}
	    }

   }

 } menu_typedef;













 extern menu_typedef menu_generator;
 extern menu_typedef menu_setting;

 extern item_typedef item_setting[]   CCMRAM;
 extern item_typedef item_generator[] CCMRAM;




#endif /* INC_MENU_TYPEDEF_H_ */

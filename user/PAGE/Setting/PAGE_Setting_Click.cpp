#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "define.h"


void setting_save_click(void)
{
  item_setting[INDEX_SETTING_SAVE].gif->command(PLAY); //Команда запуска анимации
}

void setting_script_stop(void)
{
  script.stop();
}


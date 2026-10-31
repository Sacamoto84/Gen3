#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "define.h"
#include "menu_typedef.h"
#include "scripting.h"

extern Scripting script;
extern item_typedef item_setting[];

void setting_save_click(void)
{
  sprintf(item_setting[INDEX_SETTING_SAVE].text, "%s", Gen.Config_Save() ? "Сохранено" : "Ошибка SD");
}

void setting_script_stop(void)
{
  script.stop();
}


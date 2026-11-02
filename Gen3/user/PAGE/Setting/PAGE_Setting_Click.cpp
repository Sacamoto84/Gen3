#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "define.h"
#include "menu_typedef.h"
#include "scripting.h"
#include "PAGE_Battery.h"

extern Scripting script;
extern item_typedef item_setting[];

void setting_save_click(void)
{
  bool ok = Gen.Config_Save() && Battery_Save();
  sprintf(item_setting[INDEX_SETTING_SAVE].text, "%s", ok ? "Сохранено" : "Ошибка SD");
}

void setting_script_stop(void)
{
  script.stop();
}

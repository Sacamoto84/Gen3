#include "Page.h"
#include "global_define.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "adc.h"

#include "menu_typedef.h"
#include "global_typedef.h"
#include "PAGE_Battery.h"
#include "fatfs.h"
#include "generator.h"

extern Encoder_typedef Encoder;
extern menu_typedef menu_setting;
extern item_typedef item_setting[];
extern void PAGE_Menu(menu_typedef * menu , item_typedef * item, int NUM);
extern void update_setting_voltage(void);

//┌─────────────────────────────────────────┐
	#define INDEX_BATT_BACK   0         //│ Назад
	#define INDEX_BATT_VIN    1         //│ U вх
	#define INDEX_BATT_VCELL  2         //│ U 1S
	#define INDEX_BATT_COEF   3         //│ Коэффициент делителя
	#define INDEX_BATT_CELLS  4         //│ Количество банок
	#define INDEX_BATT_SAVE   5         //│ Сохранить на SD
	                                    //│
	#define NUM_ITEM_BATTERY  6         //│
//└─────────────────────────────────────────┘

static menu_typedef menu_battery CCMRAM;
static item_typedef item_battery[NUM_ITEM_BATTERY] CCMRAM;

static float battery_coef = 1.0f;
static uint8_t battery_cells = 1;
static const char * save_text = "Сохранить";

static uint32_t last_adc_time = 0;
static float cached_v_adc = 0.0f;
static uint32_t last_coef_time = 0;
static bool battery_loaded = false;

float Battery_GetAdcVoltage(void)
{
	uint32_t sum = 0;
	const int ADC_SAMPLES = 16;
	for (int i = 0; i < ADC_SAMPLES; i++) {
		HAL_ADC_Start(&hadc1);
		if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
			sum += HAL_ADC_GetValue(&hadc1);
		}
		HAL_ADC_Stop(&hadc1);
	}
	float raw_avg = (float)sum / (float)ADC_SAMPLES;
	return (raw_avg * 3.294f) / 4095.0f;
}

float Battery_GetVoltage(void)
{
	uint32_t now = HAL_GetTick();
	if (now - last_adc_time >= 250 || cached_v_adc == 0.0f) {
		last_adc_time = now;
		cached_v_adc = Battery_GetAdcVoltage();
	}
	return cached_v_adc * battery_coef;
}

float Battery_GetCellVoltage(void)
{
	float v_in = Battery_GetVoltage();
	uint8_t cells = (battery_cells > 0) ? battery_cells : 1;
	return v_in / (float)cells;
}

float Battery_GetCoef(void)
{
	return battery_coef;
}

void Battery_SetCoef(float k)
{
	if (k < 0.001f) k = 0.001f;
	if (k > 1000.0f) k = 1000.0f;
	battery_coef = k;
}

uint8_t Battery_GetCells(void)
{
	return (battery_cells > 0) ? battery_cells : 1;
}

void Battery_SetCells(uint8_t cells)
{
	if (cells < 1) cells = 1;
	if (cells > 32) cells = 32;
	battery_cells = cells;
}

void Battery_Load(void)
{
	battery_coef = 1.0f;
	battery_cells = 1;
	battery_loaded = true;

	Gen.pause();

	static FIL batt_file;
	bool opened = false;
	if (f_open(&batt_file, "/Config/battery.ini", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
		opened = true;
	} else if (f_open(&batt_file, "/battery.ini", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
		opened = true;
	}

	if (opened) {
		char line[48];
		while (f_gets(line, sizeof(line), &batt_file)) {
			line[strcspn(line, "\r\n")] = 0;
			char *p = line;
			while (*p == ' ' || *p == '\t') p++;
			if (*p == '#' || *p == ';' || *p == '\0') continue;

			if (strncmp(p, "COEF=", 5) == 0) {
				float val = (float)atof(p + 5);
				if (val > 0.0001f && val < 1000.0f) battery_coef = val;
			} else if (strncmp(p, "K=", 2) == 0) {
				float val = (float)atof(p + 2);
				if (val > 0.0001f && val < 1000.0f) battery_coef = val;
			} else if (strncmp(p, "CELLS=", 6) == 0) {
				int c = atoi(p + 6);
				if (c >= 1 && c <= 32) battery_cells = (uint8_t)c;
			} else if (strncmp(p, "S=", 2) == 0) {
				int c = atoi(p + 2);
				if (c >= 1 && c <= 32) battery_cells = (uint8_t)c;
			} else if ((*p >= '0' && *p <= '9') || *p == '.') {
				float val = (float)atof(p);
				if (val > 0.0001f && val < 1000.0f) battery_coef = val;
			}
		}
		f_close(&batt_file);
	}

	Gen.start();
}

bool Battery_Save(void)
{
	char str[64];
	int len = snprintf(str, sizeof(str), "COEF=%.3f\r\nCELLS=%u\r\n", battery_coef, (unsigned int)battery_cells);
	return Gen.Save_File("/Config/battery.ini", str, len);
}

void Battery_Init(void)
{
	if (!battery_loaded) {
		Battery_Load();
	}
}

static void update_display_voltages(void)
{
	float v_in = cached_v_adc * battery_coef;
	uint8_t cells = (battery_cells > 0) ? battery_cells : 1;
	float v_cell = v_in / (float)cells;

	snprintf(item_battery[INDEX_BATT_VIN].text, sizeof(item_battery[INDEX_BATT_VIN].text), "U вх: %.3f V", v_in);
	snprintf(item_battery[INDEX_BATT_VCELL].text, sizeof(item_battery[INDEX_BATT_VCELL].text), "U 1S: %.3f V", v_cell);
}

static void batt_pre_callback(void)
{
	uint32_t now = HAL_GetTick();
	if (now - last_adc_time >= 300) {
		last_adc_time = now;
		cached_v_adc = Battery_GetAdcVoltage();
		update_display_voltages();
		menu_battery.field.needUpdate = 1;
	}
}

static void batt_pre_VIN(uint32_t index)
{
	update_display_voltages();
}

static void batt_pre_VCELL(uint32_t index)
{
	update_display_voltages();
}

static void batt_VIN_click(void)
{
	cached_v_adc = Battery_GetAdcVoltage();
	last_adc_time = HAL_GetTick();
	update_display_voltages();
	menu_battery.field.needUpdate = 1;
}

static void batt_pre_COEF(uint32_t index)
{
	if (menu_battery.field.encoder_block && (index == INDEX_BATT_COEF)) {
		const int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;

		if (delta) {
			uint32_t now = HAL_GetTick();
			uint32_t dt = now - last_coef_time;
			last_coef_time = now;

			float step = 0.005f;
			if (dt < 30) {
				step = 0.1f;
			} else if (dt < 70) {
				step = 0.02f;
			} else if (dt < 130) {
				step = 0.01f;
			}

			battery_coef += delta * step;
			if (battery_coef < 0.001f) battery_coef = 0.001f;
			if (battery_coef > 1000.0f) battery_coef = 1000.0f;

			update_display_voltages();
			save_text = "Сохранить";
			menu_battery.field.needUpdate = 1;
		}

		snprintf(item_battery[INDEX_BATT_COEF].text, sizeof(item_battery[INDEX_BATT_COEF].text), "> %.3f <", battery_coef);
	} else {
		snprintf(item_battery[INDEX_BATT_COEF].text, sizeof(item_battery[INDEX_BATT_COEF].text), "Коэф: %.3f", battery_coef);
	}
}

static void batt_COEF_click(void)
{
	menu_battery.field.encoder_block = !menu_battery.field.encoder_block;
	last_coef_time = HAL_GetTick();
	menu_battery.field.needUpdate = 1;
}

static void batt_COEF_hold(void)
{
	battery_coef = 1.0f;
	update_display_voltages();
	save_text = "Сохранить";
	menu_battery.field.needUpdate = 1;
}

static void batt_pre_CELLS(uint32_t index)
{
	if (menu_battery.field.encoder_block && (index == INDEX_BATT_CELLS)) {
		const int delta = (Encoder.Right ? 1 : 0) - (Encoder.Left ? 1 : 0);
		Encoder.Left = 0;
		Encoder.Right = 0;

		if (delta) {
			int c = (int)battery_cells + delta;
			if (c < 1) c = 1;
			if (c > 32) c = 32;
			battery_cells = (uint8_t)c;

			update_display_voltages();
			save_text = "Сохранить";
			menu_battery.field.needUpdate = 1;
		}

		snprintf(item_battery[INDEX_BATT_CELLS].text, sizeof(item_battery[INDEX_BATT_CELLS].text), "> %d <", battery_cells);
	} else {
		snprintf(item_battery[INDEX_BATT_CELLS].text, sizeof(item_battery[INDEX_BATT_CELLS].text), "Банок: %d", battery_cells);
	}
}

static void batt_CELLS_click(void)
{
	menu_battery.field.encoder_block = !menu_battery.field.encoder_block;
	menu_battery.field.needUpdate = 1;
}

static void batt_CELLS_hold(void)
{
	battery_cells = 1;
	update_display_voltages();
	save_text = "Сохранить";
	menu_battery.field.needUpdate = 1;
}

static void batt_pre_SAVE(uint32_t index)
{
	snprintf(item_battery[INDEX_BATT_SAVE].text, sizeof(item_battery[INDEX_BATT_SAVE].text), "%s", save_text);
}

static void batt_SAVE_click(void)
{
	save_text = Battery_Save() ? "Сохранено" : "Ошибка SD";
	menu_battery.field.needUpdate = 1;
}

void PAGE_Battery(void)
{
	Battery_Init();

	for (int i = 0; i < NUM_ITEM_BATTERY; i++) {
		item_battery[i].text_color = -1;
		item_battery[i].preCallBackFunc = NULL;
		item_battery[i].callBackFunc_isClick = NULL;
		item_battery[i].callBackFunc_isHolded = NULL;
		item_battery[i].callBackFunc_isDouble = NULL;
		item_battery[i].field.exit = 0;
		item_battery[i].field.disable = 0;
		item_battery[i].nameGif = 0;
		item_battery[i].gif = 0;
	}

	cached_v_adc = Battery_GetAdcVoltage();
	last_adc_time = HAL_GetTick();

	snprintf(item_battery[INDEX_BATT_BACK].text, sizeof(item_battery[INDEX_BATT_BACK].text), "Назад");
	item_battery[INDEX_BATT_BACK].field.exit = 1;

	item_battery[INDEX_BATT_VIN].preCallBackFunc = &batt_pre_VIN;
	item_battery[INDEX_BATT_VIN].callBackFunc_isClick = &batt_VIN_click;

	item_battery[INDEX_BATT_VCELL].preCallBackFunc = &batt_pre_VCELL;
	item_battery[INDEX_BATT_VCELL].callBackFunc_isClick = &batt_VIN_click;

	item_battery[INDEX_BATT_COEF].preCallBackFunc = &batt_pre_COEF;
	item_battery[INDEX_BATT_COEF].callBackFunc_isClick = &batt_COEF_click;
	item_battery[INDEX_BATT_COEF].callBackFunc_isHolded = &batt_COEF_hold;

	item_battery[INDEX_BATT_CELLS].preCallBackFunc = &batt_pre_CELLS;
	item_battery[INDEX_BATT_CELLS].callBackFunc_isClick = &batt_CELLS_click;
	item_battery[INDEX_BATT_CELLS].callBackFunc_isHolded = &batt_CELLS_hold;

	item_battery[INDEX_BATT_SAVE].preCallBackFunc = &batt_pre_SAVE;
	item_battery[INDEX_BATT_SAVE].callBackFunc_isClick = &batt_SAVE_click;

	update_display_voltages();

	menu_battery.item_count   = NUM_ITEM_BATTERY;
	menu_battery.item_height  = 40;
	menu_battery.item_start_y = 0;

	menu_battery.font = Roboto_Medium_en_ru_30;
	menu_battery.items = item_battery;
	menu_battery.ColorBackground = COLOR_BACKGROUND;
	menu_battery.field.encoder_block = 0;
	menu_battery.field.verticalScroll = 0;
	menu_battery.preCallBackFunc = &batt_pre_callback;
	menu_battery.postCallBackFunc = NULL;

	save_text = "Сохранить";

	PAGE_Menu(&menu_battery, &item_battery[0], NUM_ITEM_BATTERY);

	Font_Smooth_Load(menu_setting.font);
	update_setting_voltage();
}

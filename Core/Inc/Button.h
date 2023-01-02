//HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)

//https://alexgyver.ru/gyverbutton/

//butt1.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
//if (butt1.isClick()) Serial.println("Click");         // проверка на один клик
//if (butt1.isSingle()) Serial.println("Single");       // проверка на один клик
//if (butt1.isDouble()) Serial.println("Double");       // проверка на двойной клик
//if (butt1.isTriple()) Serial.println("Triple");       // проверка на тройной клик
//if (butt1.hasClicks())                                // проверка на наличие нажатий
//Serial.println(butt1.getClicks());                  // получить (и вывести) число нажатий
//if (butt1.isPress()) Serial.println("Press");         // нажатие на кнопку (+ дебаунс)
//if (butt1.isRelease()) Serial.println("Release");     // отпускание кнопки (+ дебаунс)
//if (butt1.isHolded()) Serial.println("Holded");       // проверка на удержание
//if (butt1.isHold()) Serial.println("Holding");        // проверка на удержание
////if (butt1.state()) Serial.println("Hold");          // возвращает состояние кнопки
//if (butt1.isStep()) {                                 // если кнопка была удержана (это для инкремента)
//value++;                                            // увеличивать/уменьшать переменную value с шагом и интервалом
//Serial.println(value);                              // для примера выведем в порт
//}

// Варианты инициализации:
// GButton btn;							// без привязки к пину (виртуальная кнопка) и без указания типа (по умолч. LOW_PULL и NORM_OPEN)
// GButton btn(пин);					// с привязкой к пину и без указания типа (по умолч. HIGH_PULL и NORM_OPEN)
// GButton btn(пин, тип подключ.);		// с привязкой к пину и указанием типа подключения (HIGH_PULL / LOW_PULL) и без указания типа кнопки (по умолч. NORM_OPEN)
// GButton btn(пин, тип подключ., тип кнопки);			// с привязкой к пину и указанием типа подключения (HIGH_PULL / LOW_PULL) и типа кнопки (NORM_OPEN / NORM_CLOSE)
// GButton btn(BTN_NO_PIN, тип подключ., тип кнопки);	// без привязки к пину и указанием типа подключения (HIGH_PULL / LOW_PULL) и типа кнопки (NORM_OPEN / NORM_CLOSE)

//#define ZIGBEE

#ifndef ZIGBEE
  #include <stdbool.h>
  #include "main.h"
  #include "gpio.h"
#endif


#ifndef BUTTON_H_
#define BUTTON_H_

#pragma pack(push,1)
typedef struct {
	bool btn_deb: 1;
	bool hold_flag: 1;
	bool counter_flag: 1;
	bool isHolded_f: 1;
	bool isRelease_f: 1;
	bool isPress_f: 1;
	bool step_flag: 1;
	bool oneClick_f: 1;
	bool isOne_f: 1;
	bool inv_state: 1;
	bool mode: 1;
	bool type: 1;
	bool tickMode: 1;
	bool noPin: 1;
	bool counter_reset: 1;			//Добавлен

#ifndef ZIGBEE
    GPIO_TypeDef* GPIOx;
#endif

	uint16_t GPIO_Pin;

} ButtonFlags;
#pragma pack(pop)

#define BTN_NO_PIN -1
#define HIGH_PULL 0
#define LOW_PULL 1
#define NORM_OPEN 0
#define NORM_CLOSE 1
#define MANUAL 0
#define AUTO 1

typedef struct{

    ButtonFlags flags;
    uint8_t  _PIN = 0;
	uint16_t _debounce = 10;//60
	uint16_t _timeout = 500;
	uint16_t _click_timeout = 500;
	uint16_t _step_timeout = 400;
	uint8_t  btn_counter = 0, last_counter = 0, last_hold_counter = 0;
	uint32_t btn_timer = 0;
	bool btn_state = false;
	bool btn_flag = false;


	//GButton(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, bool type = HIGH_PULL, bool dir = NORM_OPEN);	// конструктор кнопки, принимает PIN пин, тип type (HIGH_PULL / LOW_PULL) и направление dir (NORM_OPEN / NORM_CLOSE)
														// HIGH_PULL - кнопка подключена к GND, пин подтянут к VCC, pinMode - INPUT_PULLUP (по умолчанию)
														// LOW_PULL - кнопка подключена к VCC, пин подтянут к GND, pinMode - INPUT
														// NORM_OPEN - кнопка по умолчанию разомкнута (по умолчанию)
														// NORM_CLOSE - кнопка по умолчанию замкнута



    //Zegbee
#ifdef ZIGBEE
    void init(uint16_t GPIO_Pin, bool type = HIGH_PULL, bool dir = NORM_OPEN)
        {
        	flags.GPIO_Pin = GPIO_Pin;
        	setType(type);
        	flags.mode = false;
        	flags.tickMode = false;
        	flags.inv_state = dir;
        }
#else
    void init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, bool type = HIGH_PULL, bool dir = NORM_OPEN)
    {
    	flags.GPIOx = GPIOx;
    	flags.GPIO_Pin = GPIO_Pin;

    	setType(type);
    	flags.mode = false;
    	flags.tickMode = false;
    	flags.inv_state = dir;
    }

#endif





	//установка времени антидребезга (по умолчанию 80 мс)
	void setDebounce(uint16_t debounce) {
		_debounce = debounce;
	}

	// установка таймаута удержания (по умолчанию 300 мс)
	void setTimeout(uint16_t new_timeout) {
		_timeout = new_timeout;
	}

	// установка таймаута между кликами (по умолчанию 500 мс)
	void setClickTimeout(uint16_t new_timeout) {
		_click_timeout = new_timeout;
	}

	// установка таймаута между инкрементами (по умолчанию 400 мс)
	void setStepTimeout(uint16_t step_timeout) {
		_step_timeout = step_timeout;
	}

	// установка типа кнопки (HIGH_PULL - подтянута к питанию, LOW_PULL - к gnd)
	void setType(bool type) {
		flags.type = type;
	}

	// установка направления (разомкнута/замкнута по умолчанию - NORM_OPEN, NORM_CLOSE)
	void setDirection(bool dir) {
		flags.inv_state = dir;
	}

	// (MANUAL / AUTO) ручной или автоматический опрос кнопки функцией tick()
	// MANUAL - нужно вызывать функцию tick() вручную
	// AUTO - tick() входит во все остальные функции и опрашивается сама
	void setTickMode(bool tickMode) {
		flags.tickMode = tickMode;
	}

	// ==================== TICK ====================
	// опрос внешнего значения (1 нажато, 0 не нажато) (для матричных, резистивных клавиатур и джойстиков)
	void tick(bool state) {
		flags.mode = true;
		btn_state = state ^ flags.inv_state;
		tick();
		flags.mode = false;
	}

	// опрос кнопки
	void tick() {
		// читаем пин
		//if (!flags.mode && !flags.noPin) btn_state = !_buttonRead() ^ (flags.inv_state ^ flags.type);

		if (!flags.mode)	// && !flags.noPin)

#ifdef ZIGBEE
			//Добавить сбда чтение порта
			btn_state = !GPIO_PinRead(GPIO, 0, flags.GPIO_Pin) ^ (flags.inv_state ^ flags.type);
#else
			btn_state = !HAL_GPIO_ReadPin(flags.GPIOx, flags.GPIO_Pin) ^ (flags.inv_state ^ flags.type);
#endif


		uint32_t thisMls = uwTick;
		//uint32_t thisMls = millis();

		// нажатие
		if (btn_state && !btn_flag) {
			if (!flags.btn_deb) {
				flags.btn_deb = true;
				btn_timer = thisMls;
			} else {
				if (thisMls - btn_timer >= _debounce) {
					btn_flag = true;
					flags.isPress_f = true;
					flags.oneClick_f = true;
				}
			}
		} else {
			flags.btn_deb = false;
		}

		// отпускание
		if (!btn_state && btn_flag) {
			btn_flag = false;
			if (!flags.hold_flag)
				btn_counter++;
			flags.hold_flag = false;
			flags.isRelease_f = true;
			btn_timer = thisMls;
			if (flags.step_flag) {
				last_counter = 0;
				btn_counter = 0;
				flags.step_flag = false;
			}
			if (flags.oneClick_f) {
				flags.oneClick_f = false;
				flags.isOne_f = true;
			}
		}

		// кнопка удерживается
		if (btn_flag && btn_state && (thisMls - btn_timer >= _timeout)
				&& !flags.hold_flag) {
			flags.hold_flag = true;
			last_hold_counter = btn_counter;
			//btn_counter = 0;
			//last_counter = 0;
			flags.isHolded_f = true;
			flags.step_flag = true;
			flags.oneClick_f = false;
			btn_timer = thisMls;
		}

		// обработка накликивания
		if ((thisMls - btn_timer >= _click_timeout) && (btn_counter != 0)
				&& !btn_state) {    //И здесь еще добавлен !btn_state
			last_counter = btn_counter;
			btn_counter = 0;
			flags.counter_flag = true;
		}

		// сброс накликивания						//Добавлено
		if (flags.counter_reset) {
			last_counter = 0;
			flags.counter_flag = false;
			flags.counter_reset = false;
		}
	}

	// возвращает true при нажатии на кнопку. Сбрасывается после вызова
	bool isPress() {
		if (flags.tickMode)
			tick();
		if (flags.isPress_f) {
			flags.isPress_f = false;
			return true;
		} else
			return false;
	}

	// возвращает true при отпускании кнопки. Сбрасывается после вызова
	bool isRelease() {
		if (flags.tickMode)
			tick();
		if (flags.isRelease_f) {
			flags.isRelease_f = false;
			return true;
		} else
			return false;
	}
	// возвращает true при клике. Сбрасывается после вызова
	bool isClick() {
		if (flags.tickMode)
			tick();
		if (flags.isOne_f) {
			flags.isOne_f = false;
			return true;
		} else
			return false;
	}

	//возвращает true при удержании дольше timeout. Сбрасывается после вызова
	bool isHolded() {
		if (flags.tickMode)
			tick();
		if (flags.isHolded_f) {
			flags.isHolded_f = false;
			return true;
		} else
			return false;
	}

	// возвращает true при нажатой кнопке, не сбрасывается
	bool isHold() {
		if (flags.tickMode)
			tick();
		if (flags.step_flag)
			return true;
		else
			return false;
	}

	// возвращает состояние кнопки
	bool state() {
		if (flags.tickMode)
			tick();
		return btn_state;
	}

	// возвращает true при одиночном клике. Сбрасывается после вызова
	bool isSingle() {
		if (flags.tickMode)
			tick();
		if (flags.counter_flag && last_counter == 1) {
			//		last_counter = 0;
			//		flags.counter_flag = false;
			flags.counter_reset = true;
			return true;
		} else
			return false;
	}

	// возвращает true при двойном клике. Сбрасывается после вызова
	bool isDouble() {
		if (flags.tickMode)
			tick();
		if (flags.counter_flag && last_counter == 2) {
			//		flags.counter_flag = false;
			//		last_counter = 0;
			flags.counter_reset = true;
			return true;
		} else
			return false;
	}

	// возвращает true при тройном клике. Сбрасывается после вызова
	bool isTriple() {
		if (flags.tickMode)
			tick();
		if (flags.counter_flag && last_counter == 3) {
			//		flags.counter_flag = false;
			//		last_counter = 0;
			flags.counter_reset = true;
			return true;
		} else
			return false;
	}


	// проверка на наличие кликов. Сбрасывается после вызова
	bool hasClicks() {
		if (flags.tickMode)
			tick();
		if (flags.counter_flag) {
			//		flags.counter_flag = false;
			flags.counter_reset = true;
			return true;
		} else
			return false;
	}

	// вернуть количество кликов
	uint8_t getClicks() {
		//	byte thisCount = last_counter;			// Исключено	14.01.2021
		//		last_counter = 0;
		flags.counter_reset = true;
		return last_counter;		//	return thisCount;	(замена)	14.01.2021
	}

	// вернуть количество кликов, предшествующее удерживанию
	uint8_t getHoldClicks() {
		if (flags.tickMode)
			tick();
		return last_hold_counter;//return flags.hold_flag ? last_hold_counter : 0;	(Замена)	 14.01.2021
	}

	// возвращает true по таймеру setStepTimeout, смотри пример
	bool isStep(uint8_t clicks) {
		if (flags.tickMode)
			tick();
		if (btn_counter == clicks && flags.step_flag
				&& (uwTick - btn_timer >= _step_timeout)) {
			btn_timer = uwTick;
			return true;
		} else
			return false;
	}

	// сбрасывает все is-флаги и счётчики
	void resetStates() {
		flags.isPress_f = false;
		flags.isRelease_f = false;
		flags.isOne_f = false;
		flags.isHolded_f = false;
		flags.step_flag = false;
		flags.counter_flag = false;
		last_hold_counter = 0;
		last_counter = 0;
	}
} button_typedef;











#endif /* BUTTON_H_ */

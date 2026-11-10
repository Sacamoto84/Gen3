#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ============================================================================
// AlexGyver uButton State Machine for Raspberry Pi Pico / RP2350
// Ultra-lightweight FSM button handler with debounce, click, hold & steps
// ============================================================================

#ifndef UB_DEB_TIME
#define UB_DEB_TIME 20   // Дебаунс лок-аут (мс)
#endif

#ifndef UB_HOLD_TIME
#define UB_HOLD_TIME 600 // Время до перехода в удержание (мс)
#endif

#ifndef UB_STEP_TIME
#define UB_STEP_TIME 400 // Время до перехода в импульсный режим (мс)
#endif

#ifndef UB_STEP_PRD
#define UB_STEP_PRD 200  // Период импульсов шага (мс)
#endif

#ifndef UB_CLICK_TIME
#define UB_CLICK_TIME 220 // Окно ожидания серии кликов (мс)
#endif

class uButton {
public:
    enum State : uint8_t {
        Idle,          // Простаивает
        Press,         // Нажатие [событие]
        Click,         // Клик (отпущено до удержания) [событие]
        WaitHold,      // Ожидание удержания
        Hold,          // Удержание [событие]
        ReleaseHold,   // Отпущено после удержания [событие]
        WaitStep,      // Ожидание импульсов
        Step,          // Импульс [событие]
        WaitNextStep,  // Ожидание следующего импульса
        ReleaseStep,   // Отпущено после импульсов [событие]
        Release,       // Отпущено в любом случае [событие]
        WaitClicks,    // Ожидание серии кликов
        Clicks,        // Серия кликов зафиксирована [событие]
        WaitTimeout,   // Ожидание таймаута
        Timeout,       // Таймаут [событие]
        SkipEvents,    // Пропуск событий до отпускания
    };

    uButton(uint8_t pin = 255)
        : _pin(pin), _tmr(0), _deb(0), _press(0), _steps(0), _clicks(0), _state(State::Idle) {}

    void init(uint8_t pin) {
        _pin = pin;
        gpio_init(_pin);
        gpio_set_dir(_pin, GPIO_IN);
        gpio_pull_up(_pin);
    }

    // Вызывать регулярно в цикле. Возвращает true при смене состояния
    bool tick() {
        bool raw_pressed = (_pin != 255) ? !gpio_get(_pin) : false; // Active LOW
        return poll(raw_pressed);
    }

    // Сбросить состояние
    void reset() {
        _state = State::Idle;
        _clicks = 0;
        _steps = 0;
    }

    // События
    bool press()        { return _state == State::Press; }
    bool click()        { return _state == State::Click; }
    bool hold()         { return _state == State::Hold; }
    bool releaseHold()  { return _state == State::ReleaseHold; }
    bool step()         { return _state == State::Step; }
    bool releaseStep()  { return _state == State::ReleaseStep; }
    bool release()      { return _state == State::Release; }
    bool hasClicks()    { return _state == State::Clicks; }
    uint8_t getClicks() { return _clicks; }
    bool pressing()     { return _press; }

    // Проверка с количеством кликов
    bool click(uint8_t clicks) { return _clicks == clicks && click(); }
    bool hold(uint8_t clicks)  { return _clicks == clicks && hold(); }

    State getState() { return State(_state); }

private:
    static const uint8_t _UB_MAX_CLICKS = 15;
    static const uint8_t _UB_MAX_STEPS = 127;

    uint8_t _pin;
    uint32_t _tmr;
    uint32_t _deb;

    uint8_t _press : 1;
    uint8_t _steps : 7;
    uint8_t _clicks : 4;
    uint8_t _state : 4;

    uint32_t _now() {
        return to_ms_since_boot(get_absolute_time());
    }

    bool poll(bool pressed) {
        uint32_t now = _now();
        if (_deb) {
            if ((now - _deb) >= UB_DEB_TIME) {
                _deb = 0;
            }
        } else if (_press != pressed) {
            _press = pressed;
            _deb = now;
        }
        return pollRaw(_press);
    }

    bool pollRaw(bool pressed) {
        State pstate = State(_state);
        uint32_t now = _now();
        uint32_t elapsed = now - _tmr;

        switch (_state) {
            case State::Idle:
                if (pressed) {
                    _state = State::Press;
                    _tmr = now;
                }
                break;

            case State::Press:
                _state = State::WaitHold;
                _tmr = now;
                break;

            case State::WaitHold:
                if (!pressed) {
                    _state = State::Click;
                    if (_clicks < _UB_MAX_CLICKS) ++_clicks;
                } else if (elapsed >= UB_HOLD_TIME) {
                    _state = State::Hold;
                    _tmr = now;
                }
                break;

            case State::Hold:
                _state = State::WaitStep;
                break;

            case State::WaitStep:
                if (!pressed) {
                    _state = State::ReleaseHold;
                } else if (elapsed >= UB_STEP_TIME) {
                    _state = State::Step;
                    _tmr = now;
                }
                break;

            case State::Step:
                _state = State::WaitNextStep;
                break;

            case State::WaitNextStep:
                if (!pressed) {
                    _state = State::ReleaseStep;
                } else if (elapsed >= UB_STEP_PRD) {
                    _state = State::Step;
                    if (_steps < _UB_MAX_STEPS) ++_steps;
                    _tmr = now;
                }
                break;

            case State::SkipEvents:
                if (!pressed) _state = State::Release;
                break;

            case State::ReleaseHold:
            case State::ReleaseStep:
            case State::Click:
                _state = State::Release;
                break;

            case State::Release:
                _steps = 0;
                _state = _clicks ? State::WaitClicks : State::Idle;
                _tmr = now;
                break;

            case State::WaitClicks:
                if (pressed) {
                    _state = State::Press;
                    _tmr = now;
                } else if (elapsed >= UB_CLICK_TIME) {
                    _state = State::Clicks;
                    _tmr = now;
                }
                break;

            case State::Clicks:
                _clicks = 0;
                _state = State::Idle;
                break;

            default:
                _state = State::Idle;
                break;
        }

        return pstate != _state;
    }
};

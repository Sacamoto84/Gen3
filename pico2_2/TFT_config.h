#ifndef TFT_CONFIG_H_
#define TFT_CONFIG_H_

// Интерфейсы
#define TFT_USE_SPI

// Подсветка (ШИМ)
#define TFT_USE_BL

// Драйвер дисплея
#define TFT_DRIVER_ST7789

// Частота обновления матрицы ST7789 в Герцах (60 - штатная стабильная; 75, 80, 90 - умеренный разгон; 120 - не все матрицы держат)
#define ST7789_FRAMERATE 75

// Цветовой режим 16 бит
#define TFT_USE_16BIT

#endif /* TFT_CONFIG_H_ */

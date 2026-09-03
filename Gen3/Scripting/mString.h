/*
    Документация:
    GitHub: https://github.com/GyverLibs/mString
    Возможности:
    - Практически полный аналог String, но статический
    - Есть несколько встроенных функций парсинга строк
    
    AlexGyver, alex@alexgyver.ru
    https://alexgyver.ru/
    MIT License

    Версии:
    v1.0 - релиз



  Создание
  mString<размер> str

  str.length() - текущий размер
  str.clear() - очистить
  str.add( [char / char* / Fchar / числа / String] ) - добавить
  str += [char / char* / Fchar / числа / String] - добавить
  str = str + [char / char* / Fchar / числа / String] - можно суммировать
  str == [char / char* / числа / String] - сравнить
  Для добавления/сравнения с mString используй str.buf

  Чтение символа по индексу
  str[idx]
  str.buf[idx]
  str.charAt(idx)

  Запись символа по индексу
  str[idx] = с
  str.buf[idx] = с
  str.setCharAt(idx, c)

  Доступ к char буферу
  str.buf
  str.c_str()

  str.toInt(from) - преобразовать в int начиная с from
  str.toUint(from) - преобразовать в uint начиная с from
  str.toFloat(from) - преобразовать в float начиная с from
  str.startsWith(char*) - начинается с


  	  char buf3[10];
  	  test.substring(0, 5, buf3);
  	  Serial.println(buf3);   // abcd,1

  str.substring(from, to, char* arr) - скопировать с from до to во внешний arr


  str.truncate(amount) - обрезать с конца на amount
  str.remove(idx, amount) - удалить (вырезать) amount символов начиная с idx
  --str.toLowerCase() - преобразовать буквы в нижний регистр
  --str.toUpperCase() - преобразовать буквы в верхний регистр
  str.indexOf(char, from) - найти символ char, искать начиная с from
  str.indexOf(char*, from) - найти строку char, искать начиная с from
  str.split(char* str[], div) - разделить на строки по разделителю div

  	  // разделение
  	  test = "hello,1234,test,5678";
  	  char* strings[5];
  	  int amount = test.split(strings, ',');
  	  // действие испортит строку, добавив в неё NULLы!

  	  for (int i = 0; i < amount; i++) {
    	Serial.println(strings[i]);
  	  }


  Парсинг пакета, в котором данные разделены разделителем div и оканчиваются символом ter
  str.parseBytes(data, len, div, ter) - распарсить содержимое в массив byte длиной len
  str.parseInts(data, len, div, ter) - распарсить содержимое в массив int длиной len
  div и ter по умолчанию , и NULL
  Например для парсинга таких пакетов: "12,34,56"
  Кастомные: "12;34;56;78\n"
  Парсим str.parseBytes(data, len, ';', '\n')


  // прочие тесты
  mString<50> test;

  test = "puk ";
  test += "kek ";
  test = test + "cheburek" + ',' + 123 + ',' + 3.14;


*/

#ifndef mString_h
#define mString_h

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

template < uint16_t _MS_SIZE >
class mString {
public:
    char buf[_MS_SIZE];
    
    mString() {
        clear();
    }
    
    uint16_t length() {
        return strlen(buf);
    }
    
    void clear() {
        buf[0] = '\0';
    }

    // add
    mString& add(const char c) {
        int len = length();
        if (len + 1 >= _MS_SIZE) return *this;
        buf[len++] = c;
        buf[len] = '\0';
        return *this;
    }


    mString& add(const char* data) {
        if (length() + strlen(data) >= _MS_SIZE) return *this;
        strcat(buf, data);
        return *this;
    }


    mString& add(uint16_t value) {
        return add((uint32_t)value);
    }
    mString& add(uint8_t value) {
        return add((uint32_t)value);
    }

    mString& add(int16_t value) {
        return add((int32_t)value);
    }
    mString& add(int8_t value) {
        return add((int32_t)value);
    }



    mString& add(mString data) {
        return add(data.buf);
    }

    // add +=
    mString& operator += (const char c) {
        return add(c);
    }
    mString& operator += (const char* data) {
        return add(data);
    }
    mString& operator += (uint32_t value) {
        return add(value);
    }
    mString& operator += (int32_t value) {
        return add(value);
    }
    mString& operator += (uint16_t value) {
        return add(value);
    }
    mString& operator += (int16_t value) {
        return add(value);
    }
    mString& operator += (uint8_t value) {
        return add(value);
    }
    mString& operator += (int8_t value) {
        return add(value);
    }
    mString& operator += (double value) {
        return add(value);
    }
    mString& operator += (mString data) {
        return add(data.buf);
    }


    // +
    mString operator + (const char c) {
        return (*this).add(c);
    }
    mString operator + (const char* data) {
        return (*this).add(data);
    }

    mString operator + (uint32_t value) {
        return (*this).add(value);
    }
    mString operator + (int32_t value) {
        return (*this).add(value);
    }
    mString operator + (uint16_t value) {
        return (*this).add(value);
    }
    mString operator + (int16_t value) {
        return (*this).add(value);
    }
    mString operator + (uint8_t value) {
        return (*this).add(value);
    }
    mString operator + (int8_t value) {
        return (*this).add(value);
    }
    mString operator + (double value) {
        return (*this).add(value);
    }
    mString operator + (mString data) {
        return (*this).add(data);
    }


    // assign
    mString& operator = (const char c) {
        clear();
        return add(c);
    }
    mString& operator = (const char* data) {
        clear();
        return add(data);
    }
    mString& operator = (uint32_t value) {
        clear();
        return add(value);
    }
    mString& operator = (int32_t value) {
        clear();
        return add(value);
    }
    mString& operator = (uint16_t value) {
        clear();
        return add(value);
    }
    mString& operator = (int16_t value) {
        clear();
        return add(value);
    }
    mString& operator = (uint8_t value) {
        clear();
        return add(value);
    }
    mString& operator = (int8_t value) {
        clear();
        return add(value);
    }
    mString& operator = (double value) {
        clear();
        return add(value);
    }


    // compare
    bool operator == (const char c) {
        return (buf[0] == c && buf[1] == 0);
    }
    bool operator == (const char* data) {
        return !strcmp(buf, data);
    }
    bool operator == (mString data) {
        return !strcmp(buf, data.buf);
    }


    // convert & parse
    char operator [] (uint16_t index) const {
        return buf[index];
    }
    char& operator [] (uint16_t index) {
        return buf[index];
    }
    char charAt(uint16_t index) {
        return buf[index];
    }
    void setCharAt(uint16_t index, char c) {
        buf[index] = c;
    }
    int32_t toInt(uint16_t from = 0) {
        return atol(buf + from);
    }
    uint32_t toUint(uint16_t from = 0) {
        return atol(buf + from);
    }
    float toFloat(uint16_t from = 0) {
        return atof(buf + from);
    }
    const char* c_str() {
        return buf;
    }

    bool startsWith(const char *data, uint16_t offset = 0) {
        return strlen(data) == strspn(buf + offset, data);
    }

    void substring(uint16_t from, uint16_t to, char* arr) {
        char backup = buf[++to];
        buf[to] = '\0';
        strcpy(arr, buf + from);
        buf[to] = backup;
    }
    int split(char** ptrs, char div = ',') {
        int i = 0, j = 1;
        ptrs[0] = buf;
        while (buf[i]) {
            if (buf[i] == div) {
                buf[i] = '\0';
                ptrs[j++] = buf + i + 1;
            }
            i++;
        }
        return j;
    }
    void truncate(uint16_t amount) {
        uint16_t len = length();
        if (amount >= len) clear();
        else buf[len - amount] = '\0';
    }
    void remove(uint16_t index, uint16_t count) {
        uint16_t len = length();
        if (index >= len) return;
        if (count <= 0) return;
        if (count > len - index) {
            count = len - index;
        }
        char *writeTo = buf + index;
        len = len - count;
        strncpy(writeTo, buf + index + count, len - index);
        buf[len] = 0;
    }





    int indexOf(char ch, uint16_t fromIndex = 0) {
        if (fromIndex >= length()) return -1;
        const char* temp = strchr(buf + fromIndex, ch);
        return (temp == NULL) ? -1 : (temp - buf);
    }

    // найти строку char, искать начиная с from
    int indexOf(char* ch, uint16_t fromIndex = 0) {
        if (fromIndex >= length()) return -1;
        const char* temp = strstr(buf + fromIndex, ch);
        return (temp == NULL) ? -1 : (temp - buf);
    }

    int parseBytes(uint8_t * data, int len, char div = ',', char ter = '\0') {
        int b = 0, c = 0;
        data[b] = 0;
        while (true) {
            if (buf[c] == div) {
                b++;
                c++;
                if (b == len) return b;
                data[b] = 0;
                continue;
            }
            if (buf[c] == ter || b == len) return b + 1;
            data[b] *= 10;
            data[b] += buf[c] - '0';
            c++;
        }
    }
    int parseInts(int* data, int len, char div = ',', char ter = '\0') {
        int b = 0, c = 0;
        data[b] = 0;
        while (true) {
            if (buf[c] == div) {
                b++;
                c++;
                if (b == len) return b;
                data[b] = 0;
                continue;
            }
            if (buf[c] == ter || b == len) return b + 1;
            data[b] *= 10;
            data[b] += buf[c] - '0';
            c++;
        }
    }
    //cast operators
    operator const char*(){ // теперь mString автоматом переконвертируется в cstring (const char *) если будет нужно
        return this->c_str();
    }
    /*
    Пример:
    void printString(const char * cstr){cout<<cstr;}
    mString<16> mStr = "Hello";
    
        Было:
        printString(mStr.c_str());
        Стало:
        printString(mStr);
    
    */
    
    operator bool()
    {
        return (this->length());
    }
    /* Теперь строка может возвращать bool 
    Пример:
        Было:
        if(str.length())
        {
            doSomeMagic();
        }
        
        Стало:
        if(str)
        {
            doSomeMagic();
        }
    */
private:
};
#endif

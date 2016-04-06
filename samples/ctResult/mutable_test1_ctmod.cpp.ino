#define UBRR0H
#include <Arduino.h>
#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_2[] PROGMEM ="a";
const char f_STRLT_1[] PROGMEM ="test";

void setup()
{
}

void loop()
{
  char buff[20];
  strcpy_P(buff,f_STRLT_2);
  strcat_P(buff,f_STRLT_1);
  char t = 'A';
  for (int i = 0; i < 10; i++) {
    buff[0] = (t + i);
  }
  Serial .  println (buff);
}

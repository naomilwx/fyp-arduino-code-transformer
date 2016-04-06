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
  Serial .  println (buff);
}

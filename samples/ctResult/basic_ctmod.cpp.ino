#define UBRR0H
#include <Arduino.h>
#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_1[] PROGMEM ="This is a string";

void setup()
{
}

void loop()
{
  Serial .  println (FS(f_STRLT_1));
}

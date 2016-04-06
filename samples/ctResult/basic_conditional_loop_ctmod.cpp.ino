#define UBRR0H
#include <Arduino.h>
#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_1[] PROGMEM ="item bigger than five";
const char f_STRLT_2[] PROGMEM ="item less than five";

void setup()
{
}

void loop()
{
  for (int i = 0; i < 10; i++) {
    int test = (random(10));
    if (test > 5) {
      Serial .  println (FS(f_STRLT_1));
    }
    else {
      Serial .  println (FS(f_STRLT_2));
    }
  }
}

#define UBRR0H
#include <Arduino.h>
#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_1[] PROGMEM ="abcde";
const char f_STRLT_2[] PROGMEM ="fgh";

void setup()
{
}

void loop()
{
  Serial .  println (FS(f_STRLT_1));
  for (int i = 0; i < 10; i++) {
    int r = (random(10));
    if (r > 4) {
      Serial .  println (FS(f_STRLT_2));
    }
    else {
      Serial .  println (FS(f_STRLT_1));
    }
  }
}

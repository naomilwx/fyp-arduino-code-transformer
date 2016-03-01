#define UBRR0H
#include <Arduino.h>

#define FS(x) reinterpret_cast<const __FlashStringHelper *>(x)

const char flash_string1[] PROGMEM = "item bigger than five";
const char flash_string2[] PROGMEM = "item less than five";

void setup() {}

void loop() {
  for(int i = 0; i < 10; i++{
    int test = random(10);
    if(test > 5) {
      Serial.println(FS(flash_string1));
    } else {
      Serial.println(FS(flash_string2));
    }
  }
}

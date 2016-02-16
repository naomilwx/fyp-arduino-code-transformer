#define UBRR0H
#include <Arduino.h>
#define FS(x) reinterpret_cast<const __FlashStringHelper *>(x)
const char flash_store_string_1[] PROGMEM = "item bigger than five";
const char flash_store_string_2[] PROGMEM = "item less than five";

void setup() {}

void loop() {
  int test = random(10);
  if(test > 5) {
    Serial.println(FS(flash_store_string1));
  } else {
    Serial.println(FS(flash_store_strin2));
  }
 
}

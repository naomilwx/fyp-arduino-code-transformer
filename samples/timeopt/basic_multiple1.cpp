#define UBRR0H
#include <Arduino.h>

#define FS(x) reinterpret_cast<const __FlashStringHelper *>(x)

const char flash_store_string_1[] PROGMEM = "abcde";
const char flash_store_string_2[] PROGMEM = "fgh";

void setup() {}

void loop() {
  char buff[6];
  strcpy_P(buff, flash_store_string_1);
  Serial.println(buff);
  int r = random(10);
  if(r > 4) {
    Serial.println(FS(flash_store_string_2));
  } else {
    Serial.println(buff);
  }
}

#define UBRR0H
#include <Arduino.h>
#define FS(x) reinterpret_cast<const __FlashStringHelper *>(x)

const char flash_store_string_1[] PROGMEM = "abcde";
const char flash_store_string_2[] PROGMEM = "fgh";

void setup() {}

void loop() {
  Serial.println(FS(flash_store_string_1));
  int r = random(10);
  if(r > 4) {
    Serial.println(FS(flash_store_string_2));
  } else {
    Serial.println(FS(flash_store_string_1));
  }
}

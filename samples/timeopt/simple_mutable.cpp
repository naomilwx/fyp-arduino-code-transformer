#define UBRR0H
#include <Arduino.h>

#define FS(x) reinterpret_cast<const __FlashStringHelper *>(x)

const char flash_store_string_1[] PROGMEM = "test";
const char flash_store_string_2[] PROGMEM = "a";

void setup() {}
void loop() {
  char buff[20];
  strcpy_P(buff, flash_store_string_2);
  strcat_P(buff, flash_store_string_1);
  
  Serial.println(buff);
}

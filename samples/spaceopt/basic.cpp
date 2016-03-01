#define UBRR0H
#include <Arduino.h>
#define FS(x) reinterpret_cast<const __FlashStringHelper *>(x)

const char flash_store_string_1[] PROGMEM = "This is a string";
void setup() {}

void loop() {
  Serial.println(FS(flash_store_string_1));
}

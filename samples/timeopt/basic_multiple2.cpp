#define UBRR0H
#include <Arduino.h>

#define FS(x) reinterpret_cast<const __FlashStringHelper *>(x)

const char flash_store_string_1[] PROGMEM = "abcde";
const char flash_store_string_2[] PROGMEM = "fgh";

void setup() {}

void loop() {
  char buff[10];
  strcpy_P(buff, flash_store_string_1);
  strcpy_P(&buff[6],flash_store_string_2);
  Serial.println(buff);


   for(int i = 0; i < 10; i++) {
  	int r = random(10);
	if(r > 4) {
    	  Serial.println(&buff[6]);
        } else {
	  Serial.println(buff);
        }
   }
}

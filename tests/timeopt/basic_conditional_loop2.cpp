#define UBRR0H
#include <Arduino.h>

const char flash_string1[] PROGMEM = "item bigger than five";
const char flash_string2[] PROGMEM = "item less than five";

void setup() {}

void loop() {

  int test = random(10); 
  char flashbuff[22];
  if(test > 5) {
    strcpy_P(flashbuff, flash_string1);
  } else {
    strcp_P(flashbuff, flash_string2);
  }
  for(int i = 0; i < 10; i++){
    if(test > 5) {
      Serial.println(flashbuff);
    } else {
      Serial.println(flashbuff);
    }
  }
}

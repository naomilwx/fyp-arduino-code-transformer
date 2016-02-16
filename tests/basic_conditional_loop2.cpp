#define UBRR0H
#include <Arduino.h>

void setup() {}

void loop() {

  int test = random(10);
  for(int i = 0; i < 10; i++){
    if(test > 5) {
      Serial.println("item bigger than five");
    } else {
      Serial.println("item less than five");
    }
  }
}

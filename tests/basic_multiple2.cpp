#define UBRR0H
#include <Arduino.h>

void setup() {}

void loop() {
   Serial.println("abcde");
   for(int i = 0; i < 10; i++) {
  	int r = random(10);
	if(r > 4) {
    	  Serial.println("fgh");
        } else {
	  Serial.println("abcde");
        }
   }
}

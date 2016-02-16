#define UBRR0H
#include <Arduino.h>

void setup() {}
void loop() {
  const char *x = "test";
  char buff[20];
  strcpy(buff, "a");
  strcat(buff, x);
  
  Serial.println(buff);
}

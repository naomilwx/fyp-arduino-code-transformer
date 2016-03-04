#define UBRR0H
#include <Arduino.h>

void setup() {}
void loop() {
  const char *x = "test";
  char buff[20];
  strcpy(buff, "a");
  strcat(buff, x);
  char t = 'A';
  for(int i = 0; i < 10; i++){
    buff[0] = t + i;
  }  
  Serial.println(buff);
}

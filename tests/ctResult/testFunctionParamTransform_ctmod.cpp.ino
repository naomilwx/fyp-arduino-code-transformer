#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_2[] PROGMEM ="hello";
const char f_STRLT_3[] PROGMEM ="lala";
const char f_STRLT_1[] PROGMEM ="test string print";

void printStr(const char *x)
{
  Serial .  println (x);
}

void setup()
{
  Serial .  println (FS(f_STRLT_1));
}

void loop()
{
  char arr[10];
  strcpy_P(arr,f_STRLT_2);
  printStr(arr);
  Serial .  println (FS(f_STRLT_3));
}

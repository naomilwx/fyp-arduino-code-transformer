#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_2[] PROGMEM ="this will not be modified";
const char ar_STRLT_2[] PROGMEM = "this is a test string";

void setup()
{
}

void loop()
{
  char arr[22];
// 
 strcpy_P(arr, ar_STRLT_2);
  arr[4] = '-';
  Serial .  println (arr);
  Serial .  println (FS(f_STRLT_2));
}

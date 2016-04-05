#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_4[] PROGMEM ="lala";
const char f_STRLT_1[] PROGMEM ="new string";
const char f_STRLT_2[] PROGMEM ="old string: ";
const char *f_STRLT_3 = "this is a string";

void testRef(const char *&x)
{
  const char *old_x = x;
  Serial .  println (old_x);
  x = f_STRLT_1;
  Serial .  println (x);
  Serial .  print (FS(f_STRLT_2));
  Serial .  println (old_x);
}

void setup()
{
}

void loop()
{
  const char *ptr = f_STRLT_3;
  testRef(ptr);
  Serial .  println (FS(f_STRLT_1));
  Serial .  println (FS(f_STRLT_4));
}

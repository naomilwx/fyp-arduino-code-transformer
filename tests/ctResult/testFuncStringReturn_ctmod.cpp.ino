#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_2[] PROGMEM ="test print";
const char *f_STRLT_1 = "this is a return string";

const char *returnString()
{
  return f_STRLT_1;
}

void setup()
{
  Serial .  begin (9600);
}

void loop()
{
  Serial .  println (FS(f_STRLT_2));
  Serial .  println ((returnString()));
}

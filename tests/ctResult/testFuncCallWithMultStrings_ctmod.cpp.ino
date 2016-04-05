#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_1[] PROGMEM ="string literal 1";
const char f_STRLT_2[] PROGMEM ="string literal 2";

void multArgs(int a,const char *arg1,const char *arg2)
{
  Serial .  println (arg1);
  Serial .  println (arg2);
}

void setup()
{
}

void loop()
{
  char f_arrbuf[34];
 strcpy_P(&f_arrbuf[0], f_STRLT_1);
 strcpy_P(&f_arrbuf[17], f_STRLT_2);
  multArgs(1,&f_arrbuf[0]/*f_STRLT_1*/,&f_arrbuf[17]/*f_STRLT_2*/);
}

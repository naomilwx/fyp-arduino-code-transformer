#define FS(x)(__FlashStringHelper*)(x)
const char f_STRLT_3[] PROGMEM ="second";
const char *f_STRLT_2 = "first";
const char *f_STRLT_1 = "ok";

const char *mayRetArg(const char *x1,const char *x2)
{
  if (random(10) < 5) {
    return x1;
  }
  return f_STRLT_1;
}

void setup()
{
  char f_arrbuf[7];
 strcpy_P(&f_arrbuf[0], f_STRLT_3);
  mayRetArg(f_STRLT_2,&f_arrbuf[0]/*f_STRLT_3*/);
}

void loop()
{
}

const char *mayRetArg(const char *x1, const char *x2) {
	if(random(10) < 5) {
		return x1;
	}
	return "ok";
}
void setup(){
	mayRetArg("first", "second");
}
void loop(){}

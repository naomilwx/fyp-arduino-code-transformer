void multArgs(int a, const char *arg1, const char *arg2) {
	Serial.println(arg1);
	Serial.println(arg2);
}

void setup(){}
void loop() {
	multArgs(1,"string literal 1", "string literal 2");
}

const char *returnString() {
	return "this is a return string";
}

void setup() {
  Serial.begin(9600);
}

void loop() {
	const char *x = "test print";
	Serial.println(x);
	Serial.println(returnString());
}

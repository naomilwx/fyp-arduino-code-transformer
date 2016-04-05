void testRef(const char * &x) {
	const char *old_x = x;
	Serial.println(old_x);
	x = "new string";
	Serial.println(x);
	Serial.print("old string: ");
	Serial.println(old_x);
}
void setup() {}

void loop() {
	const char *ptr = "this is a string";
	const char *other = "lala";
	testRef(ptr);
	Serial.println(ptr);
	Serial.println(other);
}

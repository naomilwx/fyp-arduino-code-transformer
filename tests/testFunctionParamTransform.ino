void printStr(char *x) {
	Serial.println(x);
}

void setup() {
	Serial.println("test string print");
}

void loop() {
	char arr[10];
	strcpy(arr, "hello");
	printStr(arr);
	Serial.println("lala");
}

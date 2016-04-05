void setup() {}
void loop() {
	char arr[] = "this is a test string";
	char unchanged[] = "this will not be modified";
	arr[4] = '-';

	Serial.println(arr);
	Serial.println(unchanged);
}

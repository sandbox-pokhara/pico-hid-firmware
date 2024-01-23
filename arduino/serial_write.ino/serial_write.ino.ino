void setup() {
	Serial.begin(115200);
}

void loop() {
  int incomingByte;
        
	if (Serial.available() > 0) {
		incomingByte = Serial.read();
    
		Serial.write(incomingByte);
    delay(10);
    Serial.flush();
    delay(1000);
	}
}


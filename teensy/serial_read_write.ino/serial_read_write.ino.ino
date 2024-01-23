#define HWSERIAL Serial2

void setup() {
  Serial.begin(9600);
  HWSERIAL.begin(115200);
}

void loop() {
  char incomingByte;

  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    Serial.print("UART Sent: ");
    Serial.println(incomingByte, DEC);

    HWSERIAL.write(incomingByte);
    delay(10);
    Serial.flush();
    HWSERIAL.flush();
    delay(1000);
  }
}

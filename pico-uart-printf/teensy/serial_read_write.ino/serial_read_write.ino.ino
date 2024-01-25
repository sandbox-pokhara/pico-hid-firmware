#include <Arduino.h>

#define HWSERIAL Serial2
#define BUTTON_PIN 2 // Replace with the actual pin number where your button is connected

void setup() {
  Serial.begin(9600);
  HWSERIAL.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // Check if the button is pressed
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Button is pressed, send UART message
    sendUartMessage();
    
    // Wait for a debounce delay (adjust as needed)
    delay(100);
  }
}

void sendUartMessage() {
  // const char* message = "Smou,1,-220,-221,-222,-223,1E";
  const char* message = "Smou,1,0,0,0,0,9999999999999E";
  
  // Print the message on the Serial Monitor
  Serial.print("UART Sent: ");
  Serial.println(message);

  // Send each character through hardware serial
  // for (int i = 0; i < 26; ++i) {
  //   HWSERIAL.write(message[i]);
  // }
  HWSERIAL.print(message);

  // Add a newline character for better readability
  HWSERIAL.write('\n');

  // Wait for a short delay (optional)
  delay(10);

  // Flush HWSERIAL buffer (optional)
  HWSERIAL.flush();
  delay(200);
}

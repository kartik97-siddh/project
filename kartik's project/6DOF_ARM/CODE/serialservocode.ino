#include <ESP32Servo.h>

Servo myservo;

const int ledPin = 2;       // LED connected to GPIO D2
int angle = 0;               // Current servo angle
int step = 1;                // Step for servo sweep
unsigned long previousMillis = 0;
const long ledInterval = 50; // Blink interval in ms
bool ledState = LOW;

void setup() {
  myservo.attach(21);        // Servo on GPIO 21
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // --- Servo sweep ---
  myservo.write(angle);
  angle += step;

  if (angle >= 180 || angle <= 0) {
    step = -step; // Reverse direction at ends
  }

  delay(20); // Keep small delay for smooth servo

  // --- LED blink ---
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= ledInterval) {
    previousMillis = currentMillis;
    ledState = !ledState;           // Toggle LED state
    digitalWrite(ledPin, ledState);
  }
}

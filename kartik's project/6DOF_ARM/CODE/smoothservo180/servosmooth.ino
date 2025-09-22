#include <ESP32Servo.h>

Servo myservo;

void setup() {
  myservo.attach(4);   // attach servo to GPIO 4
}

void loop() {
  // Sweep from 0 to 180 smoothly
  for (int angle = 0; angle <= 180; angle++) {
    myservo.write(angle);
    delay(20);   // smaller delay = smoother movement, but slower
  }

  // Sweep back from 180 to 0 smoothly
  for (int angle = 180; angle >= 0; angle--) {
    myservo.write(angle);
    delay(20);
  }
}

#include <ESP32Servo.h>

Servo myservo;  
int angle = 0;  

void setup() {
  Serial.begin(9600);          
  myservo.attach(4);   // attach servo to GPIO 4
  Serial.println("Enter angle (0 - 180):");
}

void loop() {
  if (Serial.available() > 0) {
    angle = Serial.parseInt();   // read integer from Serial Monitor

    // clear leftover characters (like '\n')
    while (Serial.available() > 0) {
      Serial.read();
    }

    // clamp values between 0 and 180
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    myservo.write(angle);      
    Serial.print("Servo moved to: ");
    Serial.println(angle);
  }
}

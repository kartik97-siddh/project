#include <micro_ros_arduino.h>
#include <ESP32Servo.h>
#include <std_msgs/msg/int16.h>

Servo myservo;

rclc_executor_t executor;
rcl_subscription_t subscriber;
std_msgs__msg__Int16 msg;

void subscription_callback(const void *msgin) {
  const std_msgs__msg__Int16 *msg = (const std_msgs__msg__Int16 *)msgin;
  int target = msg->data;
  if(target < 0) target = 0;
  if(target > 180) target = 180;

  int current = myservo.read();
  if(current < target) {
    for(int a = current; a <= target; a++) { myservo.write(a); delay(10); }
  } else {
    for(int a = current; a >= target; a--) { myservo.write(a); delay(10); }
  }
}

void setup() {
  set_microros_transports(); // depends on your setup
  myservo.attach(2);
  
  // Initialize node, subscription etc
  // rclc node, executor, subscriber setup here
}

void loop() {
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
}

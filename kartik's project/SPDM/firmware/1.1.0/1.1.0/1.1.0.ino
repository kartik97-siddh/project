#define BLYNK_TEMPLATE_ID "TMPL3MMYdyRxG"
#define BLYNK_TEMPLATE_NAME "ESP32"
#define BLYNK_AUTH_TOKEN "EsSVZFrmcmCOaj5Wqcd7s-E5iBtuQzh2"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Wi-Fi credentials
char ssid[] = "Maruti Home 405";        // Replace with your Wi-Fi SSID
char pass[] = "98242698";    // Replace with your Wi-Fi password
//char ssid[] = "vivo";        // Replace with your Wi-Fi SSID
//char pass[] = "vivo1234";  
// GPIO pin for controlling the induction heater
#define INDUCTION_PIN 2

// Duration definitions (in milliseconds) for each state
const unsigned long HEAT_SHORT_DURATION = 7000;  // 7 seconds
const unsigned long COOL_LONG_DURATION  = 5000;  // 5 seconds
const unsigned long HEAT_LONG_DURATION  = 15000;  // 15 seconds
const unsigned long COOL_SHORT_DURATION = 4000;   // 4 seconds

// Enum for representing the states of the heating cycle
enum SeqState { IDLE, HEAT_SHORT, COOL_LONG, HEAT_LONG, COOL_SHORT, FINISHED };
SeqState currentState = IDLE;  // Initialize the state as IDLE

// Variables to track timing for each state
unsigned long stateStartTime = 0;
BlynkTimer timer;  // Timer object for handling the sequence steps

// Function to control the induction heater (ON or OFF)
void setInduction(bool on) {
  digitalWrite(INDUCTION_PIN, on ? HIGH : LOW);  // Set the pin HIGH to turn ON, LOW to turn OFF
  Serial.print("Induction heater turned ");
  Serial.println(on ? "ON" : "OFF");
}

// Function to update the status on the Blynk app (using Virtual Pin V1)
void updateStatus(const char* status) {
  Blynk.virtualWrite(V1, status);  // Send status message to Virtual Pin V1 on Blynk
  Serial.println(status);  // Print status to Serial Monitor for debugging
}

// Function to start the sequence (triggered via Blynk button)
void startSequence() {
  if (currentState == IDLE || currentState == FINISHED) {  // Only start if in IDLE or FINISHED state
    currentState = HEAT_SHORT;  // Start the heating cycle with a short heat period
    stateStartTime = millis();  // Track the start time of the current state
    setInduction(true);  // Turn ON the induction heater
    updateStatus("Heating 10 seconds");  // Display status on Blynk
  } else {
    updateStatus("Sequence already running");  // Show error if sequence is already running
  }
}

// Function to stop the sequence
void stopSequence() {
  setInduction(false);  // Turn off the induction heater
  currentState = IDLE;  // Reset the state to IDLE
  updateStatus("System Stopped");  // Update status on Blynk
  Blynk.virtualWrite(V0, 0);  // Reset Virtual Pin V0 to indicate cycle completion
  Serial.println("System stopped, everything is OFF.");
}

// Function to handle state transitions and timing logic
void handleSequence() {
  unsigned long now = millis();  // Get the current time

  switch (currentState) {
    case IDLE:
      break;

    case HEAT_SHORT:
      if (now - stateStartTime >= HEAT_SHORT_DURATION) {  // Check if time has elapsed
        setInduction(false);  // Turn off induction heater after the short heating time
        currentState = COOL_LONG;  // Transition to the next state (cooling for long duration)
        stateStartTime = now;  // Reset start time
        updateStatus("Cooling 15 seconds");  // Display cooling status
      }
      break;

    case COOL_LONG:
      if (now - stateStartTime >= COOL_LONG_DURATION) {
        setInduction(true);  // Turn on induction heater for long heating phase
        currentState = HEAT_LONG;  // Transition to heating for long duration
        stateStartTime = now;  // Reset start time
        updateStatus("Heating 20 seconds");
      }
      break;

    case HEAT_LONG:
      if (now - stateStartTime >= HEAT_LONG_DURATION) {
        setInduction(false);  // Turn off induction heater after long heating
        currentState = COOL_SHORT;  // Transition to short cooling phase
        stateStartTime = now;  // Reset start time
        updateStatus("Cooling 4 seconds");
      }
      break;

    case COOL_SHORT:
      if (now - stateStartTime >= COOL_SHORT_DURATION) {
        currentState = FINISHED;  // Final state when cooling is done
        updateStatus("Cycle Completed");  // Notify completion
        Blynk.virtualWrite(V0, 0);  // Set Virtual Pin V0 to 0 (to indicate cycle completion)
      }
      break;

    case FINISHED: {
      static unsigned long finishTime = 0;
      static int step = 0;

      if (finishTime == 0) {  // If finish time is not yet set, initialize it
        finishTime = now;
        step = 0;
        updateStatus("Cycle Completed");
      }

      unsigned long elapsed = now - finishTime;  // Calculate elapsed time after cycle completion

      if (step == 0 && elapsed >= 5000) {
        updateStatus("");  // Clear status after 5 seconds
        finishTime = now;  // Reset finish time for the next step
        step = 1;
      }
      else if (step == 1 && elapsed >= 2000) {
        updateStatus("System Ready");  // Notify that the system is ready for the next cycle
        currentState = IDLE;  // Transition back to IDLE state
        finishTime = 0;  // Reset finish time
        step = 0;  // Reset step for the next cycle
      }
    }
    break;
  }
}

// Function to handle Blynk button press (Virtual Pin V0)
BLYNK_WRITE(V0) {
  int val = param.asInt();  // Get the value of the button press (1 for ON, 0 for OFF)
  if (val == 1) startSequence();  // Start the sequence if button is pressed
}

// Function to handle Blynk stop button press (Virtual Pin V2)
BLYNK_WRITE(V2) {
  int val = param.asInt();  // Get the value of the stop button (1 for Stop)
  Serial.print("Stop button pressed, value: ");
  Serial.println(val);  // Print value to Serial Monitor for debugging

  if (val == 1) {
    stopSequence();  // Stop the sequence if stop button is pressed
  }
}

// Setup function to initialize everything
void setup() {
  Serial.begin(115200);  // Start Serial communication for debugging
  pinMode(INDUCTION_PIN, OUTPUT);  // Set induction pin as an output
  setInduction(false);  // Make sure the induction heater is OFF initially

  // Wi-Fi connection logic with reconnection attempts
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);  // Start connecting to Wi-Fi

  // Continuously attempt to connect until Wi-Fi is connected
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");  // Print dots to indicate the attempt is in progress
    delay(500);  // Wait for 500ms before trying again
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // Print the device's IP address once connected

  // Blynk connection logic (continuous reconnection)
  Serial.println("Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // Connect to Blynk using the authentication token

  // Wait for Blynk connection (this is already handled in Blynk.begin, but this ensures Blynk is fully connected)
  while (!Blynk.connected()) {
    Serial.print(".");  // Print dots while trying to connect to Blynk
    delay(500);  // Wait for 500ms before trying again
  }

  Serial.println("\nBlynk connected!");  // Once connected, print confirmation

  timer.setInterval(100, handleSequence);  // Set up the timer to check the state every 100ms
}

// Main loop to run Blynk and handle sequence
void loop() {
  // Check if Wi-Fi is still connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost connection, reconnecting...");
    WiFi.reconnect();  // Use reconnect to handle Wi-Fi reconnection
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);  // Wait for a second before trying again
      Serial.print(".");  // Print a dot to indicate retrying
    }
    Serial.println("\nWiFi reconnected!");
  }

  // Check if Blynk is still connected
  if (!Blynk.connected()) {
    Serial.println("Blynk lost connection, reconnecting...");
    while (!Blynk.connected()) {
      delay(1000);  // Wait for a second before trying again
      Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // Try reconnecting to Blynk
      Serial.print(".");  // Print a dot to indicate retrying
    }
    Serial.println("\nBlynk reconnected!");
  }

  // Keep the Blynk connection alive and handle sequence
  Blynk.run();  
  timer.run();  // Handle the heating/cooling sequence
}

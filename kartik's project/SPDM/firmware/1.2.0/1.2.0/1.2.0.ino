// =======================================================
// Induction Heater Controller with Blynk
// Version: 1.2.0
// Author: Kartik
// Description: Timed heating/cooling sequence control via Blynk

/*
  Version 1.2.0:
  - Added state name display (V3)
  - Improved status messages & timing
  - Better WiFi/Blynk reconnection
  - Virtual pin constants added
  - UI sync on stop
  - Code cleanup & comments
*/
// =======================================================

#define BLYNK_TEMPLATE_ID "TMPL3MMYdyRxG"
#define BLYNK_TEMPLATE_NAME "ESP32"
#define BLYNK_AUTH_TOKEN "EsSVZFrmcmCOaj5Wqcd7s-E5iBtuQzh2"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// -------------------- Wi-Fi Credentials --------------------
char ssid[] = "Maruti Home 405";
char pass[] = "98242698";

// -------------------- Virtual Pin Mapping --------------------
#define VPIN_START_BUTTON    V0
#define VPIN_STATUS_TEXT     V1
#define VPIN_STOP_BUTTON     V2
#define VPIN_STATE_NAME      V3

// -------------------- Hardware Configuration --------------------
#define INDUCTION_PIN 2  // GPIO pin for induction heater

// -------------------- Sequence Durations (ms) --------------------
unsigned long HEAT_SHORT_DURATION = 7000;    // 7 seconds
unsigned long COOL_LONG_DURATION  = 5000;    // 5 seconds
unsigned long HEAT_LONG_DURATION  = 15000;   // 15 seconds
unsigned long COOL_SHORT_DURATION = 4000;    // 4 seconds

// -------------------- State Machine --------------------
enum SeqState { IDLE, HEAT_SHORT, COOL_LONG, HEAT_LONG, COOL_SHORT, FINISHED };
SeqState currentState = IDLE;

const char* stateNames[] = {
  "IDLE", "HEAT_SHORT", "COOL_LONG", "HEAT_LONG", "COOL_SHORT", "FINISHED"
};

// -------------------- Timing --------------------
unsigned long stateStartTime = 0;
BlynkTimer timer;

// -------------------- Function Prototypes --------------------
void setInduction(bool on);
void updateStatus(const char* status);
void updateStateName();
void startSequence();
void stopSequence();
void handleSequence();

// -------------------- Induction Control --------------------
void setInduction(bool on) {
  digitalWrite(INDUCTION_PIN, on ? HIGH : LOW);
  Serial.print("Induction heater turned ");
  Serial.println(on ? "ON" : "OFF");
}

// -------------------- UI Feedback --------------------
void updateStatus(const char* status) {
  Blynk.virtualWrite(VPIN_STATUS_TEXT, status);
  Serial.println(status);
}

void updateStateName() {
  Blynk.virtualWrite(VPIN_STATE_NAME, stateNames[currentState]);
}

// -------------------- Sequence Control --------------------
void startSequence() {
  if (currentState == IDLE || currentState == FINISHED) {
    currentState = HEAT_SHORT;
    stateStartTime = millis();
    setInduction(true);
    updateStatus("Step 1/4: Heating (7s)");
    updateStateName();
  } else {
    updateStatus("Sequence already running");
  }
}

void stopSequence() {
  setInduction(false);
  currentState = IDLE;
  updateStatus("System Stopped");
  updateStateName();
  Blynk.virtualWrite(VPIN_START_BUTTON, 0);
  Serial.println("System stopped, everything is OFF.");
}

// -------------------- Sequence Handler --------------------
void handleSequence() {
  unsigned long now = millis();

  switch (currentState) {
    case IDLE:
      break;

    case HEAT_SHORT:
      if (now - stateStartTime >= HEAT_SHORT_DURATION) {
        setInduction(false);
        currentState = COOL_LONG;
        stateStartTime = now;
        updateStatus("Step 2/4: Cooling (5s)");
        updateStateName();
      }
      break;

    case COOL_LONG:
      if (now - stateStartTime >= COOL_LONG_DURATION) {
        setInduction(true);
        currentState = HEAT_LONG;
        stateStartTime = now;
        updateStatus("Step 3/4: Heating (15s)");
        updateStateName();
      }
      break;

    case HEAT_LONG:
      if (now - stateStartTime >= HEAT_LONG_DURATION) {
        setInduction(false);
        currentState = COOL_SHORT;
        stateStartTime = now;
        updateStatus("Step 4/4: Cooling (4s)");
        updateStateName();
      }
      break;

    case COOL_SHORT:
      if (now - stateStartTime >= COOL_SHORT_DURATION) {
        currentState = FINISHED;
        updateStatus("Cycle Completed");
        updateStateName();
        Blynk.virtualWrite(VPIN_START_BUTTON, 0);
      }
      break;

    case FINISHED: {
      static unsigned long finishTime = 0;
      static int step = 0;

      if (finishTime == 0) {
        finishTime = now;
        step = 0;
      }

      unsigned long elapsed = now - finishTime;

      if (step == 0 && elapsed >= 5000) {
        updateStatus("");  // Clear message
        finishTime = now;
        step = 1;
      } else if (step == 1 && elapsed >= 2000) {
        updateStatus("System Ready");
        currentState = IDLE;
        updateStateName();
        finishTime = 0;
        step = 0;
      }
    }
    break;
  }
}

// -------------------- Blynk Handlers --------------------
BLYNK_WRITE(VPIN_START_BUTTON) {
  int val = param.asInt();
  if (val == 1) startSequence();
}

BLYNK_WRITE(VPIN_STOP_BUTTON) {
  int val = param.asInt();
  if (val == 1) stopSequence();
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  pinMode(INDUCTION_PIN, OUTPUT);
  setInduction(false);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Configure Blynk (non-blocking)
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  // Start timer for sequence handling
  timer.setInterval(100, handleSequence);
}

// -------------------- Loop --------------------
void loop() {
  // Reconnect WiFi if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("\nWiFi reconnected!");
  }

  // Reconnect Blynk if needed
  if (!Blynk.connected()) {
    Serial.println("Blynk disconnected. Reconnecting...");
    Blynk.connect();
  }

  Blynk.run();
  timer.run();
}

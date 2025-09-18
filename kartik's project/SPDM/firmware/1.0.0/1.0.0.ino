#define BLYNK_TEMPLATE_ID "TMPL3MMYdyRxG"
#define BLYNK_TEMPLATE_NAME "ESP32"
#define BLYNK_AUTH_TOKEN "EsSVZFrmcmCOaj5Wqcd7s-E5iBtuQzh2"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "vivo";
char pass[] = "vivo1234";

#define INDUCTION_PIN 2

const unsigned long HEAT_SHORT_DURATION = 10000;
const unsigned long COOL_LONG_DURATION  = 15000;
const unsigned long HEAT_LONG_DURATION  = 20000;
const unsigned long COOL_SHORT_DURATION = 4000;

enum SeqState { IDLE, HEAT_SHORT, COOL_LONG, HEAT_LONG, COOL_SHORT, FINISHED };
SeqState currentState = IDLE;

unsigned long stateStartTime = 0;
BlynkTimer timer;

void setInduction(bool on) {
  digitalWrite(INDUCTION_PIN, on ? HIGH : LOW);
  Serial.print("Induction heater turned ");
  Serial.println(on ? "ON" : "OFF");
}

void updateStatus(const char* status) {
  Blynk.virtualWrite(V1, status);
  Serial.println(status);
}

void startSequence() {
  if (currentState == IDLE || currentState == FINISHED) {
    currentState = HEAT_SHORT;
    stateStartTime = millis();
    setInduction(true);
    updateStatus("Heating 10 seconds");
  } else {
    updateStatus("Sequence already running");
  }
}

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
        updateStatus("Cooling 15 seconds");
      }
      break;

    case COOL_LONG:
      if (now - stateStartTime >= COOL_LONG_DURATION) {
        setInduction(true);
        currentState = HEAT_LONG;
        stateStartTime = now;
        updateStatus("Heating 20 seconds");
      }
      break;

    case HEAT_LONG:
      if (now - stateStartTime >= HEAT_LONG_DURATION) {
        setInduction(false);
        currentState = COOL_SHORT;
        stateStartTime = now;
        updateStatus("Cooling 4 seconds");
      }
      break;

    case COOL_SHORT:
      if (now - stateStartTime >= COOL_SHORT_DURATION) {
        currentState = FINISHED;
        updateStatus("Cycle Completed");
        Blynk.virtualWrite(V0, 0);
      }
      break;

    case FINISHED: {
      static unsigned long finishTime = 0;
      static int step = 0;

      if (finishTime == 0) {
        finishTime = now;
        step = 0;
        updateStatus("Cycle Completed");
      }

      unsigned long elapsed = now - finishTime;

      if (step == 0 && elapsed >= 5000) {
        updateStatus("");
        finishTime = now;
        step = 1;
      }
      else if (step == 1 && elapsed >= 2000) {
        updateStatus("System Ready");
        currentState = IDLE;
        finishTime = 0;
        step = 0;
      }
    }
    break;
  }
}

BLYNK_WRITE(V0) {
  int val = param.asInt();
  if (val == 1) startSequence();
}

void setup() {
  Serial.begin(115200);
  pinMode(INDUCTION_PIN, OUTPUT);
  setInduction(false);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);

  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if (millis() - wifiStartTime > 15000) {
      Serial.println("\nFailed to connect to Wi-Fi.");
      return;
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  unsigned long blynkStartTime = millis();
  while (!Blynk.connected()) {
    Serial.print(".");
    delay(500);
    if (millis() - blynkStartTime > 30000) {
      Serial.println("\nFailed to connect to Blynk.");
      return;
    }
  }

  Serial.println("\nBlynk connected!");
  timer.setInterval(100, handleSequence);
}

void loop() {
  Blynk.run();
  timer.run();
}

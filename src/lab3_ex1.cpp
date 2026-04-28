// - RED LED - `D26`
// - Green LED - `D27`
// - Blue LED - `D14`
// - Yellow LED - `D12`

// - Button (Active high) - `D25`
// - Light sensor (analog) - `D33`

// - LCD I2C - SDA: `D21`
// - LCD I2C - SCL: `D22`

/**************************************
 * LAB 3 - EXERCISE 1: 
 * 
 * I want to publish message to mqtt every 5 second
 **************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>

// ===== WiFi =====
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ===== MQTT =====
const char* mqtt_broker = "mqtt.iotserver.uz";
const int mqtt_port = 1883;
const char* mqtt_username = "userTTPU";
const char* mqtt_password = "mqttpass";

const char* topic_light  = "ttpu/iot/abror/sensors/light";
const char* topic_button = "ttpu/iot/abror/events/button";

// ===== Pins =====
#define RED_LED 26
#define GREEN_LED 27
#define BLUE_LED 14
#define YELLOW_LED 12
#define BUTTON_PIN 25
#define LIGHT_PIN 33

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// ===== Timing =====
unsigned long lastPublishTime = 0;
const long publishInterval = 5000;

// ===== Button debounce =====
bool lastStableState = LOW;
bool lastReading = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// ===== FUNCTIONS =====
void connectWiFi() {
  Serial.println("Connecting WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!mqtt_client.connected()) {
    String client_id = "esp32-" + String(WiFi.macAddress());

    Serial.println("Connecting MQTT...");
    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("MQTT connected!");
    } else {
      Serial.print("Failed, rc=");
      Serial.println(mqtt_client.state());
      delay(3000);
    }
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  // REQUIRED: init LEDs
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  pinMode(BUTTON_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);

  connectWiFi();

  // NTP for real Unix timestamp
  configTime(0, 0, "pool.ntp.org");

  mqtt_client.setServer(mqtt_broker, mqtt_port);
  connectMQTT();
}

// ===== LOOP =====
void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt_client.connected()) connectMQTT();

  mqtt_client.loop();

  unsigned long now = millis();

  // --- 1. periodic publish ---
  if (now - lastPublishTime >= publishInterval) {
    lastPublishTime = now;

    int light = analogRead(LIGHT_PIN);
    long timestamp = time(nullptr); // real Unix time

    String payload = "{";
    payload += "\"light\":" + String(light) + ",";
    payload += "\"timestamp\":" + String(timestamp);
    payload += "}";

    mqtt_client.publish(topic_light, payload.c_str());

    Serial.print("[PUB] ");
    Serial.print(topic_light);
    Serial.print(" -> ");
    Serial.println(payload);
  }

  // --- 2. button event ---
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) lastDebounceTime = now;

  if ((now - lastDebounceTime) > debounceDelay) {
    if (reading != lastStableState) {
      lastStableState = reading;

      String event = reading ? "PRESSED" : "RELEASED";
      long timestamp = time(nullptr);

      String payload = "{";
      payload += "\"event\":\"" + event + "\",";
      payload += "\"timestamp\":" + String(timestamp);
      payload += "}";

      mqtt_client.publish(topic_button, payload.c_str());

      Serial.print("[PUB] ");
      Serial.print(topic_button);
      Serial.print(" -> ");
      Serial.println(payload);
    }
  }

  lastReading = reading;
}
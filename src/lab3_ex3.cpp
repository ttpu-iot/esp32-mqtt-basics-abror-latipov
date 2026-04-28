// - RED LED - `D26`
// - Green LED - `D27`
// - Blue LED - `D14`
// - Yellow LED - `D12`

// - Button (Active high) - `D25`
// - Light sensor (analog) - `D33`

// - LCD I2C - SDA: `D21`
// - LCD I2C - SCL: `D22`

/**************************************
 * LAB 3 - EXERCISE 3
 **************************************/


#include "Arduino.h"
#include "WiFi.h"
#include <ArduinoJson.h>
#include "PubSubClient.h"
#include <LiquidCrystal_I2C.h>
#include <time.h>

// ===== WiFi =====
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ===== MQTT =====
const char* mqtt_broker = "mqtt.iotserver.uz";
const int mqtt_port = 1883;
const char* mqtt_username = "userTTPU";
const char* mqtt_password = "mqttpass";

// ===== Topics =====
const char* topic_btn = "ttpu/iot/abror/events/button";
const char* topic_display = "ttpu/iot/abror/display";

const char* topic_red    = "ttpu/iot/abror/led/red";
const char* topic_green  = "ttpu/iot/abror/led/green";
const char* topic_blue   = "ttpu/iot/abror/led/blue";
const char* topic_yellow = "ttpu/iot/abror/led/yellow";

// ===== Pins =====
#define RED 26
#define GREEN 27
#define BLUE 14
#define YELLOW 12
#define BUTTON 25

// ===== Objects =====
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== Button debounce =====
bool lastStable = LOW;
bool lastReading = LOW;
unsigned long lastDebounce = 0;
const int debounceDelay = 50;

// ===== FUNCTIONS =====
void connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi OK");
}

void connectMQTT() {
  while (!mqtt_client.connected()) {
    Serial.println("Connecting MQTT...");

    String id = "esp32-" + String(WiFi.macAddress());

    if (mqtt_client.connect(id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("MQTT OK");
      
    if (mqtt_client.connect(id.c_str(), mqtt_username, mqtt_password)) {
      mqtt_client.subscribe(topic_red);
      mqtt_client.subscribe(topic_green);
      mqtt_client.subscribe(topic_blue);
      mqtt_client.subscribe(topic_yellow);
      mqtt_client.subscribe(topic_display);
      Serial.println("MQTT OK");
    } else delay(3000);
  }
}

// --- LED control ---
void setLED(int pin, const char* name, const char* state) {
  if (strcmp(state, "ON") == 0) {
    digitalWrite(pin, HIGH);
    Serial.printf("[LED] %s ON\n", name);
  } else if (strcmp(state, "OFF") == 0) {
    digitalWrite(pin, LOW);
    Serial.printf("[LED] %s OFF\n", name);
  }
}

// --- MQTT callback ---
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("[MQTT] Received on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, msg)) {
    Serial.println("JSON error");
    return;
  }

  // LED control
  const char* state = doc["state"];
  if (state) {
    if (strcmp(topic, topic_red) == 0) setLED(RED, "RED", state);
    else if (strcmp(topic, topic_green) == 0) setLED(GREEN, "GREEN", state);
    else if (strcmp(topic, topic_blue) == 0) setLED(BLUE, "BLUE", state);
    else if (strcmp(topic, topic_yellow) == 0) setLED(YELLOW, "YELLOW", state);
  }

  // LCD display
  const char* text = doc["text"];
  if (text && strcmp(topic, topic_display) == 0) {
    lcd.clear();

    // Line 1
    lcd.setCursor(0, 0);
    String t = String(text);
    lcd.print(t.substring(0, 16));

    // Line 2 (time)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char buf[20];
      strftime(buf, sizeof(buf), "%d/%m %H:%M:%S", &timeinfo);

      lcd.setCursor(0, 1);
      lcd.print(buf);
    }

    Serial.println("[LCD] Updated");
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(BUTTON, INPUT);

  connectWiFi();

  // NTP (UTC+5)
  configTime(18000, 0, "pool.ntp.org");

  // NTP sync confirmation
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("NTP time synced!");
  } else {
    Serial.println("NTP sync failed!");
  }

  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(callback);
  connectMQTT();

  lcd.init();
  lcd.backlight();
}

// ===== LOOP =====
void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt_client.connected()) connectMQTT();

  mqtt_client.loop();

  // --- Button publish ---
  bool reading = digitalRead(BUTTON);

  if (reading != lastReading) lastDebounce = millis();

  if ((millis() - lastDebounce) > debounceDelay) {
    if (reading != lastStable) {
      lastStable = reading;

      const char* event = reading ? "PRESSED" : "RELEASED";

      StaticJsonDocument<128> doc;
      doc["event"] = event;
      doc["timestamp"] = time(nullptr);

      char buffer[128];
      serializeJson(doc, buffer);

      mqtt_client.publish(topic_btn, buffer);

      Serial.print("[PUB] ");
      Serial.print(topic_btn);
      Serial.print(" -> ");
      Serial.println(buffer);
    }
  }

  lastReading = reading;
}
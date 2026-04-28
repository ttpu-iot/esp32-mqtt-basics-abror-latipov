// - RED LED - `D26`
// - Green LED - `D27`
// - Blue LED - `D14`
// - Yellow LED - `D12`

// - Button (Active high) - `D25`
// - Light sensor (analog) - `D33`

// - LCD I2C - SDA: `D21`
// - LCD I2C - SCL: `D22`

/**************************************
 * LAB 3 - EXERCISE 2
 **************************************/


#include "Arduino.h"
#include "WiFi.h"
#include <ArduinoJson.h>
#include "PubSubClient.h"

// ===== WiFi =====
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ===== MQTT =====
const char* mqtt_broker = "mqtt.iotserver.uz";
const int mqtt_port = 1883;
const char* mqtt_username = "userTTPU";
const char* mqtt_password = "mqttpass";

// ===== Topics =====
const char* topic_red    = "ttpu/iot/abror/led/red";
const char* topic_green  = "ttpu/iot/abror/led/green";
const char* topic_blue   = "ttpu/iot/abror/led/blue";
const char* topic_yellow = "ttpu/iot/abror/led/yellow";

// ===== Pins =====
#define RED_LED_PIN    26
#define GREEN_LED_PIN  27
#define BLUE_LED_PIN   14
#define YELLOW_LED_PIN 12

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

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

// --- LED control helper ---
void controlLED(int pin, const char* name, const char* state) {
    if (strcmp(state, "ON") == 0) {
        digitalWrite(pin, HIGH);
        Serial.printf("[LED] %s -> ON\n", name);
    } else if (strcmp(state, "OFF") == 0) {
        digitalWrite(pin, LOW);
        Serial.printf("[LED] %s -> OFF\n", name);
    } else {
        Serial.println("[ERROR] Invalid state value");
    }
}

// --- MQTT callback ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT] Received on ");
    Serial.print(topic);
    Serial.print(": ");

    // Print payload directly (no String conversion)
    Serial.write(payload, length);
    Serial.println();

    // Parse JSON
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, (const char*)payload, length);

    if (error) {
        Serial.print("[ERROR] JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* state = doc["state"];
    if (!state) {
        Serial.println("[ERROR] 'state' field missing");
        return;
    }

    // Match topic → LED
    if (strcmp(topic, topic_red) == 0) {
        controlLED(RED_LED_PIN, "Red LED", state);
    }
    else if (strcmp(topic, topic_green) == 0) {
        controlLED(GREEN_LED_PIN, "Green LED", state);
    }
    else if (strcmp(topic, topic_blue) == 0) {
        controlLED(BLUE_LED_PIN, "Blue LED", state);
    }
    else if (strcmp(topic, topic_yellow) == 0) {
        controlLED(YELLOW_LED_PIN, "Yellow LED", state);
    }
}

// --- MQTT connect ---
void connectMQTT() {
    while (!mqtt_client.connected()) {
        Serial.println("Connecting MQTT...");

        String client_id = "esp32-" + String(WiFi.macAddress());

        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("MQTT connected!");

            mqtt_client.subscribe(topic_red);
            mqtt_client.subscribe(topic_green);
            mqtt_client.subscribe(topic_blue);
            mqtt_client.subscribe(topic_yellow);

            Serial.println("Subscribed to all LED topics");
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

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);

    // all OFF initially
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);

    connectWiFi();

    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqttCallback);

    connectMQTT();
}

// ===== LOOP =====
void loop() {
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    if (!mqtt_client.connected()) connectMQTT();

    mqtt_client.loop();
}
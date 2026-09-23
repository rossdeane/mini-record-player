#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>

#include "config.h"

constexpr uint8_t SDA_PIN = 21;
constexpr uint8_t SCL_PIN = 22;
constexpr uint8_t LED_PIN = 4;
constexpr unsigned long TAG_REMOVAL_MS = 500;
constexpr unsigned long RECONNECT_MS = 5000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_PN532 nfc(-1, -1);  // PN532 over I2C
Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

bool tagPresent = false;
unsigned long lastSeen = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastBreath = 0;
float phase = 0;
String currentTag;
String clientId;

void setLed(uint8_t red, uint8_t green, uint8_t blue) {
  pixel.setPixelColor(0, pixel.Color(red, green, blue));
  pixel.show();
}

String uidToHex(const uint8_t* uid, uint8_t length) {
  String value;
  value.reserve(length * 2);
  const char digits[] = "0123456789abcdef";
  for (uint8_t i = 0; i < length; ++i) {
    value += digits[uid[i] >> 4];
    value += digits[uid[i] & 0x0f];
  }
  return value;
}

void publishState() {
  // Retained state lets a newly started subscriber see the current tag.
  mqtt.publish(MQTT_TOPIC, tagPresent ? currentTag.c_str() : "removed", true);
}

void maintainConnections(unsigned long now) {
  if (WiFi.status() != WL_CONNECTED) {
    if (mqtt.connected()) mqtt.disconnect();
    if (now - lastWifiAttempt >= RECONNECT_MS) {
      lastWifiAttempt = now;
      WiFi.reconnect();
    }
    return;
  }

  if (mqtt.connected()) {
    mqtt.loop();
    return;
  }
  if (now - lastMqttAttempt < RECONNECT_MS) return;

  lastMqttAttempt = now;
  Serial.print("MQTT connect... ");
  if (mqtt.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("connected");
    publishState();
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
  pixel.begin();
  setLed(60, 0, 0);

  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 not found; check wiring and I2C mode");
    while (true) delay(1000);
  }
  nfc.SAMConfig();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttempt = millis();

  clientId = "rec-" + WiFi.macAddress();
  clientId.replace(":", "");
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
}

void loop() {
  unsigned long now = millis();
  maintainConnections(now);

  uint8_t uid[10];
  uint8_t length = 0;
  const bool found = nfc.readPassiveTargetID(
      PN532_MIFARE_ISO14443A, uid, &length, 30);
  now = millis();

  if (found && length > 0) {
    lastSeen = now;
    const String tag = uidToHex(uid, length);
    if (!tagPresent || tag != currentTag) {
      tagPresent = true;
      currentTag = tag;
      Serial.print("TAG ");
      Serial.println(currentTag);
      if (mqtt.connected()) publishState();
    }
  } else if (tagPresent && now - lastSeen > TAG_REMOVAL_MS) {
    tagPresent = false;
    currentTag = "";
    Serial.println("removed");
    if (mqtt.connected()) publishState();
  }

  if (WiFi.status() != WL_CONNECTED || !mqtt.connected()) {
    setLed(60, 0, 0);
  } else if (tagPresent) {
    if (now - lastBreath >= 10) {
      lastBreath = now;
      phase += 0.02f;
      if (phase > TWO_PI) phase = 0;
      const uint8_t brightness = 5 + ((sin(phase) + 1) / 2) * 70;
      setLed(0, 0, brightness);
    }
  } else {
    setLed(0, 60, 0);
  }

  delay(2);
}

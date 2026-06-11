/*
 * ESP8266 PIR motion sensor
 *
 * Sends motion events to an IFTTT Webhooks applet over HTTPS.
 * Copy secrets.h.example to secrets.h and configure it before uploading.
 */

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <memory>

enum TlsValidationMode {
  TLS_VALIDATE_CA,
  TLS_VALIDATE_FINGERPRINT,
  TLS_INSECURE
};

#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "Using placeholder settings. Copy secrets.h.example to secrets.h and configure it."
const char WIFI_SSID[] = "YOUR_WIFI_SSID";
const char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
const char IFTTT_API_KEY[] = "YOUR_IFTTT_WEBHOOK_KEY";
const char IFTTT_EVENT[] = "pirtrigger";
const TlsValidationMode TLS_VALIDATION = TLS_VALIDATE_CA;
const char IFTTT_CA_CERT[] PROGMEM = "";
const char IFTTT_FINGERPRINT[] = "";
#endif

constexpr uint8_t LED_PIN = 0;
constexpr uint8_t PIR_PIN = 14;
constexpr bool PIR_ACTIVE_STATE = LOW;

constexpr uint32_t MOTION_COOLDOWN_MS = 15UL * 1000UL;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20UL * 1000UL;
constexpr uint16_t HTTP_TIMEOUT_MS = 10UL * 1000UL;
constexpr uint8_t MAX_REDIRECTS = 3;
constexpr uint32_t BATTERY_KEEPALIVE_INTERVAL_MS = 15UL * 1000UL;

constexpr bool POWER_SAVE = true;
constexpr bool FIVE_VOLT_BATTERY_KEEPALIVE = false;

int motionState = HIGH;
uint32_t lastMotionEventAt = 0;
bool hasSentMotionEvent = false;
uint32_t lastBatteryKeepaliveAt = 0;

void blink() {
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
}

void powerSave() {
  if (!POWER_SAVE) {
    return;
  }

  Serial.println(F("Entering modem sleep"));
  delay(100);
  WiFi.forceSleepBegin();
  delay(1);
}

bool connectWiFi() {
  if (POWER_SAVE) {
    Serial.println(F("Waking Wi-Fi"));
    WiFi.forceSleepWake();
    delay(1);
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Wi-Fi connection timed out"));
    return false;
  }

  Serial.print(F("Wi-Fi connected, IP: "));
  Serial.println(WiFi.localIP());
  return true;
}

bool configureTls(BearSSL::WiFiClientSecure &client,
                  std::unique_ptr<BearSSL::X509List> &trustAnchor) {
  switch (TLS_VALIDATION) {
    case TLS_VALIDATE_CA:
      if (IFTTT_CA_CERT[0] == '\0') {
        Serial.println(F("TLS CA certificate is not configured"));
        return false;
      }
      trustAnchor.reset(new BearSSL::X509List(IFTTT_CA_CERT));
      client.setTrustAnchors(trustAnchor.get());
      return true;

    case TLS_VALIDATE_FINGERPRINT:
      if (!client.setFingerprint(IFTTT_FINGERPRINT)) {
        Serial.println(F("TLS fingerprint is invalid or not configured"));
        return false;
      }
      return true;

    case TLS_INSECURE:
      Serial.println(F("Warning: TLS certificate validation is disabled"));
      client.setInsecure();
      return true;
  }

  Serial.println(F("Unknown TLS validation mode"));
  return false;
}

bool sendEvent() {
  bool success = false;

  if (!connectWiFi()) {
    powerSave();
    return false;
  }

  BearSSL::WiFiClientSecure client;
  std::unique_ptr<BearSSL::X509List> trustAnchor;
  if (!configureTls(client, trustAnchor)) {
    powerSave();
    return false;
  }

  client.setTimeout(HTTP_TIMEOUT_MS);

  const String url = String(F("https://maker.ifttt.com/trigger/")) +
                     IFTTT_EVENT + F("/with/key/") + IFTTT_API_KEY;

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setRedirectLimit(MAX_REDIRECTS);

  Serial.print(F("Sending IFTTT event: "));
  Serial.println(IFTTT_EVENT);

  if (!http.begin(client, url)) {
    Serial.println(F("Unable to initialize HTTPS request"));
  } else {
    const int statusCode = http.GET();
    if (statusCode >= 200 && statusCode < 300) {
      Serial.print(F("IFTTT accepted the event, HTTP "));
      Serial.println(statusCode);
      success = true;
    } else if (statusCode > 0) {
      Serial.print(F("IFTTT returned HTTP "));
      Serial.println(statusCode);
    } else {
      Serial.print(F("HTTPS request failed: "));
      Serial.println(HTTPClient::errorToString(statusCode));
    }
    http.end();
  }

  client.stop();
  powerSave();
  return success;
}

void batteryKeepalive() {
  if (!FIVE_VOLT_BATTERY_KEEPALIVE || !POWER_SAVE) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastBatteryKeepaliveAt < BATTERY_KEEPALIVE_INTERVAL_MS) {
    return;
  }
  lastBatteryKeepaliveAt = now;

  Serial.println(F("Battery keepalive pulse"));
  WiFi.forceSleepWake();
  delay(100);
  WiFi.forceSleepBegin();
  delay(1);
  blink();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  motionState = digitalRead(PIR_PIN);

  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(WIFI_SSID);

  if (connectWiFi()) {
    blink();
    blink();
    blink();
    blink();
  }

  powerSave();
}

void loop() {
  delay(50);

  const int newState = digitalRead(PIR_PIN);
  if (newState != motionState) {
    motionState = newState;
    Serial.print(F("PIR state changed: "));
    Serial.println(motionState);

    const uint32_t now = millis();
    const bool cooldownExpired =
        !hasSentMotionEvent ||
        now - lastMotionEventAt >= MOTION_COOLDOWN_MS;

    if (motionState == PIR_ACTIVE_STATE && cooldownExpired) {
      lastMotionEventAt = now;
      hasSentMotionEvent = true;
      sendEvent();
    }
  }

  digitalWrite(LED_PIN, motionState == PIR_ACTIVE_STATE ? LOW : HIGH);
  batteryKeepalive();
}

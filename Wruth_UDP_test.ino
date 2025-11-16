#include <WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>
#define LED_TYPE APA102
#define COLOR_ORDER BGR  // You observed BGR
#define DATA_PIN 3
#define CLOCK_PIN 2
#define NUM_LEDS 150
#define BRIGHTNESS 200

CRGB leds[NUM_LEDS];

const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASSWORD";

#define UDP_PORT 19446  // Must match Prismatik
WiFiUDP udp;

uint8_t prefix[] = { 'A', 'd', 'a' };

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n===== ESP32-C6 WiFi Adalight Receiver =====");
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  connectToWiFi();
  udp.begin(UDP_PORT);
  Serial.printf("Listening for Prismatik on UDP port %d\n", UDP_PORT);

  // LED startup flash
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::White;
  FastLED.show();
  delay(400);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize <= 0) return;

  static uint8_t buffer[4096];
  int len = udp.read(buffer, sizeof(buffer));

  // Check for Adalight header "Ada"
  if (len < 6 || buffer[0] != 'A' || buffer[1] != 'd' || buffer[2] != 'a') return;

  uint8_t hi = buffer[3];
  uint8_t lo = buffer[4];
  uint8_t chk = buffer[5];

  if (chk != (hi ^ lo ^ 0x55)) return;

  int num_leds = (hi << 8) + lo + 1;
  num_leds = min(num_leds, NUM_LEDS);

  // Copy pixel data
  int dataStart = 6;
  for (int i = 0; i < num_leds; i++) {
    int idx = dataStart + i * 3;
    if (idx + 2 >= len) break;
    leds[i].r = buffer[idx + 0];
    leds[i].g = buffer[idx + 1];
    leds[i].b = buffer[idx + 2];
  }

  FastLED.show();
}
#include <WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>

// WiFi credentials
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_Password";

// LED setup
#define DATA_PIN    23
#define CLOCK_PIN   18
#define LED_TYPE    APA102
#define COLOR_ORDER BGR
#define NUM_LEDS    150
#define BRIGHTNESS  200

CRGB leds[NUM_LEDS];

// UDP setup
WiFiUDP udp;
const int UDP_PORT = 7890;

#define MAX_UDP_SIZE (NUM_LEDS * 7)
uint8_t buffer[MAX_UDP_SIZE];

// Structure to buffer LED updates and track update times
struct LedUpdate {
  CRGB color = CRGB::Black;          // default off
  unsigned long lastUpdate = 0;      // timestamp of last update
};

LedUpdate ledBuffer[NUM_LEDS];

unsigned long lastShowTime = 0;
const unsigned long SHOW_INTERVAL_MS = 10;  // reduced from 25 to 10 ms for lower latency

void setup() {
  Serial.begin(115200);
  delay(1000);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  udp.begin(UDP_PORT);

  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);
  FastLED.show();

  Serial.println("Listening for DNRGB LED packets with buffering...");
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize < 7) return;

  int len = udp.read(buffer, MAX_UDP_SIZE);
  if (len < 7) return;

  int i = 0;
  bool immediateShow = false;
  while (i + 6 < len) {
    if (buffer[i] == 0x04) {
      uint16_t ledIndex = (buffer[i + 2] << 8) | buffer[i + 3];
      if (ledIndex < NUM_LEDS) {
        float scale = buffer[i + 1] / 255.0f;
        uint8_t r = buffer[i + 4];
        uint8_t g = buffer[i + 5];
        uint8_t b = buffer[i + 6];

        CRGB newColor = CRGB(r * scale, g * scale, b * scale);

        // Check if color change is large enough to trigger immediate update
        uint8_t diff = max(abs(newColor.r - ledBuffer[ledIndex].color.r),
                      max(abs(newColor.g - ledBuffer[ledIndex].color.g),
                          abs(newColor.b - ledBuffer[ledIndex].color.b)));
        if (diff > 20) immediateShow = true;

        ledBuffer[ledIndex].color = newColor;
        ledBuffer[ledIndex].lastUpdate = millis();
      }
      i += 7;
    } else {
      i++;
    }
  }

  unsigned long now = millis();
  if (immediateShow || (now - lastShowTime > SHOW_INTERVAL_MS)) {
    // Optionally fade out LEDs not updated recently to reduce ghosting
    for (int j = 0; j < NUM_LEDS; j++) {
      if (now - ledBuffer[j].lastUpdate > (SHOW_INTERVAL_MS * 5)) {  // timeout ~50 ms
        ledBuffer[j].color.fadeToBlackBy(30);
      }
      leds[j] = ledBuffer[j].color;
    }
    FastLED.show();
    lastShowTime = now;
  }
}

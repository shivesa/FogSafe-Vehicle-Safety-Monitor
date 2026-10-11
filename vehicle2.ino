#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

// ======================================================
// WIFI
// ======================================================

const char* ssid = "ESP_DISTANCE";
const char* password = "12345678";

const char* serverIP = "192.168.4.1";

// ======================================================
// LCD
// ======================================================

hd44780_I2Cexp lcd;

// ======================================================
// PINS
// ======================================================

#define LCD_SDA D2
#define LCD_SCL D1

#define TRIG_PIN D5
#define ECHO_PIN D6

#define BUZZER_PIN D4
#define LED_PIN D7

// ======================================================
// LIMITS
// ======================================================

const float SAFE_DISTANCE = 5.0;
const float WARNING_DISTANCE = 2.0;

const float OBSTACLE_WARNING = 50.0;

// ======================================================
// RSSI CALIBRATION
// ======================================================

// Measure RSSI when ESPs are exactly 1 meter apart.

float RSSI_1M = -45.0;

// Indoor path-loss factor
float N = 2.5;

// ======================================================
// ULTRASONIC
// ======================================================

float measureDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration =
    pulseIn(
      ECHO_PIN,
      HIGH,
      30000
    );

  if (duration == 0)
    return 999.0;

  return duration * 0.0343 / 2.0;
}

// ======================================================
// WIFI CONNECT
// ======================================================

void connectWiFi()
{
  WiFi.mode(WIFI_STA);

  WiFi.begin(
    ssid,
    password
  );

  Serial.print("Connecting");

  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 30
  )
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (
    WiFi.status() ==
    WL_CONNECTED
  )
  {
    Serial.println("WiFi connected");

    Serial.print("IP: ");
    Serial.println(
      WiFi.localIP()
    );
  }
  else
  {
    Serial.println(
      "WiFi connection failed"
    );
  }
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  delay(500);

  // ====================================================
  // I2C
  // ====================================================

  Wire.begin(
    LCD_SDA,
    LCD_SCL
  );

  // ====================================================
  // LCD
  // ====================================================

  int lcdStatus =
    lcd.begin(
      16,
      2
    );

  if (lcdStatus)
  {
    Serial.print(
      "LCD Error: "
    );

    Serial.println(
      lcdStatus
    );
  }

  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("VEHICLE 2");

  lcd.setCursor(0, 1);
  lcd.print("Connecting...");

  // ====================================================
  // GPIO
  // ====================================================

  pinMode(
    TRIG_PIN,
    OUTPUT
  );

  pinMode(
    ECHO_PIN,
    INPUT
  );

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  pinMode(
    LED_PIN,
    OUTPUT
  );

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  digitalWrite(
    LED_PIN,
    LOW
  );

  // ====================================================
  // WIFI
  // ====================================================

  connectWiFi();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");

  delay(1000);
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
  // ====================================================
  // CHECK WIFI
  // ====================================================

  if (
    WiFi.status() !=
    WL_CONNECTED
  )
  {
    digitalWrite(
      LED_PIN,
      LOW
    );

    digitalWrite(
      BUZZER_PIN,
      LOW
    );

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("WiFi Lost");

    connectWiFi();

    delay(1000);

    return;
  }

  // ====================================================
  // RSSI
  // ====================================================

  int rssi =
    WiFi.RSSI();

  // ====================================================
  // RSSI DISTANCE
  // ====================================================

  float espDistance =
    pow(
      10.0,
      (RSSI_1M - rssi) /
      (10.0 * N)
    );

  // ====================================================
  // ULTRASONIC
  // ====================================================

  float obstacle =
    measureDistance();

  // ====================================================
  // LCD
  // ====================================================

  lcd.clear();

  lcd.setCursor(0, 0);

  lcd.print("D:");
  lcd.print(
    espDistance,
    1
  );
  lcd.print("m");

  lcd.setCursor(0, 1);

  lcd.print("OBS:");
  lcd.print(
    obstacle,
    0
  );
  lcd.print("cm");

  // ====================================================
  // STATUS
  // ====================================================

  bool danger = false;
  bool warning = false;

  if (
    obstacle <=
    OBSTACLE_WARNING
  )
  {
    danger = true;
  }

  else if (
    espDistance > 0 &&
    espDistance < WARNING_DISTANCE
  )
  {
    danger = true;
  }

  else if (
    espDistance >= WARNING_DISTANCE &&
    espDistance <= SAFE_DISTANCE
  )
  {
    warning = true;
  }

  // ====================================================
  // LED + BUZZER
  // ====================================================

  if (danger)
  {
    digitalWrite(
      LED_PIN,
      HIGH
    );

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );
  }

  else if (warning)
  {
    digitalWrite(
      LED_PIN,
      HIGH
    );

    digitalWrite(
      BUZZER_PIN,
      LOW
    );
  }

  else
  {
    digitalWrite(
      LED_PIN,
      LOW
    );

    digitalWrite(
      BUZZER_PIN,
      LOW
    );
  }

  // ====================================================
  // SEND TO ESP8266 #1
  // ====================================================

  WiFiClient client;

  HTTPClient http;

  String url =
    "http://192.168.4.1/data?";

  url +=
    "distance=";

  url +=
    String(
      espDistance,
      2
    );

  url +=
    "&obstacle=";

  url +=
    String(
      obstacle,
      1
    );

  url +=
    "&rssi=";

  url +=
    String(
      rssi
    );

  Serial.println(
    "-------------------------"
  );

  Serial.print(
    "RSSI: "
  );

  Serial.print(
    rssi
  );

  Serial.println(
    " dBm"
  );

  Serial.print(
    "Distance: "
  );

  Serial.print(
    espDistance,
    2
  );

  Serial.println(
    " m"
  );

  Serial.print(
    "Obstacle: "
  );

  Serial.print(
    obstacle,
    1
  );

  Serial.println(
    " cm"
  );

  if (
    http.begin(
      client,
      url
    )
  )
  {
    int httpCode =
      http.GET();

    Serial.print(
      "HTTP Code: "
    );

    Serial.println(
      httpCode
    );

    http.end();
  }

  delay(500);
}
/*
  Temperature Reading & LCD Display System
  ==========================================
  Hardware : Arduino UNO + DHT11 sensor + 16x2 I2C LCD (PCF8574 backpack)

  Behaviour:
   - Row 1 (top)    : Candidate name. Scrolls horizontally if longer than 16 chars.
   - Row 2 (bottom) : Live temperature reading.
   - Every reading is sent over USB serial as: CANDIDATE_NAME,TEMPERATURE\n
     so the PC-side program can pick it up and publish it to MQTT.

  Required libraries (Arduino IDE -> Tools -> Manage Libraries):
   - "DHT sensor library" by Adafruit   (pulls in "Adafruit Unified Sensor" too)
   - "LiquidCrystal I2C" by Frank de Brabander

  If your LCD is a plain parallel 16x2 (no I2C backpack), this sketch needs
  a small change (use the LiquidCrystal library with 6 data pins instead) -
  ask and I'll adapt it.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ------------------------- USER CONFIG -------------------------
const char* CANDIDATE_NAME = "IMPANO Blessed Winner";  // <-- set this (avoid commas)
#define LCD_I2C_ADDRESS 0x27       // common addresses: 0x27 or 0x3F. Run an I2C
                                    // scanner sketch if unsure which one you have.
#define DHTPIN   2                  // DHT11 DATA pin
#define DHTTYPE  DHT11

const unsigned long TEMP_READ_INTERVAL = 2000;  // ms - DHT11 needs >=1s between reads
const unsigned long SCROLL_INTERVAL    = 400;    // ms between each scroll step
// ------------------------------------------------------------------

LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

String scrollBuffer;          // padded name string used for the scroll loop
bool   nameNeedsScroll = false;
int    scrollIndex = 0;
unsigned long lastScrollTime   = 0;
unsigned long lastTempReadTime = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();

  lcd.init();
  lcd.backlight();

  String name = String(CANDIDATE_NAME);
  if (name.length() > 16) {
    nameNeedsScroll = true;
    scrollBuffer = name + "    ";   // trailing spaces make the wrap-around read cleanly
  } else {
    nameNeedsScroll = false;
    lcd.setCursor(0, 0);
    lcd.print(name);
  }

  lcd.setCursor(0, 1);
  lcd.print("Reading...");
}

void loop() {
  unsigned long now = millis();

  // ---------- Non-blocking name scroll ----------
  if (nameNeedsScroll && (now - lastScrollTime >= SCROLL_INTERVAL)) {
    lastScrollTime = now;
    String window = scrollBuffer.substring(scrollIndex) + scrollBuffer.substring(0, scrollIndex);
    lcd.setCursor(0, 0);
    lcd.print(window.substring(0, 16));
    scrollIndex++;
    if (scrollIndex >= (int)scrollBuffer.length()) scrollIndex = 0;
  }

  // ---------- Non-blocking temperature read ----------
  if (now - lastTempReadTime >= TEMP_READ_INTERVAL) {
    lastTempReadTime = now;
    float temperature = dht.readTemperature();  // Celsius

    if (isnan(temperature)) {
      lcd.setCursor(0, 1);
      lcd.print("Sensor error    ");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print((char)223);   // degree symbol (works on most HD44780 ROMs)
      lcd.print("C   ");      // trailing spaces clear leftover digits from previous value

      // Send to PC: "CANDIDATE_NAME,TEMP"
      Serial.print(CANDIDATE_NAME);
      Serial.print(",");
      Serial.println(temperature, 2);
    }
  }
}

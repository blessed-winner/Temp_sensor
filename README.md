# Temperature Reading & LCD Display System

## System flow
DHT11 sensor -> Arduino UNO -> 16x2 I2C LCD (display) + USB Serial -> PC Python program -> MQTT broker (VPS) + real-time console display

## Project diagram

```
[DHT11 Sensor]     [16x2 I2C LCD]
      |                 ^
      v                 |
   [Arduino UNO] -- USB Serial --> [PC Python Script] -- MQTT --> [VPS MQTT Broker]
                                      |
                                      v
                            [Console / Live display]
```

## Hardware wiring

**DHT11 sensor**
| DHT11 pin | Arduino UNO pin |
|---|---|
| VCC | 5V |
| GND | GND |
| DATA | D2 |

If your DHT11 is a bare 3-pin sensor (not a breakout board), add a 10kΩ pull-up resistor between DATA and VCC. Most 3-pin breakout *modules* already have this resistor built in, so you can skip it.

**16x2 I2C LCD (PCF8574 backpack)**
| LCD pin | Arduino UNO pin |
|---|---|
| VCC | 5V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

If you actually have a plain parallel 16x2 LCD (16 pins, no I2C backpack) rather than an I2C one, tell me and I'll adapt the sketch — wiring uses 6 data/control pins instead of 2, and the code swaps `LiquidCrystal_I2C` for `LiquidCrystal`.

## Software setup

### Arduino
1. Install the Arduino IDE.
2. Library Manager: install **"DHT sensor library"** by Adafruit (it pulls in "Adafruit Unified Sensor" as a dependency).
3. Library Manager: install **"LiquidCrystal I2C"** by Frank de Brabander.
4. Open `temperature_lcd_display.ino`. Set `CANDIDATE_NAME` (avoid commas in the name) and `LCD_I2C_ADDRESS` — most backpacks are `0x27` or `0x3F`. If your LCD shows nothing/garbage, run an I2C scanner sketch (search "Arduino I2C scanner") to find the correct address.
5. Select Board: Arduino Uno, pick the correct Port, Upload.
6. Adjust the small potentiometer on the I2C backpack if the LCD text isn't visible (contrast).

### PC
1. Python 3.8+.
2. `pip install -r requirements.txt`
3. Edit the config block at the top of `pc_mqtt_publisher.py`:
   - `SERIAL_PORT` — the Arduino's serial port (check Arduino IDE's Tools > Port, or Device Manager on Windows / `ls /dev/tty*` on Linux/macOS)
   - `MQTT_BROKER_HOST`, `MQTT_BROKER_PORT` — your VPS broker address
   - `MQTT_USERNAME` / `MQTT_PASSWORD` if your broker requires auth
4. Make sure an MQTT broker (e.g. Mosquitto) is running on the VPS, and that the firewall allows the port you're using (1883 plaintext, or 8883 if TLS).
5. Run: `python pc_mqtt_publisher.py`

Note: close the Arduino IDE's Serial Monitor before running the Python script — only one program can hold the serial port open at a time.

## Protocol

**Arduino -> PC (serial):** one line per reading, every ~2 seconds:
```
CANDIDATE_NAME,TEMPERATURE
```
e.g. `JEAN BAPTISTE MUGISHA,23.50`

**PC -> MQTT broker:** JSON payload published to `exam/temperature/<candidate_name_slug>`:
```json
{"candidate": "JEAN BAPTISTE MUGISHA", "temperature": 23.50, "timestamp": "2026-06-16T10:42:11.123456+00:00"}
```

## Troubleshooting
- **LCD blank/garbled** — wrong I2C address (scan for it) or contrast pot needs adjusting.
- **DHT11 always returns an error** — check wiring/polarity, make sure reads are spaced ≥2s apart, try a different sensor (DHT11 units fail somewhat often).
- **Python can't open the serial port** — close any other program (Serial Monitor, other scripts) using it.
- **MQTT won't connect** — verify the VPS firewall allows the port, the broker is actually running, and credentials are correct. `telnet <vps-ip> 1883` is a quick reachability check.

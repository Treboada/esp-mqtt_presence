/**
 * HC-SR501 presence detector with ESP8266 (Wemos D1 mini with ESP-12E)
 *
 * @see https://github.com/Treboada/esp-mqtt_presence
 * @author Rafa Couto <caligari@treboada.net>
 */

#include "Arduino.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define PIN_LED 2 // gpio02 (D4)
#define PIN_PIR 4 // gpio04 (D2)

#define CALIBRATION_SECONDS 60 // according to datasheet
#define ALARM_SECONDS 3 // time to alarm the presence with the led

#define WIFI_SSID "RecunchoMaker"
#define WIFI_PASS "........"

#define MQTT_TOPIC "recuncho/caramonina/sensors/presence/01"

WiFiClient wclient;
IPAddress server(10, 27, 0, 103);
PubSubClient client(server, 1883, wclient);

void mqtt_publish(bool presence) {

    bool mqtt_ok = false;

    if (client.connected()) {

        // already connected
        mqtt_ok = true;

    } else {

        // try connection
        if (client.connect("ESP-170052")) {
            mqtt_ok = true;
        }
    }

    if (mqtt_ok) {

        // publish the event
        client.publish(MQTT_TOPIC, presence ? "1" : "0");
    }
}

void setup() {

    pinMode(PIN_LED, OUTPUT_OPEN_DRAIN);
    pinMode(PIN_PIR, INPUT);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void loop() {

    static unsigned long seconds = 0;
    static unsigned long next_now = 1000;
    unsigned long partial;

    bool connected = (WiFi.status() == WL_CONNECTED);

    unsigned long now = millis();
    if (now >= next_now) {
        next_now = now + 1000;
        seconds++;
    }

    if (seconds < CALIBRATION_SECONDS) {

        // fast blink while in calibration time
        partial = now % 250;
        digitalWrite(PIN_LED, partial < 125 ? LOW : HIGH);
    }
    else {

        static unsigned int triggered = 0;
        bool led_on;

        if (triggered > seconds) {

            // alarm on
            led_on = true;

        } else {

            // alarm off
            led_on = false;

            // little blink (20ms/5s) indicating the wifi connection
            if (connected) {
                partial = now % 5000;
                if (partial < 20) led_on = true;
            }
        }

        digitalWrite(PIN_LED, led_on ? LOW : HIGH); // led is low-activated

        // read the sensor
        bool presence = (digitalRead(PIN_PIR) == HIGH);
        static bool published = false;
        if (presence) {

            // set the alarm on
            triggered = seconds + ALARM_SECONDS;
        }

        // send the presence changes as MQTT events
        if (connected && presence ^ published) {

            mqtt_publish(presence);
            published = !published;
        }
    }

    // wake up every 1/40 second
    delay(25);

}


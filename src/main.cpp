/**
 * HC-SR501 presence detector with ESP8266 (Wemos D1 mini with ESP-12E)
 *
 * @see https://github.com/Treboada/esp-mqtt_presence
 * @author Rafa Couto <caligari@treboada.net>
 */

#include "Arduino.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define PIN_LED 2 // gpio02 (D4)
#define PIN_PIR 4 // gpio04 (D2)

#define CALIBRATION_SECONDS 60 // according to datasheet
#define ALARM_SECONDS 3 // time to alarm the presence with the led

#define MQTT_SERVER "mqtt.vpn.recunchomaker.org"
#define MQTT_PORT 1883
#define MQTT_TOPIC "recuncho/caramonina/sensors/presence/01"

WiFiClient wclient;
IPAddress server(10, 27, 0, 103);
PubSubClient client(server, MQTT_PORT, wclient);
//PubSubClient client(MQTT_SERVER, MQTT_PORT, wclient);

static char mqtt_client_name[] = "esp-123456";

void setup_mqtt_client_name() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    sprintf(mqtt_client_name, "esp-%02x%02x%02x", mac[3], mac[4], mac[5]);
}

void mqtt_publish(bool presence) {

    bool mqtt_ok = false;

    if (client.connected()) {

        // already connected
        mqtt_ok = true;

    } else {

        // try connection
        if (client.connect(mqtt_client_name)) {
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

    setup_mqtt_client_name();
}

void loop() {

    bool led_on;
    bool connected = (WiFi.status() == WL_CONNECTED);

    // time managing
    static unsigned long seconds = 0;
    static unsigned long next_now = 1000;
    unsigned long partial;
    unsigned long now = millis();
    if (now >= next_now) {
        next_now = now + 1000;
        seconds++;
    }

    // calibration
    if (seconds < CALIBRATION_SECONDS) {

        // fast blink while in calibration time
        partial = now % 250;
        led_on = (partial < 125);
    }
    else {

        // LED indications
        static unsigned int triggered = 0;
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

        // read the sensor
        bool presence = (digitalRead(PIN_PIR) == HIGH);
        if (presence) {

            // set the alarm on
            triggered = seconds + ALARM_SECONDS;
        }

        // send the presence changes as MQTT events
        static bool published = false;
        if (connected && presence ^ published) {

            mqtt_publish(presence);
            published = !published;
        }
    }

    // LED signal
    digitalWrite(PIN_LED, led_on ? LOW : HIGH); // led is low-activated

    // wake up every 1/40 second
    delay(25);

}


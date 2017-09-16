/**
 * HC-SR501 presence detector with ESP8266 (Wemos D1 mini with ESP-12E)
 *
 * @see https://github.com/Treboada/esp-mqtt_presence
 * @author Rafa Couto <caligari@treboada.net>
 */

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "AsyncBlinker.h"

#define PIN_LED 2 // gpio02 (D4)
#define PIN_PIR 4 // gpio04 (D2)

#define CALIBRATION_SECONDS 60 // according to datasheet
#define ALARM_SECONDS 3 // time to alarm the presence with the led

// state machine

#define MQTT_SERVER "mqtt.vpn.recunchomaker.org"
#define MQTT_PORT 1883
#define MQTT_TOPIC "recuncho/caramonina/sensors/presence/01"

enum State : uint8_t {
    STATE_NONE,
    STATE_CALIBRATING,
    STATE_SCANNING_WIFI_ON,
    STATE_SCANNING_WIFI_OFF,
    STATE_ALARMED
};

WiFiClient wclient;
IPAddress server(10, 27, 0, 103);
PubSubClient client(server, MQTT_PORT, wclient);
//PubSubClient client(MQTT_SERVER, MQTT_PORT, wclient);

const uint16_t calibration_blink[] = { 125, 125 };
const uint16_t wifi_on_blink[] = { 20, 5000 };
const uint16_t presence_blink[] = { ALARM_SECONDS * 1000, 0 };

void blink_led(bool enable) {
    // led is low-activated125:
    digitalWrite(PIN_LED, enable ? LOW : HIGH);
}

AsyncBlinker blinker(blink_led);

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

    blinker.setIntervals(calibration_blink, sizeof(calibration_blink));
    blinker.start();
}

static State state = STATE_NONE;
static unsigned long system_seconds = 0;
static unsigned long alarm_second;

void state_changed(State from, State to) {

    blinker.stop();

    switch (to) {

        case STATE_CALIBRATING:
            blinker.setIntervals(calibration_blink, sizeof(calibration_blink));
            break;

        case STATE_SCANNING_WIFI_OFF:
            if (from == STATE_ALARMED) {
                mqtt_publish(0);
            }
            break;

        case STATE_SCANNING_WIFI_ON:
            blinker.setIntervals(wifi_on_blink, sizeof(wifi_on_blink));
            break;

        case STATE_ALARMED:
            alarm_second = system_seconds;
            blinker.setIntervals(presence_blink, sizeof(presence_blink));
            if (from == STATE_SCANNING_WIFI_ON) {
                mqtt_publish(1);
            }
            break;
    }

    blinker.start();
}

void loop() {

    // time managing
    static unsigned long last_millis = 0;
    unsigned long now_millis = millis();
    unsigned long elapsed_millis = now_millis - last_millis;
    if (now_millis / 1000 != system_seconds) {
        system_seconds++;
    }
    last_millis = now_millis;

    // asyncronous updates
    blinker.tickUpdate(elapsed_millis);

    // state machine changer
    static State last_state = STATE_NONE;
    if (last_state != state) {
        state_changed(last_state, state);
        last_state = state;
    }

    // state machine business
    switch (state) {

        case STATE_NONE:
            state = STATE_CALIBRATING;
            break;

        case STATE_CALIBRATING:
            if (system_seconds >= CALIBRATION_SECONDS) {
                state = STATE_SCANNING_WIFI_OFF;
            }
            break;

        case STATE_SCANNING_WIFI_ON:
        case STATE_SCANNING_WIFI_OFF:
            if (digitalRead(PIN_PIR) == HIGH) {
                state = STATE_ALARMED;
            }
            else {
                bool connected = (WiFi.status() == WL_CONNECTED);
                if (connected && state == STATE_SCANNING_WIFI_OFF) {
                    state = STATE_SCANNING_WIFI_ON;
                }
                else if (!connected && state == STATE_SCANNING_WIFI_ON) {
                    state = STATE_SCANNING_WIFI_OFF;
                }
            }
            break;

        case STATE_ALARMED:
            if (system_seconds - alarm_second > ALARM_SECONDS) {
                state = STATE_SCANNING_WIFI_OFF;
            }
            break;
    }


    // wake up every 1/40 second
    delay(25);

}


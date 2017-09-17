/**
 * HC-SR501 presence detector with ESP8266 (Wemos D1 mini with ESP-12E)
 *
 * @see https://github.com/Treboada/esp-mqtt_presence
 * @author Rafa Couto <caligari@treboada.net>
 */

////////// config

#define PIN_LED 2 // gpio02 (D4)
#define PIN_PIR 4 // gpio04 (D2)

#define CALIBRATION_SECONDS 60 // according to datasheet
#define ALARM_SECONDS 60 // time to alarm the presence

#define MQTT_SERVER "mqtt.vpn.recunchomaker.org"
#define MQTT_PORT 1883
#define MQTT_TOPIC "recuncho/caramonina/sensors/presence/01"

////////// libraries

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <AsyncBlinker.h>

////////// wifi connection

WiFiClient wifi_client;
static bool connected;

////////// led blinker

const uint16_t calibration_blink[] = { 125, 125 };
const uint16_t wifi_on_blink[] = { 10, 3000 };

void set_led(bool enable) {
    digitalWrite(PIN_LED, enable ? LOW : HIGH); // led is low-activated
}

AsyncBlinker blinker(set_led);

////////// mqtt

static char mqtt_client_name[] = "esp-123456";
IPAddress mqtt_server(10, 27, 0, 103);
PubSubClient mqtt_client(mqtt_server, MQTT_PORT, wifi_client);
//PubSubClient mqtt_client(MQTT_SERVER, MQTT_PORT, wifi_client);

void setup_mqtt_client_name() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    sprintf(mqtt_client_name, "esp-%02x%02x%02x", mac[3], mac[4], mac[5]);
}

void mqtt_publish(bool presence) {

    bool mqtt_ok = false;

    if (mqtt_client.connected()) {

        // already connected
        mqtt_ok = true;

    } else {

        // try connection
        if (mqtt_client.connect(mqtt_client_name)) {
            mqtt_ok = true;
        }
    }

    if (mqtt_ok) {

        // publish the event
        mqtt_client.publish(MQTT_TOPIC, presence ? "1" : "0");
    }
}

//////////

enum State : uint8_t {
    STATE_NONE,
    STATE_CALIBRATING,
    STATE_SCANNING_WIFI_ON,
    STATE_SCANNING_WIFI_OFF,
    STATE_ALARMED
};

static State state = STATE_NONE;
static unsigned long system_seconds = 0;
static unsigned long alarm_second;

void state_changed(State from, State to) {

    switch (to) {
        case STATE_CALIBRATING:
            blinker.setIntervals(calibration_blink, sizeof(calibration_blink) / 2);
            blinker.start();
            break;
        case STATE_SCANNING_WIFI_ON:
            blinker.setIntervals(wifi_on_blink, sizeof(wifi_on_blink) / 2);
            blinker.start();
            break;
        default:
            blinker.stop();
    }

    if (from == STATE_ALARMED || to == STATE_ALARMED) {
        bool alarm_on = (to == STATE_ALARMED);
        set_led(alarm_on);
        mqtt_publish(alarm_on);
    }
}

//////////

void setup() {

    pinMode(PIN_LED, OUTPUT_OPEN_DRAIN);
    pinMode(PIN_PIR, INPUT);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    setup_mqtt_client_name();
}

//////////

void loop() {

    // time managing
    static uint32_t last_millis = 0;
    uint32_t now_millis = millis();
    uint32_t elapsed_millis = now_millis - last_millis;
    if ((last_millis / 1000) != (now_millis / 1000)) system_seconds++;
    last_millis = now_millis;

    // asyncronous updates
    blinker.tickUpdate(elapsed_millis);
    connected = (WiFi.status() == WL_CONNECTED);

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
                if (connected && state == STATE_SCANNING_WIFI_OFF) {
                    state = STATE_SCANNING_WIFI_ON;
                }
                else if (!connected && state == STATE_SCANNING_WIFI_ON) {
                    state = STATE_SCANNING_WIFI_OFF;
                }
            }
            break;

        case STATE_ALARMED:
            if (alarm_second != system_seconds) {
                if (digitalRead(PIN_PIR) == HIGH) {
                    alarm_second = system_seconds;
                }
                else if (system_seconds - alarm_second > ALARM_SECONDS) {
                    state = STATE_SCANNING_WIFI_OFF;
                }
            }
            break;
    }

    // wake up every 1/100 second
    delay(10);
}


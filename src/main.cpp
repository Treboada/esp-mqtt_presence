/**
 * HC-SR501 presence detector with ESP8266 (Wemos D1 mini with ESP-12E)
 *
 * @see https://github.com/Treboada/esp-mqtt_presence
 * @author Rafa Couto <caligari@treboada.net>
 */

#include "Arduino.h"

#define PIN_LED 2 // gpio02 (D4)
#define PIN_PIR 4 // gpio04 (D2)

#define CALIBRATION_SECONDS 60 // according to datasheet
#define ALARM_SECONDS 5 // time to alarm the presence with the led

void setup() {

    pinMode(PIN_LED, OUTPUT_OPEN_DRAIN);
    pinMode(PIN_PIR, INPUT);
}

void loop() {

    static unsigned long seconds = 0;
    static unsigned long next_now = 1000;

    unsigned long now = millis();
    if (now >= next_now) {
        next_now = now + 1000;
        seconds++;
    }

    if (seconds < CALIBRATION_SECONDS) {

        // fast blink while in calibration time
        unsigned long half = now % 250;
        digitalWrite(PIN_LED, half < 125 ? LOW : HIGH);
    }
    else {

        static unsigned int triggered = 0;

        if (triggered > seconds) {

            // alarm on
            digitalWrite(PIN_LED, LOW); // led is low-activated
        }
        else {

            // alarm off
            digitalWrite(PIN_LED, HIGH); // led is low-activated
        }

        // read the sensor
        bool presence = (digitalRead(PIN_PIR) == HIGH);
        if (presence) {

            // set the alarm on
            triggered = seconds + ALARM_SECONDS;
        }
    }

    // wake up every 1/40 second
    delay(25);
}


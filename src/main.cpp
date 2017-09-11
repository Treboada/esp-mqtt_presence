
#include "Arduino.h"

/**
 * HC-SR501 presence detector with ESP8266 (Wemos D1 mini with ESP-12E)
 * @author Rafa Couto <caligari@treboada.net>
 */

#define PIN_LED 2 // gpio02 (D4)
#define PIN_PIR 4 // gpio04 (D2)

#define CALIBRATION_SECONDS 60 // according to datasheet
#define ALARM_SECONDS 5 // time to alarm the presence with the led

void setup() {

    pinMode(PIN_LED, OUTPUT_OPEN_DRAIN);
    pinMode(PIN_PIR, INPUT);
}

void loop() {

    unsigned long now = millis();
    int seconds = now / 1000;
    static int last_second = 0;

    if (seconds < CALIBRATION_SECONDS) {

        // calibration time (1 minute)
        unsigned long half = now % 250;
        digitalWrite(PIN_LED, half < 125 ? LOW : HIGH);
    }
    else {

        static int triggered = 0;

        if (triggered > seconds) {

            // alarm is on
            digitalWrite(PIN_LED, LOW); // led is low-activated
        }
        else {

            // alarm is off
            digitalWrite(PIN_LED, HIGH); // led is low-activated
        }

        bool presence = (digitalRead(PIN_PIR) == HIGH);
        if (presence) {

            triggered = seconds + ALARM_SECONDS;
        }
    }

    // wake up every 1/40 second
    delay(25);
}


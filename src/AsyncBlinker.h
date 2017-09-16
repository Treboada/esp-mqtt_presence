/**
 * @file AsyncBlinker.h
 * @author Rafa Couto <caligari@treboada.net>
 * @see https://github.com/Treboada/AsyncBlinker
 */

#ifndef _ASYNC_BLINKER_H
#define _ASYNC_BLINKER_H

#include <stdint.h>

// blinks each second (500ms ON and 500ms OFF):
static const uint16_t _default_blink[2] = { 500, 500 };

/**
 * This class manages a blinking led (or any switchable device) changing the
 * light in ON-OFF time intervals.
 *
 * You are the responsible of coding your switching function:
 *
 *     void blink_my_led(bool enable) {
 *         digitalWrite(LED_BUILTIN, enable ? HIGH : LOW);
 *     }
 *
 * Declare an interval of 2 numbers representing the time in milliseconds to
 * keep ON and OFF. Example:
 *
 *     // fast blink (200ms ON and 200ms OFF):
 *     uint16_t fast_blink[] = { 200, 200 };
 *
 * Define a blinker instance, set the intervals and start it:
 *
 *     AsyncBlinker blinker(blink_my_led);
 *     blinker.setIntervals(fast_blink, sizeof(fast_blink));
 *     blinker.start();
 *
 * IMPORTANT: you must call tickUpdate(elapsed_time) in your main loop.
 * Example for Arduino framework:
 *
 *    void setup() {
 *
 *        blinker.start();
 *    }
 *
 *    void loop() {
 *
 *        static unsigned long last_millis = 0;
 *        unsigned long now = millis();
 *
 *        blinker.tickUpdate(now - last_millis);
 *
 *        last_millis = now;
 *    }
 *
 * With 3 or more intervals, the first interval is the milliseconds to keep
 * ON, second interval is to keep OFF, third one to keep OFF, fourth to keep
 * ON and so on... First interval is always ON. Example:
 *
 *     #define PIN_BUZZER 10
 *
 *     void morse_tone(bool enable) {
 *         if (enable) tone(PIN_BUZZER, 8000) else noTone(PIN_BUZZER);
 *     }
 *
 *     // milliseconds of morse marks:
 *     #define DOT 250
 *     #define DASH (DOT * 3)
 *     #define SEP DOT
 *     #define CHAR_SEP DASH
 *     #define WORD_SEP (DOT * 7)
 *
 *     const uint16_t morse_sos[] = {
 *         DOT, SEP, DOT, SEP, DOT, CHAR_SEP,     // S     (0-5)
 *         DASH, SEP, DASH, SEP, DASH, CHAR_SEP,  // O     (6-11)
 *         DOT, SEP, DOT, SEP, DOT, CHAR_SEP,     // S     (12-17)
 *         0, WORD_SEP                            // blank (18-19)
 *     };
 *
 *     AsyncBlinker sos_blinker(morse_tone, morse_sos, sizeof(morse_sos);
 *     sos_blinker.start();
 *
 * By default it cycles intervals endlessly from the first to the last. You
 * can limit to a number of cycles (1-254):
 *
 *     sos_blinker.start(5); // blink "SOS" word by 5 times
 *
 * If you want to cycle between specific intervals, pass 'from' and 'to'
 * interval indexes as parameters:
 *
 *     sos_blinker.start(AsyncBlinker::ENDLESSLY, 0, 11); // "SOSOSOSOSO..."
 *
 */
class AsyncBlinker
{
    public:

        typedef void (*Callback)(bool enable);
        static const int ENDLESSLY = UINT8_MAX;

        AsyncBlinker(Callback cb);

        void setIntervals(const uint16_t* intervals, uint8_t count);

        void setCallback(Callback cb)
            { _callback = cb; }

        int start(uint8_t cycles, uint8_t from, uint8_t to);

        int start(uint8_t cycles)
            { start(cycles, 0, _count - 1); }

        int start()
            { start(ENDLESSLY); }

        void stop()
            { _cycles = 0; }

        bool tickUpdate(uint32_t elapsed_millis);

    private:

        const uint16_t* _intervals;
        uint8_t _count;

        Callback _callback;

        uint8_t _from;
        uint8_t _to;
        uint8_t _current;

        uint32_t _past_millis;
        uint8_t _cycles;

        bool _nextInterval();
};

#endif // _ASYNC_BLINKER_H


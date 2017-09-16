/**
 * @file AsyncBlinker.cpp
 * @author Rafa Couto <caligari@treboada.net>
 * @see https://github.com/Treboada/AsyncBlinker
 */

#include "AsyncBlinker.h"

AsyncBlinker::AsyncBlinker(Callback cb)
{
    _callback = cb;
    setIntervals(_default_blink, sizeof(_default_blink));
}

void AsyncBlinker::setIntervals(const uint16_t* intervals, uint8_t count)
{
    _intervals = intervals;
    _count = count;
    _cycles = 0;
}

int AsyncBlinker::start(uint8_t cycles, uint8_t from, uint8_t to)
{
    _cycles = cycles;
    _from = from;
    _to = to;

    _current = _from;
    _past_millis = 0;

    if (_callback && cycles > 0 && _intervals[from] > 0) _callback(true);
}

bool AsyncBlinker::tickUpdate(uint32_t elapsed_millis)
{
    if (_cycles == 0) return false;

    _past_millis += elapsed_millis;
    while (_past_millis >= _intervals[_current]) {

        if (!_nextInterval()) return false;

        _past_millis -= _intervals[_current];
    }

    return true;
}

bool AsyncBlinker::_nextInterval()
{
    if (++_current > _to) {

        if (_cycles != ENDLESSLY && --_cycles == 0) return false;

        _current = _from;
    }

    if (_callback && _intervals[_current] > 0) {

        _callback(_current % 2 == 0 ? true : false);
    }

    return true;
}



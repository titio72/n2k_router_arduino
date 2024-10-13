#ifdef ESP32_ARCH
#include <Arduino.h>
#include "ArduinoPort.h"
#include "Log.h"

ArduinoPort::ArduinoPort(HardwareSerial& s, int rx, int tx, bool _invert): serial(s), rx_pin(rx), tx_pin(tx), open(false), invert(_invert)
{
}

ArduinoPort::ArduinoPort(HardwareSerial& s, unsigned int bps, int rx, int tx, bool _invert): serial(s), rx_pin(rx), tx_pin(tx), open(false), invert(_invert)
{
    speed = bps;
}

ArduinoPort::~ArduinoPort()
{
    serial.end();
}

void ArduinoPort::_open()
{
    Log::tracex("PORT", "Opening serial", "speed {%d BPS} RX {%d} TX {%d} invert {%d}", speed, rx_pin, tx_pin, invert);
    serial.begin(speed, SERIAL_8N1, rx_pin, tx_pin, invert);
    open = true;
    Log::tracex("PORT", "Serial opened", "speed {%d BPS}", speed);
}

void ArduinoPort::_close()
{
    Log::tracex("PORT", "Closing serial port");
    serial.end();
    open = false;
    Log::tracex("PORT", "Serial port closed");
}

int ArduinoPort::_read(bool &nothing_to_read, bool &error)
{
    error = false;
    if (serial.available()>0)
    {
        return serial.read();
    }
    else
    {
        nothing_to_read = true;
        return -1;
    }
}

bool ArduinoPort::is_open()
{
    return open;
}
#endif
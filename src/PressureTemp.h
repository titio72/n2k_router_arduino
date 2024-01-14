#ifndef _PressureTemp_H
#define _PressureTemp_H

#include <stdlib.h>
#include "Context.h"

class Adafruit_BMP280;

class PressureTemp
{
public:
    PressureTemp(Context ctx);
    ~PressureTemp();

    AB_AGENT

private:
    bool enabled;
    Context ctx;
    Adafruit_BMP280 *bmp;
    unsigned long last_read;

    void read_pressure(unsigned long ms);
};






#endif
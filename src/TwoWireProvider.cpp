#include "TwoWireProvider.h"
#include "Log.h"
#include "Constants.h"

TwoWire ic2 = TwoWire(0);
bool active = false;

TwoWire& TwoWireProvider::get_two_wire()
{
    if (!active)
    {
        active = ic2.begin(I2C_SDA, I2C_SCL);
        Log::trace("[I2C] TwoWires active {%d}\n", active);
    }
    return ic2;
}

bool TwoWireProvider::is_active()
{
    return active;
}
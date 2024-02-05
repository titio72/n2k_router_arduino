#include "TwoWireProvider.h"
#include "Log.h"
#include "Constants.h"

TwoWire* TwoWireProvider::get_two_wire()
{
    Log::trace("[I2C] Initializing TwoWires...\n");
    static TwoWire* ic2 = NULL;
    static bool active = false;
    if (!active)
    {
        Log::trace("[I2C] Initializing TwoWires\n");
        ic2 = &Wire;
        active = ic2->begin(I2C_SDA, I2C_SCL);
        Log::trace("[I2C] TwoWires active {%d}\n", active);

        int error, address;
        int nDevices;

        Log::trace("[I2C] Scanning...\n");

        nDevices = 0;
        for(address = 1; address < 127; address++ )
        {
            // The i2c_scanner uses the return value of
            // the Write.endTransmisstion to see if
            // a device did acknowledge to the address.
            Wire.beginTransmission(address);
            error = Wire.endTransmission();

            if (error == 0)
            {
                Log::trace("I2C device found at address 0x");
                if (address<16)
                    Log::trace("0");
                Log::trace("%x", address);
                Log::trace("  !\n");

                nDevices++;
            }
            else if (error==4)
            {
                Log::trace("Unknown error at address 0x");
                if (address<16)
                    Log::trace("0");
                Log::trace("%x", address);
            }
        }
        Log::trace("[I2C] Scan done\n");
    }
    return ic2;

}
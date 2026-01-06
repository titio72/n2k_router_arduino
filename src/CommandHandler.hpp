#ifndef _COMMAND_HANDLER_HPP
#define _COMMAND_HANDLER_HPP
#include <Log.h>
#include "Conf.h"
#include "Context.h"
#include "Tachometer.h"

static const char* CMD_LOG_TAG = "CMD";

class CommandHandler
{
public:
    static void on_command(char command, const char *command_value, Configuration &conf, EngineHours &engineHours, Data &data)
    {
        switch (command)
        {
        case 'S':
        {
            Log::tracex(CMD_LOG_TAG, "Command switch services", "S {%s}", command_value);
            N2KServices c = conf.get_services();
            c.from_string(command_value);
            conf.save_services(c);
        }
        break;
        case 'N':
        {
            Log::tracex(CMD_LOG_TAG, "Command set device name", "N {%s}", command_value);
            conf.save_device_name(command_value);
        }
        break;
        case 'C':
        {
            Log::tracex(CMD_LOG_TAG, "Command set services", "C {%s}", command_value);
            N2KServices s;
            s.from_string(command_value);
            conf.save_services(s);
        }
        break;
        case 'H':
        {
            Log::tracex(CMD_LOG_TAG, "Command set engine time", "H {%s}", command_value);
            int64_t engine_time_secs = atol(command_value);
            if (engine_time_secs > 0)
            {
                uint64_t new_t = (uint64_t)1000 * engine_time_secs; // convert in milliseconds
                Log::tracex(CMD_LOG_TAG, "Command set engine time", "ms {%lu-%03d}", (uint32_t)(new_t / 1000), (uint16_t)(new_t % 1000));
                engineHours.save_engine_hours(new_t);
                data.engine.engine_time = new_t;
            }
        }
        break;
        case 'T':
        {
            Log::tracex(CMD_LOG_TAG, "Command tachometer calibration", "T {%s}", command_value);
            int rpm = atoi(command_value);
            if (rpm > 0)
            {
                double adj = conf.get_rpm_adjustment();
                double current_rpm = data.engine.rpm / adj;
                double new_adj = (double)rpm / current_rpm;
                Log::tracex(CMD_LOG_TAG, "Command tachometer calibration", "RPM {%d} 2RPM {%d} Adj {%.2f} 2Adj {%.2f}", current_rpm, rpm, adj, new_adj);
                conf.save_rpm_adjustment(new_adj);
            }
        }
        break;
        case 't':
        {
            Log::tracex(CMD_LOG_TAG, "Command tachometer adjustment", "t {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0)
            {
                conf.save_rpm_adjustment(adj / RPM_ADJUSTMENT_SCALE);
            }
        }
        break;
        default:
            Log::tracex(CMD_LOG_TAG, "Unknown command", " CMD {%c} Value {%s}", command, command_value);
        }
    }
};

#endif
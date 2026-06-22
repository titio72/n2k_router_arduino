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
        case 'S': // switch services
        {
            Log::tracex(CMD_LOG_TAG, "Command switch services", "S {%s}", command_value);
            N2KServices c = conf.get_services();
            c.from_string(command_value);
            conf.save_services(c);
        }
        break;
        case 'N': // set device name
        {
            Log::tracex(CMD_LOG_TAG, "Command set device name", "N {%s}", command_value);
            conf.save_device_name(command_value);
        }
        break;
        case 'B': // set battery capacity in Ah
        {
            Log::tracex(CMD_LOG_TAG, "Command set battery capacity AH", "B {%s}", command_value);
            int c = atoi(command_value);
            if (c > 0)
            {
                conf.save_battery_capacity(c);
            }
        }
        break;
        case 'H': // set engine hours
        {
            Log::tracex(CMD_LOG_TAG, "Command set engine time hhhh:mm", "H {%s}", command_value);
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
        case 'T': // tachometer calibration
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
        case 't': // tachometer adjustment
        {
            Log::tracex(CMD_LOG_TAG, "Command tachometer adjustment", "t {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0)
            {
                conf.save_rpm_adjustment(adj / RPM_ADJUSTMENT_SCALE);
            }
        }
        break;
        case 's': // stw paddle adjustment
        {
            Log::tracex(CMD_LOG_TAG, "Command stw paddle adjustment", "s {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0)
            {
                conf.save_stw_paddle_adjustment(adj / STW_PADDLE_ADJUSTMENT_SCALE);
            }
        }
        break;
        case 'a': // stw paddle alpha
        {
            Log::tracex(CMD_LOG_TAG, "Command stw paddle alpha", "a {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0)
            {
                conf.save_stw_paddle_alpha(adj / STW_PADDLE_ALPHA_SCALE);
            }
        }
        break;
        case 'h': // heartbeat
        {
            // heartbeat
        }
        break;
        default:
            Log::tracex(CMD_LOG_TAG, "Unknown command", " CMD {%c} Value {%s}", command, command_value);
        }
    }
};

#endif
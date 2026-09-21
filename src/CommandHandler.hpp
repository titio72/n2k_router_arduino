#ifndef _COMMAND_HANDLER_HPP
#define _COMMAND_HANDLER_HPP
#include <Log.h>
#include <math.h>
#include <stdlib.h>
#include "Constants.h"
#include "Conf.h"
#include "Context.h"

static const char* CMD_LOG_TAG = "CMD";

static const char COMMAND_SWITCH_SERVICES = 'S';
static const char COMMAND_SET_DEVICE_NAME = 'N';
static const char COMMAND_SET_BATTERY_CAPACITY = 'B';
static const char COMMAND_SET_ENGINE_HOURS = 'H';
static const char COMMAND_TACHOMETER_CALIBRATION = 'T';
static const char COMMAND_TACHOMETER_ADJUSTMENT = 't';
static const char COMMAND_STW_PADDLE_ADJUSTMENT = 's';
static const char COMMAND_STW_PADDLE_ALPHA = 'a';
static const char COMMAND_SEA_TEMP_ADJUSTMENT = 'w';
static const char COMMAND_SEA_TEMP_ALPHA = 'x';
static const char COMMAND_HEARTBEAT = 'h';
static const char COMMAND_RESET = 'R';

class CommandHandler
{
public:
    static void on_command(char command, const char *command_value, Configuration &conf, EngineHours &engineHours, Data &data)
    {
        switch (command)
        {
        case COMMAND_SWITCH_SERVICES: // switch services
        {
            Log::tracex(CMD_LOG_TAG, "Command switch services", "S {%s}", command_value);
            if (command_value[0] == '\0')
            {
                // an empty payload would silently switch every service off
                Log::tracex(CMD_LOG_TAG, "Error command switch services - empty value");
                break;
            }
            N2KServices c = conf.get_services();
            c.from_string(command_value);
            conf.save_services(c);
            #if DO_LOGGER == 1
            if (c.is_use_logger())
            {
                Log::enable();
            }
            else
            {
                Log::disable();
            }
            #endif
        }
        break;
        case COMMAND_SET_DEVICE_NAME: // set device name
        {
            Log::tracex(CMD_LOG_TAG, "Command set device name", "N {%s}", command_value);
            conf.save_device_name(command_value);
        }
        break;
        case COMMAND_SET_BATTERY_CAPACITY: // set battery capacity in Ah
        {
            Log::tracex(CMD_LOG_TAG, "Command set battery capacity AH", "B {%s}", command_value);
            int c = atoi(command_value);
            if (c > 0 && c <= 0xFFFF)
            {
                conf.save_battery_capacity(c);
            }
        }
        break;
        case COMMAND_SET_ENGINE_HOURS: // set engine hours
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
        case COMMAND_TACHOMETER_CALIBRATION: // tachometer calibration
        {
            Log::tracex(CMD_LOG_TAG, "Command tachometer calibration", "T {%s}", command_value);
            int rpm = atoi(command_value);
            if (rpm > 0)
            {
                double adj = conf.get_rpm_adjustment();
                if (adj <= 0.001) // check for zero or negative
                {
                    Log::tracex(CMD_LOG_TAG, "Error command tachometer calibration - adjustment is 0");
                }
                else if (data.engine.rpm == 0)
                {
                    Log::tracex(CMD_LOG_TAG, "Error command tachometer calibration - no current RPM reading");
                }
                else
                {
                    double current_rpm = data.engine.rpm / adj;
                    double new_adj = (double)rpm / current_rpm;
                    if (!isfinite(new_adj) || new_adj <= 0.0)
                    {
                        Log::tracex(CMD_LOG_TAG, "Error command tachometer calibration - invalid adjustment");
                        break;
                    }
                    Log::tracex(CMD_LOG_TAG, "Command tachometer calibration", "RPM {%.2f} 2RPM {%d} Adj {%.2f} 2Adj {%.2f}", current_rpm, rpm, adj, new_adj);
                    conf.save_rpm_adjustment(new_adj);
                }
            }
        }
        break;
        case COMMAND_TACHOMETER_ADJUSTMENT: // tachometer adjustment
        {
            Log::tracex(CMD_LOG_TAG, "Command tachometer adjustment", "t {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0)
            {
                conf.save_rpm_adjustment(adj / RPM_ADJUSTMENT_SCALE);
            }
        }
        break;
        case COMMAND_STW_PADDLE_ADJUSTMENT: // stw paddle adjustment
        {
            Log::tracex(CMD_LOG_TAG, "Command stw paddle adjustment", "s {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0)
            {
                conf.save_stw_paddle_adjustment(adj / STW_PADDLE_ADJUSTMENT_SCALE);
            }
        }
        break;
        case COMMAND_STW_PADDLE_ALPHA: // stw paddle alpha
        {
            Log::tracex(CMD_LOG_TAG, "Command stw paddle alpha", "a {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0 && adj <= STW_PADDLE_ALPHA_SCALE) // alpha is a smoothing factor in (0, 1]
            {
                conf.save_stw_paddle_alpha(adj / STW_PADDLE_ALPHA_SCALE);
            }
        }
        break;
        case COMMAND_SEA_TEMP_ADJUSTMENT: // sea temp adjustment
        {
            Log::tracex(CMD_LOG_TAG, "Command sea temp adjustment", "w {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0)
            {
                conf.save_sea_temp_adjustment(adj / SEA_TEMP_ADJUSTMENT_SCALE);
            }
        }
        break;
        case COMMAND_SEA_TEMP_ALPHA: // sea temp alpha
        {
            Log::tracex(CMD_LOG_TAG, "Command sea temp alpha", "x {%s}", command_value);
            int adj = atoi(command_value);
            if (adj > 0 && adj <= SEA_TEMP_ALPHA_SCALE) // alpha is a smoothing factor in (0, 1]
            {
                conf.save_sea_temp_alpha(adj / SEA_TEMP_ALPHA_SCALE);
            }
        }
        break;
        case COMMAND_HEARTBEAT: // heartbeat
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
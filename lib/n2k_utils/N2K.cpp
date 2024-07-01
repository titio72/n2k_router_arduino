#ifdef ESP32_ARCH
    #ifndef ESP32_CAN_TX_PIN
    #define ESP32_CAN_TX_PIN GPIO_NUM_4
    #endif
    #ifndef ESP32_CAN_RX_PIN
    #define ESP32_CAN_RX_PIN GPIO_NUM_5
    #endif
    #ifdef ESP32_C3
    #include <NMEA2000_esp32xx.h>
    #define N2K_CLASS tNMEA2000_esp32xx
    #else
    #include <NMEA2000_esp32.h>
    #define N2K_CLASS tNMEA2000_esp32
    #endif
#else
    #include <NMEA2000_SocketCAN.h>
#endif

#include <time.h>
#include <math.h>
#include "N2K.h"
#include "Utils.h"
#include "Log.h"

bool static_initialized = false;
N2K* instance = NULL;
N2KStats stats;

N2K* N2K::get_instance(void (*_msg_andler)(const tN2kMsg &N2kMsg), void (*_src_handler)(const unsigned char old_s, const unsigned char new_s))
{
    if (instance==NULL)
    {
        instance = new N2K(_msg_andler, _src_handler);
    }
    return instance;
}

void (*_handler)(const tN2kMsg &N2kMsg);
void (*_source_handler)(const unsigned char old_source, const unsigned char new_source);

N2K::N2K(void (*_msg_handler)(const tN2kMsg &N2kMsg), void (*_src_handler)(const unsigned char old_s, const unsigned char new_s))
{
    _handler = _msg_handler;
    _source_handler = _src_handler;
    desired_source = 22;
    pgns = (unsigned long*)malloc(sizeof(unsigned long) * 1);
    pgns[0] = 0;
    n_pgns = 0;
}

N2K::~N2K()
{
    free(pgns);
}

void private_message_handler(const tN2kMsg &N2kMsg)
{
    stats.recv++;
    if (_handler) _handler(N2kMsg);
}

unsigned char N2K::get_source()
{
    if (NMEA2000)
        return NMEA2000->GetN2kSource();
    else
        return 0xfe;
}

void N2K::set_desired_source(unsigned char src)
{
    desired_source = src;
}

bool N2K::is_initialized()
{
    return static_initialized;
}

void N2K::loop(unsigned long time)
{
    if (is_initialized())
    {
        unsigned char s = NMEA2000->GetN2kSource();
        if (s!=desired_source)
        {
            // claimed new source
            Log::trace("[N2k] Claimed new N2K source: old {%d} new {%d}\n", desired_source, s);
            if (_source_handler) _source_handler(desired_source, s);
            desired_source = s;
        }
        NMEA2000->ParseMessages();
    }
}

void N2K::set_can_socket_name(const char* name)
{
    strcpy(socket_name, name);
}

void N2K::add_pgn(unsigned long pgn)
{
    // pgns is an array terminated by a 0 (so we do not have to define the length)
    pgns = (unsigned long*)realloc(pgns, sizeof(unsigned long) * (n_pgns + 2));
    pgns[n_pgns] = pgn;
    pgns[n_pgns + 1] = 0;
    n_pgns++;
}

void N2K::setup()
{
    if (!is_initialized())
    {
        #ifdef ESP32_ARCH
        Log::trace("[N2k] Initializing N2K on {RX %d - TX %d}\n", ESP32_CAN_RX_PIN, ESP32_CAN_TX_PIN);
        NMEA2000 = new N2K_CLASS(ESP32_CAN_TX_PIN, ESP32_CAN_RX_PIN);
        #else
        Log::trace("[N2k] Initializing N2K on {%s}\n", socket_name);
        NMEA2000 = new tNMEA2000_SocketCAN(socket_name);
        #endif

        NMEA2000->SetProductInformation("00000001",                         // Manufacturer's Model serial code
                                       100,                                // Manufacturer's product code
                                       "ABN2k                           ", // Manufacturer's Model ID
                                       "1.0.3.0 (2024-01-07)",             // Manufacturer's Software version code
                                       "1.0.2.0 (2019-07-07)"              // Manufacturer's Model version
        );
        NMEA2000->SetDeviceInformation(1,   // Unique number. Use e.g. Serial number.
                                      145, // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                      60,  // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                      2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
        );
        NMEA2000->SetMode(tNMEA2000::N2km_ListenAndNode, desired_source);
        NMEA2000->SetMsgHandler(private_message_handler);
        NMEA2000->SetN2kCANSendFrameBufSize(1000);
        NMEA2000->EnableForward(false); // Disable all msg forwarding to USB (=Serial)
        if (pgns) NMEA2000->ExtendTransmitMessages(pgns);
        static_initialized = NMEA2000->Open();
        stats.canbus = static_initialized;
        Log::trace("[N2K] N2K initialized {%s}\n", is_initialized() ? "OK" : "KO");
    }
    else
    {
        Log::trace("[N2K] N2K already initialized!\n");
    }
}

bool N2K::send_msg(const tN2kMsg &N2kMsg)
{
    if (is_initialized())
    {
        if (_handler)
        {
            // notify internal listeners, that otherwise would not get the message
            _handler(N2kMsg);
        }
        if (NMEA2000->SendMsg(N2kMsg))
        {
            stats.sent++;
            return true;
        }
        else
        {
            Log::trace("[N2K] Failed message {%d}\n", N2kMsg.PGN);
            stats.fail++;
            return false;
        }
    }
    else
    {
        return false;
    }
}

void N2KStats::dump()
{
    Log::trace("[N2K] Bus: Bus {%d} CAN.TX {%d/%d} CAN.RX {%d}\n", canbus, sent, fail, recv);
}

void N2KStats::reset()
{
    recv = 0;
    sent = 0;
    fail = 0;
}

N2KStats N2K::getStats()
{
    return stats;
}

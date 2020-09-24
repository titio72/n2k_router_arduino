#include "constants.h"
#ifdef ESP32_ARCH
#define USE_N2K_CAN USE_N2K_MCP_CAN
#define N2k_SPI_CS_PIN 5
#define N2k_CAN_INT_PIN 0xff
#define USE_MCP_CAN_CLOCK_SET 8

//#define USE_N2K_CAN USE_N2K_ESP32_CAN
//#define ESP32_CAN_TX_PIN GPIO_NUM_21
//#define ESP32_CAN_RX_PIN GPIO_NUM_22
#else
#define SOCKET_CAN_PORT "vcan0"
#endif
#include <NMEA2000_CAN.h>
#include <time.h>
#include <math.h>
#include "N2K.h"
#include "Utils.h"
#include "Log.h"

ulong* _can_received;
void (*_handler)(const tN2kMsg &N2kMsg);

void private_message_handler(const tN2kMsg &N2kMsg) {
    (*_can_received)++;
    _handler(N2kMsg);
}

void N2K::loop() {
    NMEA2000.ParseMessages();
}

bool N2K::sendMessage(int dest, ulong pgn, int priority, int len, unsigned char* payload) {
    tN2kMsg m(src);
    m.Init(priority, pgn, src, dest);
    for (int i = 0; i<len; i++) m.AddByte(payload[i]);
    return send_msg(m);
}

bool N2K::send126996Request(int dst) {
    tN2kMsg N2kMsg(src);
    SetN2kPGNISORequest(N2kMsg, dst, 126996);
    return send_msg(N2kMsg);
}

void N2K::setup(void (*_MsgHandler)(const tN2kMsg &N2kMsg), statistics* s, uint8_t _src) {
    src = _src;
    stats = s;
    _can_received = &(s->can_received);
    _handler = _MsgHandler;
    Log::trace("Initializing N2K\n");
    NMEA2000.SetN2kCANSendFrameBufSize(150);
    NMEA2000.SetN2kCANReceiveFrameBufSize(150),
    NMEA2000.SetN2kCANMsgBufSize(15);
    Log::trace("Initializing N2K Product Info\n");
    NMEA2000.SetProductInformation("00000001", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                /*1234567890123456789012345678901234567890*/
                                 "ABN2k                           ",  // Manufacturer's Model ID
                                 "1.0.2.25 (2019-07-07)",  // Manufacturer's Software version code
                                 "1.0.2.0 (2019-07-07)" // Manufacturer's Model version
                                 );
    Log::trace("Initializing N2K Device Info\n");
    NMEA2000.SetDeviceInformation(1, // Unique number. Use e.g. Serial number.
                                132, // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                25, // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
                               );
    Log::trace("Initializing N2K mode\n");
    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, src);
    NMEA2000.EnableForward(false); // Disable all msg forwarding to USB (=Serial)
    Log::trace("Initializing N2K Port & Handlers\n");
    NMEA2000.SetMsgHandler(private_message_handler);
    bool initialized = NMEA2000.Open();
    Log::trace("Initializing N2K %s\n", initialized?"OK":"KO");

}

bool N2K::send_msg(const tN2kMsg &N2kMsg) {
    _handler(N2kMsg);
    if (NMEA2000.SendMsg(N2kMsg)) {
        stats->can_sent++;
        return true;
    } else {
        Serial.printf("Failed message {%d}\n", N2kMsg.PGN);
        stats->can_failed++;
        return false;
    }
}

bool N2K::sendCOGSOG(GSA& gsa, RMC& rmc, int sid) {
    if (gsa.valid && rmc.valid && gsa.fix >= 2) {
        if (isnan(rmc.sog) && isnan(rmc.cog)) return false;
        else {
            tN2kMsg N2kMsg(src);
            SetN2kCOGSOGRapid(N2kMsg, sid, N2khr_true, DegToRad(isnan(rmc.cog)?0.0:rmc.cog), rmc.sog * 1852.0 / 3600);
            return send_msg(N2kMsg);
        }
    }
    return false;
}

bool N2K::sendGNNSStatus(GSA& gsa, int sid) {
    if (gsa.valid) {
        tN2kMsg N2kMsg(src);
        tN2kGNSSDOPmode mode;
        switch (gsa.fix) {
            case 3: mode = tN2kGNSSDOPmode::N2kGNSSdm_3D; break;
            case 2: mode = tN2kGNSSDOPmode::N2kGNSSdm_2D; break;
            case 1: mode = tN2kGNSSDOPmode::N2kGNSSdm_1D; break;
            default: mode = tN2kGNSSDOPmode::N2kGNSSdm_Unavailable;
        }
        SetN2kPGN129539(N2kMsg, sid, tN2kGNSSDOPmode::N2kGNSSdm_2D, mode, gsa.hdop, gsa.vdop, gsa.pdop);
        return send_msg(N2kMsg);
    }
    return false;
}


bool N2K::sendGNSSPosition(GSA& gsa, RMC& rmc, int sid) {
    if (gsa.valid && rmc.valid && gsa.fix >= 2) {
        tN2kMsg N2kMsg(src);
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kPGN129029(N2kMsg, sid, days_since_1970, second_since_midnight, rmc.lat, rmc.lon, N2kDoubleNA, tN2kGNSStype::N2kGNSSt_GPS,
            tN2kGNSSmethod::N2kGNSSm_GNSSfix, gsa.nSat, gsa.hdop);
        return send_msg(N2kMsg);
    }
    return false;
}

bool N2K::sendTime(RMC& rmc, int sid) {
    tN2kMsg N2kMsg(src);
    int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
    double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
    SetN2kSystemTime(N2kMsg, sid, days_since_1970, second_since_midnight);
    return send_msg(N2kMsg);
}

bool N2K::sendLocalTime(GSA& gsa, RMC& rmc) {
    if (gsa.valid && rmc.valid && gsa.fix >= 2)  {
        tN2kMsg N2kMsg(src);
        int days_since_1970 = getDaysSince1970(rmc.y, rmc.M, rmc.d);
        double second_since_midnight = rmc.h * 60 * 60 + rmc.m * 60 + rmc.s + rmc.ms / 1000.0;
        SetN2kLocalOffset(N2kMsg, days_since_1970, second_since_midnight, 0);
        return send_msg(N2kMsg);
    }
    return false;
}

bool N2K::sendTime(time_t _now, int sid, short ms) {
    tm* t = gmtime(&_now);
    int days_since_1970 = getDaysSince1970(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    double second_since_midnight = t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec * (double)ms/1000.0;
    tN2kMsg N2kMsg(src);
    SetN2kSystemTime(N2kMsg, sid, days_since_1970, second_since_midnight);
    return send_msg(N2kMsg);
}

bool N2K::sendPosition(GSA& gsa, RMC& rmc) {
    if (gsa.valid && rmc.valid && gsa.fix >= 2) {
        tN2kMsg N2kMsg(src);
        SetN2kPGN129025(N2kMsg, rmc.lat, rmc.lon);
        return send_msg(N2kMsg);
    }
    return false;
}

bool N2K::sendElectronicTemperature(const float temp, int sid) {
    tN2kMsg N2kMsg(src);
    SetN2kTemperature(N2kMsg, sid, 0, tN2kTempSource::N2kts_EngineRoomTemperature, temp);
    return send_msg(N2kMsg);
}

bool N2K::sendEnvironment(const float pressurePA, const float humidity, const float temperatureC, int sid) {
    tN2kMsg N2kMsg(src);
    SetN2kEnvironmentalParameters(N2kMsg, sid,
        N2kts_MainCabinTemperature, CToKelvin(temperatureC),
        N2khs_InsideHumidity, humidity,
        pressurePA);
    return send_msg(N2kMsg);
}

/*
{
    "SID":69,
    "Sats in View":11,
    "list":[
        {"PRN":2,"Elevation":27.0,"Azimuth":49.0,"SNR":39.00,"Range residuals":0,"Status":"Used"},
        {"PRN":5,"Elevation":1.0,"Azimuth":92.0,"SNR":37.00,"Range residuals":0,"Status":"Used"},
        {"PRN":6,"Elevation":1.0,"Azimuth":28.0,"SNR":35.00,"Range residuals":0,"Status":"Used"},
        {"PRN":18,"Elevation":15.0,"Azimuth":179.0,"SNR":36.00,"Range residuals":0,"Status":"Used"},
        {"PRN":25,"Elevation":66.0,"Azimuth":68.0,"SNR":34.00,"Range residuals":0,"Status":"Used"},
        {"PRN":29,"Elevation":82.0,"Azimuth":254.0,"SNR":42.00,"Range residuals":0,"Status":"Used"},
        {"PRN":31,"Elevation":165.0,"Azimuth":0.0,"SNR":38.00,"Range residuals":0,"Status":"Used"},
        {"PRN":32,"Elevation":165.0,"Azimuth":0.0,"SNR":30.00,"Range residuals":0,"Status":"Used"},
        {"PRN":33,"Elevation":34.0,"Azimuth":214.0,"SNR":0.00,"Range residuals":0,"Status":"Not tracked"},
        {"PRN":37,"Elevation":39.0,"Azimuth":163.0,"SNR":0.00,"Range residuals":0,"Status":"Not tracked"},
        {"PRN":39,"Elevation":38.0,"Azimuth":158.0,"SNR":0.00,"Range residuals":0,"Status":"Not tracked"}
    ]}

*/

bool N2K::sendSatellites(const sat* sats, uint n, int sid, GSA& gsa) {
    if (n>0) {
        tN2kMsg m(src);
        m.Init(6, 129540, src, 255);
        m.AddByte((unsigned char)sid);
        m.AddByte((unsigned char)(3 & 0x04) << 6);
        m.AddByte((unsigned char)(n<18?n:18));
        // limit to 18 sats so to remain within 232 bytes
        for (int i = 0; i<n && i<18; i++) {
            sat s = sats[i];
            m.AddByte((unsigned char)s.sat_id);
            m.Add2ByteInt((int)(s.elev / 180.0 * M_PI / 0.0001) );
            m.Add2ByteInt((int)(s.az / 180.0 * M_PI / 0.0001) );
            if (s.db) m.Add2ByteUInt((int)(s.db / 0.01)); else m.Add2ByteUInt(N2kUInt16NA);
            m.Add4ByteUInt(N2kInt32NA);
            m.AddByte((s.status & 0x0F) | 0xF0);
        }
        return send_msg(m);
    }
    return false;
/*
          "Order": 1, "Id": "sid", "BitLength": 8, "BitOffset": 0, "BitStart": 0, "Signed": false
          "Order": 2, "Id": "mode", "Name": "Mode", "BitLength": 2, "BitOffset": 8, "BitStart": 0, "Type": "Lookup table", "Signed": false, "EnumValues": [
            {
              "name": "Range residuals used to calculate position",     "value": "3"
            }
          "Order": 3, "Id": "reserved", "Name": "Reserved", "Description": "Reserved", "BitLength": 6, "BitOffset": 10, "BitStart": 2, "Type": "Binary data", "Signed": false
          "Order": 4, "Id": "satsInView", "Name": "Sats in View", "BitLength": 8, "BitOffset": 16, "BitStart": 0, "Signed": false
          
          "Order": 5, "Id": "prn", "Name": "PRN", "BitLength": 8, "BitOffset": 24, "BitStart": 0, "Signed": false
          "Order": 6, "Id": "elevation", "Name": "Elevation", "BitLength": 16, "BitOffset": 32, "BitStart": 0, "Units": "rad", "Resolution": "0.0001", "Signed": false
          "Order": 7, "Id": "azimuth", "Name": "Azimuth", "BitLength": 16, "BitOffset": 48, "BitStart": 0, "Units": "rad", "Resolution": "0.0001", "Signed": false
          "Order": 8, "Id": "snr", "Name": "SNR", "BitLength": 16, "BitOffset": 64, "BitStart": 0, "Units": "dB", "Resolution": "0.01", "Signed": false
          "Order": 9, "Id": "rangeResiduals", "Name": "Range residuals", "BitLength": 32, "BitOffset": 80, "BitStart": 0, "Signed": true
          "Order": 10, "Id": "status", "Name": "Status", "BitLength": 4, "BitOffset": 112, "BitStart": 0, "Type": "Lookup table", "Signed": false, "EnumValues": [
              "name": "Not tracked",     "value": "0"
              "name": "Tracked",     "value": "1"
              "name": "Used",     "value": "2"
              "name": "Not tracked+Diff",     "value": "3"
              "name": "Tracked+Diff",     "value": "4"
              "name": "Used+Diff",     "value": "5"
          "Order": 11, "Id": "reserved", "Name": "Reserved", "Description": "Reserved", "BitLength": 4, "BitOffset": 116, "BitStart": 4, "Type": "Binary data", "Signed": false
*/
}

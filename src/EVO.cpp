#include <Arduino.h>

#include "EVO.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

bool EVO::SetHeading(double heading) {
  String msg2 = "Z,3,126208,22,204,14,01,50,ff,00,f8,03,01,3b,07,03,04,06,00,00";  //set 0 magnetic
  /*
    "Function Code":"Command",
    "PGN":65360,
    "Priority":8,
    "# of Parameters":3,
    "list":[
        {"Parameter":1,"Value":1851},
        {"Parameter":3,"Value":4},
        {"Parameter":6,"Value":0}]
    }
    */
  // string msg3 = "Z,3,126208,22,204,14,01,50,ff,00,f8,03,01,3b,07,03,04,06,9f,3e";  //set 92 magnetic
  // string msg4 = "Z,3,126208,22,204,14,01,50,ff,00,f8,03,01,3b,07,03,04,06,4e,3f";  //set 93 example only, magnetic
  /*
    "Function Code":"Command",
    "PGN":65360,
    "Priority":8,
    "# of Parameters":3,
    "list":[
        {"Parameter":1,"Value":1851},
        {"Parameter":3,"Value":4},
        {"Parameter":6,"Value":16206}
        ]
        */
       
  double heading_normal = heading;
  while (heading_normal < 0) heading_normal += 360;
  while (heading_normal >= 360) heading_normal -= 360;
  uint16_t heading_radials1000 = (uint16_t)(heading_normal * 174.53); // heading to be set in thousands of radials

  uint8_t byte0, byte1;
  byte0 = heading_radials1000 & 0xff;
  byte1 = heading_radials1000 >> 8;

  char s[4];
  sprintf(s, "%0x", byte0);
  if (byte0 > 15) {
    msg2[56] = s[0];
    msg2[57] = s[1];
  }
  else {
    msg2[56] = '0';
    msg2[57] = s[0];
  }
  sprintf(s, "%0x", byte1);
  if (byte1 > 15) {
    msg2[59] = s[0];
    msg2[60] = s[1];
  }
  else {
    msg2[59] = '0';
    msg2[60] = s[0];
  }
}

bool EVO::SetAuto() {
  String auto_command = "Z,3,126208,22,204,17,01,63,ff,00,f8,04,01,3b,07,03,04,04,40,00,05,ff,ff";  // set auto
  /*
  "Function Code":"Command",
  "PGN":65379,
  "Priority":8,
  "# of Parameters":4,
  "list":[
      {"Parameter":1,"Value":1851},
      {"Parameter":3,"Value":4},
      {"Parameter":4,"Value":64},
      {"Parameter":0}
    ]}
  */
}

bool EVO::SetStandBy() {
  String standby_command = "Z,3,126208,22,204,17,01,63,ff,00,f8,04,01,3b,07,03,04,04,00,00,05,ff,ff";  //set standby
  /*
  "Function Code":"Command",
  "PGN":65379,
  "Priority":8,
  "# of Parameters":4,
  "list":[
    {"Parameter":1,"Value":1851},
    {"Parameter":3,"Value":4},
    {"Parameter":4,"Value":0},
    {"Parameter":0}
  ]}
  */
}

bool EVO::SetTrack() {
  String track =          "Z,3,126208,22,204,0d,3b,9f,f0,81,84,46,27,9d,4a,00,00,02,08,4e";  // status message for track
  /*
  "Function Code":"Command",
  "PGN":65379,
  "Priority":8,
  "# of Parameters":4,
  "list":[
      {"Parameter":1,"Value":1851},
      {"Parameter":3,"Value":4},
      {"Parameter":4,"Value":0},
      {"Parameter":0}
    ]}
  */
}

bool EVO::SetVane() {
    
}

bool EVO::SetWind(double wind) {
    
}


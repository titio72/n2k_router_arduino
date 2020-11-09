add_executable(gps_adapter
  main.cpp
  Network.cpp
  UDPServer.cpp
  NMEA.cpp
  Ports.cpp
  Utils.cpp
  Log.cpp
  N2K.cpp
  Conf.cpp
  Simulator.cpp
)

include_directories(../src)

target_link_libraries(gps_adapter /home/aboni/Documents/PlatformIO/Projects/NMEARouter/linux_lib/libnmea2000.a)

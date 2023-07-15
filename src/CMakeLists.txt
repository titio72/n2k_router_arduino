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

target_link_libraries(gps_adapter
	${PROJECT_SOURCE_DIR}/deps/NMEA2000/build/src/libnmea2000.a
	${PROJECT_SOURCE_DIR}/deps/NMEA2000_socketCAN/libnmea2000_socketcan.a
)

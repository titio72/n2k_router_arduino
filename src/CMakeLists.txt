add_executable(n2k_router
  main.cpp
  Network.cpp
  NMEA.cpp
  Ports.cpp
  Utils.cpp
  Log.cpp
  N2K.cpp
  Conf.cpp
  Simulator.cpp
  GPS.cpp
  HumTemp.cpp
  PressureTemp.cpp
  WebServer.cpp
  Display.cpp
)

include_directories(../src)

target_link_libraries(n2k_router
	${PROJECT_SOURCE_DIR}/deps/NMEA2000/build/src/libnmea2000.a
	${PROJECT_SOURCE_DIR}/deps/NMEA2000_socketCAN/libnmea2000_socketcan.a
)

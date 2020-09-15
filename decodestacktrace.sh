echo $1 > dump.txt
java EspStackTraceDecoder  ~/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-addr2line .pio/build/az-delivery-devkit-v4/firmware.elf dump.txt

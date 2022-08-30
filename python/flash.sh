#!/bin/sh
# runs make to flash firmware
cd ../Firmware/ice-v_mfgtest/build/
make flash ESPPORT=/dev/ttyACM0 ESPBAUD=2000000

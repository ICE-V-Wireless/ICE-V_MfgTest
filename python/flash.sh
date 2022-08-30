#!/bin/sh
# runs make to flash firmware - arg is dir to run from
cd $1
make flash ESPPORT=/dev/ttyACM0 ESPBAUD=2000000

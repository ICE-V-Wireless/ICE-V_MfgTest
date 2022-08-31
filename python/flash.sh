#!/bin/sh
# runs make to flash firmware - 1st arg is TTY port, 2nd arg is dir to run from
cd $2
make flash ESPPORT=$1 ESPBAUD=2000000

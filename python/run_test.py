#!/bin/python3
# Script for the ICE-V Wireless manufacturing testing. This program will
# execute a single pass of the test and firmware load process.

from subprocess import Popen, PIPE, STDOUT

#cmd = ['/opt/espressif/esp-idf_V4.4.2/tools/idf_monitor.py', '-p', \
#       '/dev/ttyACM0', '../Firmware/ice-v_mfgtest/build/ICE-V_MfgTst.elf']

cmd = ['python', \
       '/opt/espressif/esp-idf_V4.4.2/components/esptool_py/esptool/esptool.py', \
       '--chip esp32c3', '-p', '/dev/ttyACM0', '-b', '2000000', \
        'write_flash', \
       '@../Firmware/ice-v_mfgtest/build/flash_project_args']
p = Popen(cmd, stdout=PIPE, stdin=PIPE, stderr=STDOUT)

output = p.communicate()[0]

print(output)


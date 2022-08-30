#!/bin/python3
# Script for the ICE-V Wireless manufacturing testing. This program will
# execute a single pass of the test and firmware load process.

from subprocess import Popen, PIPE, STDOUT
import serial

# flash mfg test firmware
print("Flashing Mfg Test Firmware...")
cmd = ['./flash.sh', '../Firmware/ice-v_mfgtest/build/']
p = Popen(cmd, stdout=PIPE, stdin=PIPE, stderr=STDOUT)

output = p.communicate()[0]
output = output.decode('UTF-8')

if "[100%] Built target flash" in output:
    print("Test Firmware Flash Succeeded")
elif "Error" in output:
    print("Test Firmware Flash Failed")
else:
    print("Test Firmware Flash Unknown result")

# open up serial port and get test results
print("Collecting Mfg Test Firmware Results")
with serial.Serial("/dev/ttyACM0") as tty:
    while True:
        reply = tty.read_until()
        rplystr = reply.decode('utf-8')
        if "#TEST#" in rplystr:
            print(rplystr)
            if "Complete" in rplystr:
                # test is done so bail out and close port
                break

# Report result
if "SUCCEED" in rplystr:
    print("Test Passed")
elif "FAIL" in rplystr:
    print("Test Failed")
else:
    print("Test Unknown result")

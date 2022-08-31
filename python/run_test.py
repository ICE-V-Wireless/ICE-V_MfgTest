#!/bin/python3
# Script for the ICE-V Wireless manufacturing testing. This program will
# execute a single pass of the test and firmware load process.

from subprocess import Popen, PIPE, STDOUT
import serial
import re

def escape_ansi(line):
    ansi_escape =re.compile(r'(\x9B|\x1B\[)[0-?]*[ -\/]*[@-~]')
    return ansi_escape.sub('', line)

# enable to install and run hardware test
if True:
    if True:
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
            rplystr = escape_ansi(reply.decode('utf-8'))
            rplystr = rplystr.rstrip('\n')
            rplystr = rplystr[:-1]
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

# enable to install & run end-user firmware
if True:
    if True:
        # flash default end-user firmware
        print("Flashing End-User Firmware...")
        cmd = ['./flash.sh', '../../ICE-V-Wireless/Firmware/build/']
        p = Popen(cmd, stdout=PIPE, stdin=PIPE, stderr=STDOUT)

        output = p.communicate()[0]
        output = output.decode('UTF-8')

        if "[100%] Built target flash" in output:
            print("End-User Firmware Flash Succeeded")
        elif "Error" in output:
            print("End-User Firmware Flash Failed")
        else:
            print("End-User Firmware Flash Unknown result")

    # try usb-serial command to check ID register
    print("Testing End-User Firmware ID...")
    cmd = ['../../ICE-V-Wireless/python/send_c3usb.py', '-r', '0']

    # Try 5x before giving up
    tries = 1
    while tries<5:
        p = Popen(cmd, stdout=PIPE, stdin=PIPE, stderr=STDOUT)

        output = p.communicate()[0]
        output = output.decode('UTF-8')
        print("Try ", tries, ":", output)
        if "0xb00f0001" in output:
            break;
        tries = tries + 1
        
    if "0xb00f0001" in output:
        print("USB command Succeeded")
    elif "Error" in output:
        print("USB command Failed")
    else:
        print("USB command Unknown result")


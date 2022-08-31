#!/bin/python3
# Script for the ICE-V Wireless manufacturing testing. This program will
# execute a single pass of the test and firmware load process.

import getopt
import sys
from subprocess import Popen, PIPE, STDOUT
import serial
import re

# strips ANSI escape sequences from received text
def escape_ansi(line):
    ansi_escape =re.compile(r'(\x9B|\x1B\[)[0-?]*[ -\/]*[@-~]')
    return ansi_escape.sub('', line)

# flashes a full set of firmware to the ESP32C3, including SPIFFS
def flash_esp32(port, directory, tag):
    cmd = ['./flash.sh', port, directory]
    p = Popen(cmd, stdout=PIPE, stdin=PIPE, stderr=STDOUT)

    output = p.communicate()[0]
    output = output.decode('UTF-8')

    if "[100%] Built target flash" in output:
        print(tag, "Flash Succeeded")
        return 0
    elif "Error" in output:
        print(tag, "Flash Failed")
        return 1
    else:
        print(tag, "Flash Unknown result")
        return 2

# run the process
def test_process(port, t, u, i):
    err = 0
    if t:
        # flash mfg test firmware
        print("Flashing Mfg Test Firmware...")
        ferr = flash_esp32(port, '../Firmware/ice-v_mfgtest/build/', \
                           'Test Firmware')

        if ferr == False:
            # open up serial port and get test results
            print("Collecting Mfg Test Firmware Results")
            with serial.Serial(port) as tty:
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
                err = 1
            else:
                print("Test Unknown result")
                err = 1
        else:
            print("Skipped Mfg Test")
        
    if u:
        # flash default end-user firmware
        print("Flashing End-User Firmware...")
        ferr = flash_esp32(port, '../../ICE-V-Wireless/Firmware/build/', \
                           'End-User Firmware')
    else:
        ferr = False
    
    if ferr == False and i:
        # try usb-serial command to check ID register
        print("Testing End-User Firmware ID...")
        cmd = ['../../ICE-V-Wireless/python/send_c3usb.py', '-p', port, '-r', '0']

        # Try 5x before giving up
        tries = 1
        ID = "0xb00f0001"
        while tries<5:
            p = Popen(cmd, stdout=PIPE, stdin=PIPE, stderr=STDOUT)

            output = p.communicate()[0]
            output = output.decode('UTF-8')
            print("Try ", tries, ":", output)
            if ID in output:
                break;
            tries = tries + 1
            
        if ID in output:
            print("USB ID command PASS")
        else:
            print("USB ID command FAIL")
            err = err + 1
            
    # all done
    return err

# usage text for command line
def usage():
    print(sys.argv[0], " [options] Execute Mfg Test & Flashing")
    print("  -h, --help              : this message")
    print("  -p, --port=<tty>        : usb tty of ESP32C3 (default /dev/ttyACM0)")
    print("  -t, --test              : only flash & report mfg test")
    print("  -u, --user              : only flash user firmware")
    print("  -i, --id                : only run ID test on user firmware")

# main entry
if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], \
            "hp:tfi", \
            ["help", "port=", "test", "user", "id"])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    # defaults
    port = "/dev/ttyACM0"
    t = 1
    u = 1
    i = 1
    
    # scan thru options
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-p", "--port"):
            port = a
        elif o in ("-t", "--test"):
            f = 0
            i = 0
        elif o in ("-u", "--user"):
            t = 0
            i = 0
        elif o in ("-i", "--id"):
            t = 0
            u = 0
        else:
            assert False, "unhandled option"

    # run the test
    result = test_process(port, t, u, i)
    if result:
        print("Overall - FAIL")
    else:
        print("Overall - PASS")
    
    sys.exit(result)
    

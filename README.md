# ICE-V_MfgTest
Manufacturing Test support ware

## Making it work
This repo uses a local copy of Espressif's `esptool` so you'll need to grab
that like so:

```
git submodule update --init
```

No need to build binaries or install ESP-IDF - the approved binaries are
already included.

* Go into the `ICE-V-MfgTest/python` directory and run the `run_test.py`
script. This will execute a single pass of the automated test.
* For testing multiple units an outer looping script can be used (borrow
from the Orangecrab test suite?)

## What it does
A single pass of the automated test begins with an erased board with no
firmware. It then performs the following operations:
* Attempt to install the bootloader, test firmware and test SPIFFS filesystem.
If that fails the process will abort. If it succeeds then it proceeds to
* Run the manufacturing test which excercises all the main functions of the
board including:
  * SPIFFS initialize
  * FGPA programming with SPI passthru gateware
  * SPI PSRAM
  * FPGA programming with test gateware
  * Read FPGA ID
  * FPGA PLL and external clock oscillator
  * Continuity test on all PMOD I/O
  * Continuity test on ESP32 C3 GPIO
  * Boot button
  * Battery charger
  * WiFi
* If the manufacturing test passes then it proceeds to
* Attempt to install the bootloader, end-user firmware and SPIFFS filesystem.
* Run the USB Serial command line utility to verify the default gateware ID.

If all those pass then a final success message is printed.

## What will be needed for the test to pass
The board will need to be connected to the test fixture which provides proper
continutity loopback for all the GPIO, as well as the battery charge load.

The test operator should be prepared to press the BOOT button at the moment
when the test indicates to do so - there are 5 seconds grace before the test
continues with a failure.

There should be a WiFi network available for the DUT to join. Please set up the
`credentials.h` file in the test firmware directory with the SSID and password.
If there is no WiFi available then that part of the test will fail.


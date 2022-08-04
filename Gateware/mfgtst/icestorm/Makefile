# Makefile for icestorm tools + yosys + nextpnr
# 01-16-18 E. Brombaugh

# src directory
VPATH = ../src

SRC =	../src/mfgtst.v ../src/spi_slave.v \

# project stuff
PROJ = mfgtst
PIN_DEF = ../src/mfgtst.pcf
SDC = ../src/mfgtst.sdc
DEVICE = up5k
PACKAGE = sg48

TOOLS = /opt/openfpga/fpga-toolchain
YOSYS = $(TOOLS)/bin/yosys
NEXTPNR = $(TOOLS)/bin/nextpnr-ice40
NEXTPNR_ARGS = --pre-pack $(SDC) --package $(PACKAGE) --$(DEVICE)
ICEPACK = $(TOOLS)/bin/icepack
ICETIME = $(TOOLS)/bin/icetime
ICEPROG = $(TOOLS)/bin/iceprog
SENDBIT = ../../python/send_c3sock.py
VERILATOR = verilator
TECH_LIB = $(TOOLS)/share/yosys/ice40/cells_sim.v

all: $(PROJ).bin

%.json: $(SRC)
	$(YOSYS) -p 'synth_ice40 -top $(PROJ) -json $@' $(SRC)

%.asc: %.json $(PIN_DEF) 
	$(NEXTPNR) $(NEXTPNR_ARGS) --json $< --pcf $(PIN_DEF) --asc $@
		
%.bin: %.asc
	$(ICEPACK) $< $@

%.rpt: %.asc
	$(ICETIME) -d $(DEVICE) -mtr $@ $<
	
lint: $(SRC)
	$(VERILATOR) --lint-only -Wall --top-module $(PROJ) $(TECH_LIB) $(SRC)

clean:
	rm -f *.json *.asc *.rpt *.bin *.hex

.SECONDARY:
.PHONY: all prog clean

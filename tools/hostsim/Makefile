# Native simulator: real src/ui + src/logic + MiniDexed's real sysexfileloader against a fake SD card.
#   make -C tools/hostsim run                      built-in checks + screenshots
#   make -C tools/hostsim script SCRIPT=my.txt     play a key/encoder script
# MiniDexed (for its sysexfileloader.cpp) is cloned at the build.sh pin on first use; or pass MINIDEXED=/your/checkout.
MINIDEXED ?= ../../build/MiniDexed
PIN := $(shell sed -n 's/^MINIDEXED_REF=//p' ../../build.sh)
R = ../..
SRCS = $(wildcard $(R)/src/ui/*.cpp) $(wildcard $(R)/src/logic/*.cpp) $(R)/src/glue/zicsave.cpp $(MINIDEXED)/src/sysexfileloader.cpp
CXXFLAGS = -std=c++17 -g -O1 -Wall -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined \
  -Istubs -I$(R)/src/ui -I$(R)/src/logic -I$(R)/src/glue -I$(R)/tools/sim -I$(MINIDEXED)/src \
  -DZIC_USER_BANK_PATH='"sdcard/sysex/voice/000099_ZICUSER.syx"'
WORK ?= /tmp/zicdexed-hostsim

$(MINIDEXED)/src/sysexfileloader.cpp:
	git clone -q https://github.com/probonopd/MiniDexed.git $(MINIDEXED)
	git -C $(MINIDEXED) checkout -q $(PIN)

sim_sd: sim_sd.cpp sdsynth.h $(SRCS)
	$(CXX) $(CXXFLAGS) -c $(MINIDEXED)/src/sysexfileloader.cpp -include stubs/fatfs_readdir.h -o sysexloader.o
	$(CXX) $(CXXFLAGS) sim_sd.cpp $(filter-out %sysexfileloader.cpp,$(SRCS)) sysexloader.o -o $@

run: sim_sd
	./sim_sd $(WORK) $(abspath $(R))/sysex/voice/000090_nasty.syx
	python3 $(R)/tools/sim/ppm2png.py $(WORK)/shots

SCRIPT ?= scripts/save-and-browse.txt
script: sim_sd
	./sim_sd $(WORK) $(abspath $(R))/sysex/voice/000090_nasty.syx $(SCRIPT)
	python3 $(R)/tools/sim/ppm2png.py $(WORK)/shots

.PHONY: run script

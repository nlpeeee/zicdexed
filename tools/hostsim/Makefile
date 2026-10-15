# Native simulator: real src/ui + src/logic + MiniDexed's real sysexfileloader against a fake SD card.
#   make -C tools/hostsim run MINIDEXED=/path/to/MiniDexed   (checked out at the build.sh pin)
MINIDEXED ?= ../../build/MiniDexed
R = ../..
SRCS = $(wildcard $(R)/src/ui/*.cpp) $(wildcard $(R)/src/logic/*.cpp) $(R)/src/glue/zicsave.cpp $(MINIDEXED)/src/sysexfileloader.cpp
CXXFLAGS = -std=c++17 -g -O1 -Wall -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined \
  -Istubs -I$(R)/src/ui -I$(R)/src/logic -I$(R)/src/glue -I$(R)/tools/sim -I$(MINIDEXED)/src \
  -DZIC_USER_BANK_PATH='"sdcard/sysex/voice/000099_ZICUSER.syx"'
WORK ?= /tmp/zicdexed-hostsim

sim_sd: sim_sd.cpp sdsynth.h $(SRCS)
	$(CXX) $(CXXFLAGS) -c $(MINIDEXED)/src/sysexfileloader.cpp -include stubs/fatfs_readdir.h -o sysexloader.o
	$(CXX) $(CXXFLAGS) sim_sd.cpp $(filter-out %sysexfileloader.cpp,$(SRCS)) sysexloader.o -o $@

run: sim_sd
	./sim_sd $(WORK) $(abspath $(R))/sysex/voice/000001_nasty.syx
	python3 $(R)/tools/sim/ppm2png.py $(WORK)/shots

.PHONY: run

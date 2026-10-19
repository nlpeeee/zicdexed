# Host tests for src/ui:  make -C tests/ui test   (also writes screenshots with `make -C tests/ui shots`)
CXX ?= g++
CXXFLAGS = -std=c++14 -g -O1 -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=undefined \
           -I../../src/ui -I../../src/logic -I../../tools/sim
SRCS = $(wildcard ../../src/ui/*.cpp) $(wildcard ../../src/logic/*.cpp)

test: ui_test sound_test
	./ui_test
	./sound_test

sound_test: sound_test.cpp $(SRCS) $(wildcard ../../src/ui/*.h) $(wildcard ../../src/logic/*.h) ../../tools/sim/fakesynth.h ../../tools/sim/soundsynth.h
	$(CXX) $(CXXFLAGS) sound_test.cpp $(SRCS) -o $@

ui_test: ui_test.cpp $(SRCS) $(wildcard ../../src/ui/*.h) ../../tools/sim/fakesynth.h
	$(CXX) $(CXXFLAGS) ui_test.cpp $(SRCS) -o $@

shots: ../../tools/sim/sim.cpp $(SRCS)
	$(CXX) -std=c++14 -O1 -I../../src/ui -I../../src/logic -I../../tools/sim ../../tools/sim/sim.cpp $(SRCS) -o sim
	./sim shots && python3 ../../tools/sim/ppm2png.py shots

clean:
	rm -rf ui_test sound_test sim sim_sound shots shots_sound

sound_shots: ../../tools/sim/sim_sound.cpp ../../tools/sim/soundsynth.h $(SRCS)
	$(CXX) -std=c++14 -O1 -I../../src/ui -I../../src/logic -I../../tools/sim ../../tools/sim/sim_sound.cpp $(SRCS) -o sim_sound
	./sim_sound shots_sound && python3 ../../tools/sim/ppm2png.py shots_sound

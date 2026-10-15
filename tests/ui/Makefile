# Host tests for src/ui:  make -C tests/ui test   (also writes screenshots with `make -C tests/ui shots`)
CXX ?= g++
CXXFLAGS = -std=c++14 -g -O1 -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=undefined \
           -I../../src/ui -I../../tools/sim
SRCS = $(wildcard ../../src/ui/*.cpp)

test: ui_test
	./ui_test

ui_test: ui_test.cpp $(SRCS) $(wildcard ../../src/ui/*.h) ../../tools/sim/fakesynth.h
	$(CXX) $(CXXFLAGS) ui_test.cpp $(SRCS) -o $@

shots: ../../tools/sim/sim.cpp $(SRCS)
	$(CXX) -std=c++14 -O1 -I../../src/ui -I../../tools/sim ../../tools/sim/sim.cpp $(SRCS) -o sim
	./sim shots && python3 ../../tools/sim/ppm2png.py shots

clean:
	rm -rf ui_test sim shots

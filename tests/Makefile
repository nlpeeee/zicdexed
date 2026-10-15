# Host-side tests for the pure logic in src/logic (no Circle / MiniDexed needed).
SRC = $(wildcard ../src/logic/*.cpp)
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined

test_logic: test_logic.cpp $(SRC) $(wildcard ../src/logic/*.h)
	$(CXX) $(CXXFLAGS) -o $@ test_logic.cpp $(SRC)

test: test_logic
	./test_logic

clean:
	rm -f test_logic

.PHONY: test clean

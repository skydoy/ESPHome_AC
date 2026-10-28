# Host unit tests for the protocol layer (no ESPHome needed).
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O1 -g

SRC := ../components/hisense_ac/hisense_protocol.cpp test_protocol.cpp

test: test_protocol
	./test_protocol

test_protocol: $(SRC) ../components/hisense_ac/hisense_protocol.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

clean:
	rm -f test_protocol

.PHONY: test clean

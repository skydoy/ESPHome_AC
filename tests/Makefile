# Host unit tests for the protocol layer (no ESPHome needed).
# Locally without g++:  make -C tests CXX="zig c++"
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O1 -g

PROTO := ../components/hisense_proto/hisense_protocol.cpp
PROTO_H := ../components/hisense_proto/hisense_protocol.h
CMD := ../components/hisense_ac/hisense_command.cpp
CMD_H := ../components/hisense_ac/hisense_command.h

test: test_protocol
	./test_protocol

test_protocol: $(PROTO) $(PROTO_H) $(CMD) $(CMD_H) test_protocol.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(PROTO) $(CMD) test_protocol.cpp

clean:
	rm -f test_protocol

.PHONY: test clean

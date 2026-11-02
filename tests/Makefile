# Host unit tests for the protocol layer and the setup probe engine (no ESPHome needed).
# Locally without g++:  make -C tests CXX="zig c++"
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O1 -g

PROTO := ../components/hisense_proto/hisense_protocol.cpp
PROTO_H := ../components/hisense_proto/hisense_protocol.h
CMD := ../components/hisense_ac/hisense_command.cpp
CMD_H := ../components/hisense_ac/hisense_command.h
SETUP := ../components/ac_setup/probe_engine.cpp
SETUP_H := ../components/ac_setup/probe_engine.h

test: test_protocol test_setup
	./test_protocol
	./test_setup

test_protocol: $(PROTO) $(PROTO_H) $(CMD) $(CMD_H) test_protocol.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(PROTO) $(CMD) test_protocol.cpp

# The setup tests link the brand command builders only to prove the allow-list refuses them.
test_setup: $(PROTO) $(PROTO_H) $(CMD) $(CMD_H) $(SETUP) $(SETUP_H) test_setup.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(PROTO) $(CMD) $(SETUP) test_setup.cpp

clean:
	rm -f test_protocol test_setup

.PHONY: test clean

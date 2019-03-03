ALL_CPP=$(shell find -iname "*.cpp")

grpc_lib = -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl

BIN = bin
OUTPUT = output
EXECUTABLE = wj-server
CXX = g++
C_FLAGS := -std=c++11 -Wall -Wextra -g
LIB := -levent -pthread -lhiredis -ljsoncpp -llog4cpp

all:
	mkdir -p $(BIN) && mkdir -p $(OUTPUT)
	$(CXX) $(C_FLAGS) $(ALL_CPP) -o $(BIN)/$(EXECUTABLE) $(LIB)

clean:
	rm -rf $(BIN) $(OUTPUT) vgcore* core

start:
	./$(BIN)/$(EXECUTABLE)
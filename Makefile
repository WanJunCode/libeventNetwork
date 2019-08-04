ALL_CPP=$(shell find -iname "*.cpp")

grpc_lib = -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl

BIN = bin
OUTPUT = output
EXECUTABLE = Cloud-server
CXX = clang++
C_FLAGS := -std=c++11 -Wall -Wextra -ggdb

DEBUG_CFLAGS := -std=c++11 -ggdb -Wall -ffunction-sections -O0 -Wno-format \
-DDEBUG -DMYSQLPP_MYSQL_HEADERS_BURIED -DHAVE_SCHED_GET_PRIORITY_MAX -DLOG4CPP_FIX_ERROR_COLLISION -DLOG4CPP

LIB := -levent -pthread -lhiredis -ljsoncpp -llog4cpp -lmysqlclient

all:
	mkdir -p $(BIN) && mkdir -p $(OUTPUT)
	$(CXX) $(DEBUG_CFLAGS) $(ALL_CPP) -o $(BIN)/$(EXECUTABLE) $(LIB) 

.PHONY: clean start
clean:
	rm -rf $(BIN) $(OUTPUT) vgcore* core .vscode/ipch

start:
	./$(BIN)/$(EXECUTABLE)

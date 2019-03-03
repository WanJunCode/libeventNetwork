vpath %.h
all_name=$(shell find -iname "*.cpp")

other='-L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc` -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed -ldl'

all:
	g++ $(all_name) -g -o wj-server -std=c++11 -levent -pthread -lhiredis -ljsoncpp -llog4cpp

clean:
	rm -f wj-server core log_file.out test_log4cpp1.log vgcore*
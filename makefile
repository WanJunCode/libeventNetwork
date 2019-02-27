vpath %.h

all_name=$(shell find -iname "*.cpp")
all:
	g++ $(all_name) -g -o wj-server -levent -pthread -lhiredis -ljsoncpp -llog4cpp -std=c++11 
	# -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc` -Wl,--no-as-needed -lgrpc++_reflection \
	# -Wl,--as-needed -ldl
.PNONY:clean
clean:
	rm -f wj-server core log_file.out test_log4cpp1.log vgcore*
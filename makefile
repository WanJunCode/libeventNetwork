all_name=$(shell find -iname "*.cpp")

all:
	g++ $(all_name) -g -o server -levent -lhiredis -pthread -ljsoncpp -llog4cpp -std=c++11
.PNONY:clean
clean:
	rm -f server core log_file.out test_log4cpp1.log

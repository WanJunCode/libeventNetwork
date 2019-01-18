all:
	g++ *.cpp -g -o server -levent -pthread -ljsoncpp -llog4cpp -std=c++11
.PNONY:clean
clean:
	rm -f server core

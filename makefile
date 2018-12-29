all:
	g++ *.cpp -g -o server -levent -pthread -ljsoncpp -std=c++11
.PNONY:clean
clean:
	rm -f server core


all:
	g++ *.cpp -g -o server -levent -pthread -ljsoncpp -llog4cpp -std=c++11
clean:
	rm -f server core
	find *log* | xargs -n 1 rm -f

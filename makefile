all:
	g++ *.cpp -o wanjun_server -levent
.PNONY:clean
clean:
	rm -f wanjun_server
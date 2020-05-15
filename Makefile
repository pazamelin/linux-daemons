mydaemon.o: mydaemon.c
	gcc -c -o mydaemon.o mydaemon.c
request.o: request.c
	gcc -c -o request.o request.c
daemon: mydaemon.o request.o
	gcc -o daemon mydaemon.o request.o  -lpthread -lrt
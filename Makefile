all: server client test

client: server.o client.o
	gcc -o client client.o
	
test: server.o test.o
	gcc -o test test.o
	
server: server.o
	gcc -o server server.o -lpthread

server.o: server.c synchronization.h
	gcc -c server.c
	
client.o: client.c synchronization.h
	gcc -c client.c
	
test.o: test.c synchronization.h
	gcc -c test.c
	
clean:
	rm -f *.o client server test

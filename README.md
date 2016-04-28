== Run client with 
./client localhost 5432 2
name of file, address(localhost in example above), port, group number to connect to

== Run server with 
./server

== Build client
gcc client.c -o client -lpthread

== Build server
gcc server.c -o server -lpthread

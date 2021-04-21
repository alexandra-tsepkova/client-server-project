all: server client
	echo "Done"

server: udp_lib_server
	gcc main.c log.c -o server -pthread ./libserver.so

client: udp_lib_client
	gcc client.c -o client ./libclient.so

udp_lib_server:
	gcc -c udp_lib_server.c -o udp_lib_server.o -fPIC
	gcc -shared -o libserver.so udp_lib_server.o
	rm udp_lib_server.o

udp_lib_client:
	gcc -c udp_lib_client.c -o udp_lib_client.o -fPIC
	gcc -shared -o libclient.so udp_lib_client.o
	rm udp_lib_client.o

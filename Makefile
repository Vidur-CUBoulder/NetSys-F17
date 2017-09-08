all:
	gcc udp_client.c -o client
	gcc udp_server.c -o server

tags:
	ctags-exuberant -R ~/GitHub/glibc .

clean:
	rm server client


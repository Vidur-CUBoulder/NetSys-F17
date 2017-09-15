all:
	gcc udp_client.c -lssl -lcrypto -o client
	gcc udp_server.c -o server

tags:
	ctags-exuberant -R ./ ~/GitHub/glibc ~/GitHub/openssl .

clean:
	rm server client

clean-all:
	rm server client tags foo1 foo2 foo3


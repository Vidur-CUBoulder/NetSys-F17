all:
	gcc client.c -lssl -lcrypto -o client
	gcc server.c -lssl -lcrypto -o server

tags:
	ctags-exuberant -R ./ ~/GitHub/glibc ~/GitHub/openssl .

clean:
	rm server client

clean-all:
	rm server client tags foo1 foo2 foo3


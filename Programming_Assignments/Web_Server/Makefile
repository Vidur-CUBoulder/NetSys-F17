all:
	gcc webserver.c -lssl -lcrypto -o server

tags:
	ctags-exuberant -R ./ ~/GitHub/glibc ~/GitHub/openssl .

clean:
	rm server

clean-all:
	rm server client tags


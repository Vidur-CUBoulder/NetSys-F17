all:
	gcc client.c -lssl -lcrypto -o client

tags:
		ctags-exuberant -R ./ ~/GitHub/glibc ~/GitHub/openssl .

clean:
		rm client

clean-all:
		rm client tags

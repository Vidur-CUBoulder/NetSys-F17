all:
	gcc web_proxy.c -o web_proxy -lpthread -lssl -lcrypto

tags:
	ctags-exuberant -R ./ ~/GitHub/glibc ~/GitHub/openssl .

clean:
	rm web_proxy


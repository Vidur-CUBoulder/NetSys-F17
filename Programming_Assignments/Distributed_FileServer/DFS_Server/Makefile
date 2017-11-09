all:
	gcc server.c -lssl -lcrypto -o server 

tags:
		ctags-exuberant -R ./ ~/GitHub/glibc ~/GitHub/openssl .

clean:
		rm server
		rm -rvf DFS*/alice*

clean-all:
		rm server tags

all:
	gcc server.c -lssl -lcrypto -o server 

tags:
		ctags-exuberant -R ./ ~/GitHub/glibc ~/GitHub/openssl .

clean:
		rm server
		rm DFS1/chunk_* 
		rm DFS2/chunk_* 
		rm DFS3/chunk_* 
		rm DFS4/chunk_* 

clean-all:
		rm server tags

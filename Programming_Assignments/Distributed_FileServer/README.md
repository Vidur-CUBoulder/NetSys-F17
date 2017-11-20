## Distributed File Server

### SERVER

- First run `make`.

- Then, to run the server instances:
  ``` 
  ./server dfs.conf DFS1 9001
  ```
- __dfs.conf__ is the name of the config file that contains the username and the password in the following format:
  ```
    <username> <password>
  ```
- __DFS1__ is the name of the pre-defined server directory.
- __9001__ is the port number for that server instance. 

##### NOTE:
      There is only one executable for all the servers and the differenciation is made on the basis of the command line arguments passed to the executable.

### CLIENT

- First, run `make`.

- Then, run the client instance in the following manner:
  ```
  ./client dfc.conf
  ```

- __dfc.conf__ is the name of the client config file that contains the username and the passoword in the following format:
  ```
  Server DFS1 127.0.0.1:9001
  Server DFS2 127.0.0.1:9002
  Server DFS3 127.0.0.1:9003
  Server DFS4 127.0.0.1:9004
  Username: alice 
  Password: temppwd
  ```

- The rest of the information in the file is for the servers, their IP addresses and port numbers.

##### NOTE:
  There are a few bugs in this implementation and corner cases that would cause outright failure and exit from the program.



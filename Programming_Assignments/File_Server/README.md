# UDP File Server

This is a very basic implementation of a UDP bases File Server. There is an attempt to make the transfer as reliable as possible. 

## Compiling The Programs

Compiling the Server Program:-
* Go to **Server/** and run:
```
make
```
This should give you an executatble **client**.

Compiling the Client Program:-
* Go to **Client/** and run:
```
make
```
This should give you an executatble **server**.

Cleanup:-
```
make clean
```
This will clean the current directory of all executables.

## Client Program:

* The Client program starts off by running the command line and asking the user for his/her choice of commands. The user can type *help* to get a list of all the valid commands.
* On receiving a valid command from the user, the client will relay that to the server that then accordingly executes it. It will return or print an error on failure. 

## Server Program:

* The Server waits to get a command from the client and authenticates its validity before processing the request.

## Reliability:

### Sender:

```c
      sendto_wrapper(sock_fd, *remote, &(data_packet.data_buffer),\
                    sizeof(data_packet.data_buffer));
    
      /* Now, wait for the ack */
      nbytes = recvfrom_wrapper(sock_fd, *remote, &ack_seq_num, sizeof(ack_seq_num));
         
      if(nbytes < 0) 
      {
        /* Re-send the packet */
        nbytes = sendto_wrapper(sock_fd, *remote, &(data_packet.data_buffer),\
                    sizeof(data_packet.data_buffer));
        
        /* Now, wait for the ack */
        nbytes = recvfrom_wrapper(sock_fd, *remote, &ack_seq_num, sizeof(ack_seq_num));
        memcpy(ack_seq_num, &data_packet.data_buffer.seq_number,\
            sizeof(data_packet.data_buffer.seq_number));
      }

```

### Receiver:

```c

    recvfrom_wrapper(sock_fd, *remote, &data_packet.data_buffer, \
                  sizeof(data_packet.data_buffer));
  
    /* Send the ack/nack */
    data_packet.ack_nack = ACK;
    memcpy(ack_seq_num, &data_packet.data_buffer.seq_number,\
              sizeof(data_packet.data_buffer.seq_number));
    ack_seq_num[4] = data_packet.ack_nack;
    
    nbytes = sendto_wrapper(sock_fd, *remote, &(ack_seq_num),\
                    sizeof(ack_seq_num));
    
    if(data_packet.data_buffer.seq_number == (packet_count - 1)) {
        /* this implies the prev. packet; reverse and don't write to the file*/
        continue;
    }

```

The 2 cases that are handled for reliablity are:
1. Sender packet is dropped and never reaches the receiver.
2. ACK from the receiver is lost and never reaches the sender. 

In both the above cases, the sender will resend the packet and wait for the receiver to respond with a valid ACK. The ACK includes the seqence number against which the sender check for the validity of the ACK.

There are certain cases in which this implementation fails. An example of that is as follows:-  
_If 2 consequtive ACKs are dropped and both recvfrom functions timeout. This would mean that the packet would be completly dropped from the receiver end and effectively corrupt the file. Multiple cascading failures that occur one after another will casue issues with the current implemenation as that isn't accounted for._

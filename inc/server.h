#ifndef SERVER_H
#define SERVER_H

#include<connection_group.h>

#include<tcp_server_handler.h>
#include<udp_server_handler.h>

#define INVALID_THREAD_COUNT 59985923
#define INVALID_PROTOCOL     59489213

// here conn_fd, is the connection file discriptor, you can read and write to this discriptor, to communicate
// the client_addr, will hold the socket address of the client, we will this aswell to you
// returns the file discriptor to the tcp or udp socket, on which the server is listening to
// in case of UDP server you get a fixed thread pool executor of thread_count threads
// in case of TCP server you get a cached thread pool executor of max thread_count threads
int serve(connection_group* conn_grp_p, void (*handler)(int conn_fd), unsigned int thread_count, volatile int* listen_fd_p);

// stops the server that was started using the serve function
// it closes the file discriptor, hence the connection, here the parameter fd is the file discriptor of the listenning socket
int server_stop(int listen_fd);

#endif
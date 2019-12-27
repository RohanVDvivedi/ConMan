#ifndef SERVER_H
#define SERVER_H

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<signal.h>

#include<executor.h>

#define DEFAULT_MAXIMUM_THREADS_IO_HANDLER 200											// maximum 200 threads for IO
#define DEFAULT_NO_CONNECTION_THREAD_DESTROY_TIMEOUT_IN_MICRO_SECONDS 500 * 1000		// half a second
#define DEFAULT_BACKLOG_QUEUE_SIZE 10													// we do not ask the operating system to queue any more than 10 awaiting connections

typedef struct server server;
struct server
{
	int TRANSMISSION_PROTOCOL_TYPE;
	sa_family_t ADDRESS_FAMILY;
	uint32_t SERVER_ADDRESS;
	uint16_t PORT;

	void (*connection_handler)(int conn_fd);

	int listenfd;

	hashmap* thread_to_connection_mapping;
};

// here conn_fd, is the connection file discriptor, you can read and write to this discriptor, to communicate
// returns the failure response, in starting the server
// returns the file discriptor to the tcp or udp socket, on which the server is listening to
int serve_default(sa_family_t ADDRESS_FAMILY, int TRANSMISSION_PROTOCOL_TYPE, uint32_t SERVER_ADDRESS, uint16_t PORT, void (*connection_handler)(int conn_fd));
int serve(sa_family_t ADDRESS_FAMILY, int TRANSMISSION_PROTOCOL_TYPE, uint32_t SERVER_ADDRESS, uint16_t PORT, unsigned long long int BACKLOG_QUEUE_SIZE, void (*connection_handler)(int conn_fd));

// stops the server that was started using the serve function
// it closes the file discriptor, hence the connection, here the parameter fd is the file discriptor of the listenning socket
int server_stop(int fd);


/*
	above function is very complicated with a log of parameters, so here below are a few smaller function for the server
*/

int serve_tcp_on_ipv4(uint16_t PORT, void (*connection_handler)(int conn_fd));

int serve_tcp_on_ipv6(uint16_t PORT, void (*connection_handler)(int conn_fd));

int serve_udp_on_ipv4(uint16_t PORT, void (*datagram_handler)(int serv_fd));

int serve_udp_on_ipv6(uint16_t PORT, void (*datagram_handler)(int serv_fd));

#endif

// Examples for 
/*
TCP connection handler
do not close any file discriptors here, just return when you eant to exit the current client connection and close it
server does not shutdown by returning, from here, you return from here to exit this client connection only, other clients may be still being served

void (*connection_handler)(int conn_fd)
{
	char buffer[1000];
	while(1)
	{
		int buffreadlength = recv(conn_fd, buffer, 999, 0);
		if(buffreadlength == -1)
		{
			break;
		}

		buffer[buffreadlength] = '\0';

		// process the buffer here

		buffreadlength = strlen(buffer);
		send(conn_fd, buffer, buffreadlength, 0);
	}
}

UDP datagram handler
do not close any file discriptors here, just return when you want to exit the server, and stop serving altogether

void (*datagram_handler)(int serv_fd)
{
	char buffer[1000];
	while(1)
	{
		struct sockaddr_in cliaddr; socklen_t cliaddrlen = sizeof(cliaddr);
		int buffreadlength = recvfrom(serv_fd, buffer, 999, 0, &cliaddr, &cliaddrlen);
		if(buffreadlength == -1)
		{
			break;
		}

		buffer[buffreadlength] = '\0';

		// process the buffer here

		buffreadlength = strlen(buffer);
		sendto(serv_fd, buffer, buffreadlength, 0, &cliaddr, cliaddrlen); 
	}
}

*/
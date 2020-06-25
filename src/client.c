#include<client.h>

// returns 0 if successfull, handling of the connection was successfull
int connect_to(connection_group* conn_grp_p, void (*handler)(int fd))
{
	int err = make_connection(conn_grp_p);
	if(err == -1)
    	return err;
    int fd = err;

	// pass the file discriptor to the handler, so that request can be handled
	handler(fd);

	err = close(fd);
	if(err == -1)
    	return err;

	return 0;
}

// the sub functions, that make up the connect_to function

// returns file-discriptor to the socket, through which client connection has been made
int make_connection(connection_group* conn_grp_p)
{
	// then we try to set up socket and retrieve the file discriptor that is returned
	int err = socket(conn_grp_p->ADDRESS.sin_family, conn_grp_p->PROTOCOL, 0);
    if(err == -1)
    	return err;
    int fd = err;

    // then we set up socket address with the address received from the host using the get host name function 
	struct sockaddr_in server_addr = conn_grp_p->ADDRESS;

	// next we try and attempt to connect the socket formed whose file discriptor we have
	// to connect using the address that we have in sockaddr_in struct in server_addr
	err = connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if(err == -1)
		return err;

	return fd;
}
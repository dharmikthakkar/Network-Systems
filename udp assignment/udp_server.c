#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100
#define PACKET_SIZE 1000


void receive_file_from_client(unsigned char file_name[], int socket_n, struct sockaddr_in client_st)
{
	char read_file[PACKET_SIZE];
	char recv_done;
	char server_response[] = "orange";	
	size_t write_bytes;
	int nbytes;
	int addr_length = sizeof(client_st);
	FILE *fp;
	fp = fopen(file_name, "w");
	//printf("\n\r%s\n\r", (char *)fp);
	while(!recv_done){
		nbytes = recvfrom(socket_n, read_file, PACKET_SIZE, 0, (struct sockaddr *)&client_st, &addr_length);
		printf("\n\rPackets received = %d\n\r", nbytes);
		if(nbytes < PACKET_SIZE) recv_done = 1; 
		write_bytes = fwrite(read_file, nbytes, sizeof(char), fp);
		//printf("\n\rWritten bytes = %d\n\r", (int)write_bytes);
		nbytes = sendto(socket_n, server_response, strlen(server_response), 0, (struct sockaddr *)&client_st, sizeof(client_st));
		bzero(read_file,sizeof(read_file));
	}	
	recv_done = 0;
	fclose(fp);
}




void send_file_to_client(unsigned char file_name[], int socket_n, struct sockaddr_in client_st)
{
	char read_file[PACKET_SIZE];
	char client_response[10];	
	size_t read_bytes;
	int nbytes;
	int addr_length = sizeof(client_st);
	FILE *fp;
	fp = fopen(file_name, "r");
	//printf("\n\r%s\n\r", (char *)fp);
	while(!feof(fp)){
		read_bytes = fread(read_file, sizeof(char), PACKET_SIZE, fp);
		printf("\n\rRead bytes = %d\n\r", (int)read_bytes);
		nbytes = sendto(socket_n, read_file, read_bytes, 0, (struct sockaddr *)&client_st, sizeof(client_st));
		printf("\n\rPackets sent = %d\n\r", nbytes);
		nbytes = recvfrom(socket_n, client_response, 10, 0, (struct sockaddr *)&client_st, &addr_length);  
		printf("\n\rServer says %s\n\r", client_response);
		bzero(read_file,sizeof(read_file));
		bzero(client_response,sizeof(client_response));
	}	
	//nbytes = sendto(socket_n, file_done, strlen(read_file), 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
	fclose(fp);
}


int main (int argc, char * argv[] )
{


	int sock, i;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);

	//waits for an incoming message
	bzero(buffer,sizeof(buffer));
	nbytes = nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);

	printf("The client says %s\n", buffer);

	char msg[] = "orange";
	nbytes = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&remote, sizeof(remote));

//	receive_file_from_client("/home/djdharmik/Downloads/udp/server_files/foo1", sock, remote);
//	receive_file_from_client("/home/djdharmik/Downloads/udp/server_files/foo2", sock, remote);
//	receive_file_from_client("/home/djdharmik/Downloads/udp/server_files/foo3", sock, remote);
	receive_file_from_client("/home/djdharmik/Downloads/udp/server_files/ProblmSet1.docx", sock, remote);

//	send_file_to_client("/home/djdharmik/Downloads/udp/server_files/foo1", sock, remote);
//	send_file_to_client("/home/djdharmik/Downloads/udp/server_files/foo2", sock, remote);
//	send_file_to_client("/home/djdharmik/Downloads/udp/server_files/foo3", sock, remote);
	send_file_to_client("/home/djdharmik/Downloads/udp/server_files/ProblmSet1.docx", sock, remote);

	
	close(sock);
}


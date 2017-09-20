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
#include <errno.h>

#define MAXBUFSIZE 100
#define PACKET_SIZE 1000


/* You will have to modify the program below */



void send_file_to_server(unsigned char file_name[], int socket_n, struct sockaddr_in server_struct)
{
	char read_file[PACKET_SIZE];
	char server_response[10];	
	size_t read_bytes;
	int nbytes;
	int addr_length = sizeof(server_struct);
	FILE *fp;
	fp = fopen(file_name, "r");
	printf("\n\r%s\n\r", (char *)fp);
	while(!feof(fp)){
		read_bytes = fread(read_file, sizeof(char), PACKET_SIZE, fp);
		printf("\n\rRead bytes = %d\n\r", (int)read_bytes);
		nbytes = sendto(socket_n, read_file, read_bytes, 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
		printf("\n\rPackets sent = %d\n\r", nbytes);
		nbytes = recvfrom(socket_n, server_response, 10, 0, (struct sockaddr *)&server_struct, &addr_length);  
		printf("\n\rServer says %s\n\r", server_response);
		bzero(read_file,sizeof(read_file));
		bzero(server_response,sizeof(server_response));
	}	
	//nbytes = sendto(socket_n, file_done, strlen(read_file), 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
	fclose(fp);
}

void receive_file_from_server(unsigned char file_name[], int socket_n, struct sockaddr_in server_struct){
	char read_file[PACKET_SIZE];
	char recv_done;
	char client_response[] = "apple";	
	size_t write_bytes;
	int nbytes;
	int addr_length = sizeof(server_struct);
	FILE *fp;
	fp = fopen(file_name, "w");
	printf("\n\r%s\n\r", (char *)fp);
	while(!recv_done){
		nbytes = recvfrom(socket_n, read_file, PACKET_SIZE, 0, (struct sockaddr *)&server_struct, &addr_length);
		printf("\n\rPackets received = %d\n\r", nbytes);
		if(nbytes < PACKET_SIZE) recv_done = 1; 
		write_bytes = fwrite(read_file, nbytes, sizeof(char), fp);
		//printf("\n\rWritten bytes = %d\n\r", (int)write_bytes);
		nbytes = sendto(socket_n, client_response, strlen(client_response), 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
		bzero(read_file,sizeof(read_file));
	}	
	recv_done = 0;
	fclose(fp);

}

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];

	struct sockaddr_in remote, server;              //"Internet socket address structure"
	
/* file operations debugging begin*/

	

/* file operations debugging end*/

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	/******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	char command[] = "apple";	
	nbytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *)&remote, sizeof(remote));

	// Blocks till bytes are received
	//struct sockaddr_in from_addr;
	int addr_length = sizeof(server);
	bzero(buffer,sizeof(buffer));
	nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&server, &addr_length);  

	printf("Server says %s\n", buffer);
	
//	send_file_to_server("/home/djdharmik/Downloads/udp/foo1", sock, remote);
//	send_file_to_server("/home/djdharmik/Downloads/udp/foo2", sock, remote);
//	send_file_to_server("/home/djdharmik/Downloads/udp/foo3", sock, remote);
	send_file_to_server("/home/djdharmik/Downloads/udp/ProblmSet1.docx", sock, remote);

//	receive_file_from_server("/home/djdharmik/Downloads/udp/client_files/foo1", sock, remote);
//	receive_file_from_server("/home/djdharmik/Downloads/udp/client_files/foo2", sock, remote);
//	receive_file_from_server("/home/djdharmik/Downloads/udp/client_files/foo3", sock, remote);
	receive_file_from_server("/home/djdharmik/Downloads/udp/client_files/ProblemSet1.docx", sock, remote);

	close(sock);

}


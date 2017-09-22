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

signed char compare_packets(char packet1[], char packet2[]){
	int bit_i = 0;
	for(bit_i = 0; bit_i<PACKET_SIZE; bit_i++){
		//printf("\n\rpacket1[bit_i] = %d\tpacket2[bit_i] = %d", packet1[bit_i], packet2[bit_i]);
		if(packet1[bit_i] != packet2[bit_i]) break;
	}
	//printf("\n\rbit_i=%d", bit_i);
	if(bit_i == PACKET_SIZE) return 0;
	else return -1;
}

void receive_file_from_client(unsigned char file_name[], int socket_n, struct sockaddr_in client_st)
{
	struct timeval tv;
	char read_file[PACKET_SIZE];
	char read_file_2[PACKET_SIZE];
	char recv_done = 0;
	char server_response[] = "orange";	
	size_t write_bytes;
	int nbytes;
	int addr_length = sizeof(client_st);
	int j = 0;
	int i = 0;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	char timeout_test = 0;
	FILE *fp;
	fp = fopen(file_name, "w");
	//printf("\n\r%s\n\r", (char *)fp);
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	while(!recv_done){
		if(timeout_test == 1){
			usleep(900000);
			timeout_test = 2;
		}		
		nbytes = recvfrom(socket_n, read_file_2, PACKET_SIZE, 0, (struct sockaddr *)&client_st, &addr_length);
		printf("\n\rPacket received");
		if(compare_packets(read_file, read_file_2) < 0){
			printf("\n\rValid New Packet received = %d", i++);
			for(j=0; j<PACKET_SIZE; j++){
				read_file[j] = read_file_2[j];
			}		
		}
		else printf("\n\rPacket same as previous packet. Packet discarded");
		if(nbytes < PACKET_SIZE) recv_done = 1; 
		write_bytes = fwrite(read_file, nbytes, sizeof(char), fp);
		//printf("\n\rpacket1[bit_i] = %d\tpacket2[bit_i] = %d", read_file[0], read_file_2[0]);
		//printf("\n\rWritten bytes = %d\n\r", (int)write_bytes);
		nbytes = sendto(socket_n, server_response, strlen(server_response), 0, (struct sockaddr *)&client_st, sizeof(client_st));
		//bzero(read_file,sizeof(read_file));
		bzero(read_file_2,sizeof(read_file_2));
		//timeout_test++;
	}	
	recv_done = 0;
	fclose(fp);
}




void send_file_to_client(unsigned char file_name[], int socket_n, struct sockaddr_in client_st)
{
	struct timeval tv;
	char read_file[PACKET_SIZE];
	char client_response[10];	
	size_t read_bytes;
	int nbytes;
	int addr_length = sizeof(client_st);
	int rv;
	int i = 0;
	char resend = 0;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	FILE *fp;
	fp = fopen(file_name, "r");
	//printf("\n\r%s\n\r", (char *)fp);
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	while(!feof(fp)){
		if(resend == 0){
			read_bytes = fread(read_file, sizeof(char), PACKET_SIZE, fp);
			printf("\n\rRead bytes = %d\n\r", (int)read_bytes);
		}
		//while(1);
		nbytes = sendto(socket_n, read_file, read_bytes, 0, (struct sockaddr *)&client_st, sizeof(client_st));
		printf("\n\rPacket sent = %d\n\r", nbytes);
		nbytes = recvfrom(socket_n, client_response, 10, 0, (struct sockaddr *)&client_st, &addr_length);  
		if(nbytes < 0 && errno == EAGAIN){
			printf("\n\rACK timeout error\n\r");
			resend = 1;
			i--;	
		}
		else{
			printf("\n\rClient says %s\n\r", client_response);
			bzero(read_file,sizeof(read_file));
			bzero(client_response,sizeof(client_response));
			resend = 0;
		}
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

	receive_file_from_client("/home/djdharmik/Downloads/udp/server_files/foo1", sock, remote);
	receive_file_from_client("/home/djdharmik/Downloads/udp/server_files/foo2", sock, remote);
	receive_file_from_client("/home/djdharmik/Downloads/udp/server_files/foo3", sock, remote);
	//receive_file_from_client("/home/djdharmik/Downloads/udp/server_files/ProblmSet1.docx", sock, remote);

	send_file_to_client("/home/djdharmik/Downloads/udp/server_files/foo1", sock, remote);
	send_file_to_client("/home/djdharmik/Downloads/udp/server_files/foo2", sock, remote);
	send_file_to_client("/home/djdharmik/Downloads/udp/server_files/foo3", sock, remote);
	//send_file_to_client("/home/djdharmik/Downloads/udp/server_files/ProblmSet1.docx", sock, remote);

	
	close(sock);
}


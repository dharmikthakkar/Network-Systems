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


/* You will have to modify the program below */
char * send_packet(char packet[],  int socket_n, struct sockaddr_in server_struct, char reliability){
	struct timeval tv;
	char server_response[10];
	int nbytes;
	int addr_length = sizeof(server_struct);
	int rv;
	char resend = 0;
	if(reliability == 1){
		tv.tv_sec = 0;
		tv.tv_usec = 300000;
	}
	else{
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}		
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	do{
		nbytes = sendto(socket_n, packet, PACKET_SIZE, 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
		printf("\n\rPacket sent %s of %d bytes\n\r", packet, nbytes);
		nbytes = recvfrom(socket_n, server_response, 10, 0, (struct sockaddr *)&server_struct, &addr_length);  
		if(nbytes < 0 && errno == EAGAIN && reliability == 1){
			printf("\n\rACK timeout error\n\r");
			resend = 1;
		}
	//		printf("\n\rbytes received = %d\n\r", nbytes);		
		else{
			printf("\n\rServer says %s\n\r", server_response);
			resend = 0;			
		}	
	}while(resend == 1);
	bzero(server_response,sizeof(server_response));	
}

char * receive_packet(int socket_n, struct sockaddr_in server_struct, char reliability){
	struct timeval tv;
	static char packet[PACKET_SIZE];
	char packet_2[PACKET_SIZE];
	char recv_done = 0;
	char client_response[] = "apple";	
	int nbytes;
	int addr_length = sizeof(server_struct);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	char temp = 0;
	int rv;
	printf("\n\rpacket = %s\n\r", packet);
	//printf("\n\r%s\n\r", (char *)fp);
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	nbytes = recvfrom(socket_n, packet_2, PACKET_SIZE, 0, (struct sockaddr *)&server_struct, &addr_length);	
	printf("\n\rPacket received = %s of %d bytes\n\r", packet_2, nbytes);
	if(reliability == 0) return packet;
	if(compare_packets(packet, packet_2) < 0){
		printf("\n\rValid New Packet received\n\r");
		recv_done = 1;
		for(int j=0; j<PACKET_SIZE; j++){
			packet[j] = packet_2[j];
		}		
	}
	else printf("\n\rPacket same as previous packet. Packet discarded");
	printf("\n\rpacket = %s\n\r", packet);
	usleep(900000);
	nbytes = sendto(socket_n, client_response, strlen(client_response), 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
	bzero(packet_2,sizeof(packet_2));
	if(recv_done == 1) return packet;
	else return NULL;
	
}


void list_file(char dir_name[],  int socket_n, struct sockaddr_in server_struct){
	struct timeval tv;
	char server_response[10];
	size_t read_bytes;
	int nbytes;
	int addr_length = sizeof(server_struct);
	int rv;
	int j = 0;
	char resend = 0;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	do{
		nbytes = sendto(socket_n, dir_name, PACKET_SIZE, 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
		printf("\n\rPacket sent\n\r");
		nbytes = recvfrom(socket_n, server_response, 10, 0, (struct sockaddr *)&server_struct, &addr_length);  
		if(nbytes < 0 && errno == EAGAIN){
			printf("\n\rACK timeout error\n\r");
			resend = 1;
		}
	//		printf("\n\rbytes received = %d\n\r", nbytes);		
		else{
			printf("\n\rServer says %s\n\r", server_response);
			resend = 0;			
		}	
	}while(resend == 1);
//	if(server_response != "NODIR"){
//		for(j = 0; j < num_files; j++){
//			
//			
//		}
//	}

}

//extern int errno;


void send_file_to_server(unsigned char file_name[], int socket_n, struct sockaddr_in server_struct)
{
	struct timeval tv;
	char read_file[PACKET_SIZE];
	char server_response[10];	
	size_t read_bytes;
	int nbytes;
	int addr_length = sizeof(server_struct);
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
		nbytes = sendto(socket_n, read_file, read_bytes, 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
		printf("\n\rPacket sent = %d\n\r", i++);
		nbytes = recvfrom(socket_n, server_response, 10, 0, (struct sockaddr *)&server_struct, &addr_length);  
		if(nbytes < 0 && errno == EAGAIN){
			printf("\n\rACK timeout error\n\r");
			resend = 1;
			i--;	
		}
//		printf("\n\rbytes received = %d\n\r", nbytes);		
		else{
			printf("\n\rServer says %s\n\r", server_response);
			bzero(read_file,sizeof(read_file));
			bzero(server_response,sizeof(server_response));
			resend = 0;			
		}	
	}	
	//nbytes = sendto(socket_n, file_done, strlen(read_file), 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
	fclose(fp);
}

void receive_file_from_server(unsigned char file_name[], int socket_n, struct sockaddr_in server_struct){
	struct timeval tv;
	char read_file[PACKET_SIZE];
	char read_file_2[PACKET_SIZE];
	char recv_done = 0;
	char client_response[] = "apple";	
	size_t write_bytes;
	int nbytes;
	int addr_length = sizeof(server_struct);
	int j = 0;
	int i = 0;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	char temp = 0;
	int rv;
	char timeout_test = 0;
	FILE *fp;
	fp = fopen(file_name, "w");
	//printf("\n\r%s\n\r", (char *)fp);
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	while(!recv_done){
		nbytes = recvfrom(socket_n, read_file_2, PACKET_SIZE, 0, (struct sockaddr *)&server_struct, &addr_length);
		if(nbytes < 0 && errno == EAGAIN){
			printf("\n\rACK timeout error\n\r");
		}		
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
		//printf("\n\rWritten bytes = %d\n\r", (int)write_bytes);
		//while(1);
		nbytes = sendto(socket_n, client_response, strlen(client_response), 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
		bzero(read_file_2,sizeof(read_file_2));
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
	char *message_received;
	// Blocks till bytes are received
	//struct sockaddr_in from_addr;
	int addr_length = sizeof(server);
	bzero(buffer,sizeof(buffer));
	nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&server, &addr_length);  
	printf("Server says %s\n", buffer);

	send_packet("04", sock, remote, 1);
	send_packet("abc", sock, remote, 1);
	send_packet("xyz", sock, remote, 1);
	send_packet("def", sock, remote, 1);
	send_packet("pqr", sock, remote, 1);



	printf("\n\rdone\n\r");		
	while(1);
	send_file_to_server("/home/djdharmik/Downloads/udp/foo1", sock, remote);
	send_file_to_server("/home/djdharmik/Downloads/udp/foo2", sock, remote);
	send_file_to_server("/home/djdharmik/Downloads/udp/foo3", sock, remote);
	//send_file_to_server("/home/djdharmik/Downloads/udp/ProblmSet1.docx", sock, remote);

	receive_file_from_server("/home/djdharmik/Downloads/udp/client_files/foo1", sock, remote);
	receive_file_from_server("/home/djdharmik/Downloads/udp/client_files/foo2", sock, remote);
	receive_file_from_server("/home/djdharmik/Downloads/udp/client_files/foo3", sock, remote);
	//receive_file_from_server("/home/djdharmik/Downloads/udp/client_files/ProblmSet1.docx", sock, remote);

	close(sock);

}


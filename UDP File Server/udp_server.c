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
#include <dirent.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100
#define PACKET_SIZE 1000


signed char compare_packets(char packet1[], char packet2[]){
	int bit_i = 0;
	for(bit_i = 0; bit_i<PACKET_SIZE; bit_i++){
		if(packet1[bit_i] != packet2[bit_i]) break;
	}
	if(bit_i == PACKET_SIZE) return 0;
	else return -1;
}

/* You will have to modify the program below */
char * send_packet(char packet[],  int socket_n, struct sockaddr_in client_st, char reliability){
	struct timeval tv;
	char client_response[10];
	int nbytes;
	int addr_length = sizeof(client_st);
	int rv;
	static int packet_i=0;
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
		nbytes = sendto(socket_n, packet, PACKET_SIZE, 0, (struct sockaddr *)&client_st, sizeof(client_st));
		printf("\n\rPacket sent = %d", packet_i++);
		nbytes = recvfrom(socket_n, client_response, 10, 0, (struct sockaddr *)&client_st, &addr_length);  
		if(nbytes < 0 && errno == EAGAIN && reliability == 1){
			printf("\n\rACK timeout error");
			resend = 1;
			packet_i--;
		}
		else{
			printf("\n\rClient says %s", client_response);
			resend = 0;			
		}	
	}while(resend == 1);
	bzero(client_response,sizeof(client_response));	
}

char * receive_packet(int socket_n, struct sockaddr_in client_st, char reliability){
	struct timeval tv;
	static char packet[PACKET_SIZE];
	static char packet_2[PACKET_SIZE];
	char recv_done = 0;
	char server_response[] = "apple";	
	static int packet_i = 0;
	int nbytes;
	int addr_length = sizeof(client_st);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	int rv;
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	bzero(packet_2,sizeof(packet_2));
	nbytes = recvfrom(socket_n, packet_2, PACKET_SIZE, 0, (struct sockaddr *)&client_st, &addr_length);	
	printf("\n\rPacket received = %d", packet_i++);
	if(reliability == 0 || strcmp(packet, "eof") == 0){
		bzero(packet, sizeof(packet));
		packet_i = 0;
		nbytes = sendto(socket_n, server_response, strlen(server_response), 0, (struct sockaddr *)&client_st, sizeof(client_st));
		return packet_2;
	}
	if(compare_packets(packet, packet_2) < 0){
		printf("\n\rValid New Packet received");
		recv_done = 1;
		for(int j=0; j<PACKET_SIZE; j++){
			packet[j] = packet_2[j];
		}		
	}
	else{
		printf("\n\rPacket same as previous packet. Packet discarded");
		packet_i--;
	}	
	nbytes = sendto(socket_n, server_response, strlen(server_response), 0, (struct sockaddr *)&client_st, sizeof(client_st));
	bzero(packet_2,sizeof(packet_2));
	if(recv_done == 1) return packet;
	else return NULL;
	
}


char *(*list_files(char dir_name[], int socket_n, struct sockaddr_in client_st)){
	struct timeval tv;
	int i=0;
	int j=0;
	int rv;
	char resend = 0;
	int nbytes;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	int addr_length = sizeof(client_st);
	static char * files[200];
	struct dirent *de;
	DIR *dir = opendir(dir_name);
	if(dir == NULL){
		printf("\n\rCould not open directory\n\r");
		return NULL;
	}
	while((de = readdir(dir)) != NULL){
		printf("\n\r%s", de->d_name);
		(*(files +i)) = de->d_name;
		i++;	
	}
	printf("\n");
	printf("\n\rNumber of files in the directory=%d\n\r", i);
	closedir(dir);
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	send_packet("sof", socket_n, client_st, 1);
	for(j=0; j<i; j++){
		send_packet(files[j], socket_n, client_st, 1);
	}
	send_packet("eof", socket_n, client_st, 1);
	return files;
}


void receive_file_from_client(unsigned char file_name[], int socket_n, struct sockaddr_in client_st)
{
	struct timeval tv;
	char read_file[PACKET_SIZE];
	char read_file_2[PACKET_SIZE];
	char recv_done = 0;
	char server_response[10];	
	size_t write_bytes;
	char * message_received;
	int nbytes;
	int addr_length = sizeof(client_st);
	int j = 0;
	int i = 0;
	int rv;
	int temp_write=0;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FILE *fp;
	fp = fopen(file_name, "w");
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	bzero(read_file, sizeof(read_file));
	bzero(read_file_2, sizeof(read_file_2));
	while(!recv_done){
		nbytes = recvfrom(socket_n, read_file_2, PACKET_SIZE, 0, (struct sockaddr *)&client_st, &addr_length);
		if(nbytes >= 0 && strcmp(read_file_2, "apple") != 0){
			printf("\n\rPacket received of %d", nbytes);
			if(strcmp(read_file_2, "eof") != 0){
				if(compare_packets(read_file, read_file_2) < 0){
					printf("\n\rValid New Packet received = %d of %d", i++, nbytes);
					for(j=0; j<PACKET_SIZE; j++){
						read_file[j] = read_file_2[j];
					}	
					write_bytes = fwrite(read_file, nbytes - 1, sizeof(char), fp);
					printf("\n\rWritten Bytes = %ld | %d times", write_bytes, temp_write++);	
				}
				else{
					printf("\n\rPacket same as previous packet. Packet discarded");
				}
				server_response[0] = (i%100) - 1;
				nbytes = sendto(socket_n, server_response, 10, 0, (struct sockaddr *)&client_st, sizeof(client_st));
			}
			else{
				printf("\n\rReceive done!"); 
				recv_done = 1; 
				nbytes = sendto(socket_n, "eof", 10, 0, (struct sockaddr *)&client_st, sizeof(client_st));
//				do{
//					send_packet("eof", socket_n, client_st, 1);
//					message_received = receive_packet(socket_n, client_st, 0);
//				}while(strcmp(message_received, "done") != 0);	
			}
		}				
		else{
			printf("\n\rReceive error!");
		}
		bzero(read_file_2,sizeof(read_file_2));
	}	
	recv_done = 0;
	fclose(fp);
}




void send_file_to_client(unsigned char file_name[], int socket_n, struct sockaddr_in client_st)
{
	struct timeval tv;
	char read_file[PACKET_SIZE];
	char read_file_2[PACKET_SIZE];
	char client_response[10];	
	size_t read_bytes;
	int nbytes;
	char * message_received;
	int addr_length = sizeof(client_st);
	int rv;
	int i = 0;
	unsigned char id=0;
	char resend = 0;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	FILE *fp;
	fp = fopen(file_name, "r");
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	while(!feof(fp)){
		if(resend == 0){
			read_bytes = fread(read_file_2, sizeof(char), PACKET_SIZE - 1, fp);
			printf("\n\rRead bytes = %d", (int)read_bytes);
			read_file_2[read_bytes] = id%2;
			id++;
			printf("\n\rlast byte of read_file_2 = %d", read_file_2[read_bytes]);
			if(compare_packets(read_file, read_file_2) < 0){
				for(int temp_f=0; temp_f<PACKET_SIZE; temp_f++){
					read_file[temp_f] = read_file_2[temp_f];
				}				
			}
			else{
				printf("\n\rPacket same as previous packet. Read repeat\n\r");
				while(1);
			}
		}
		nbytes = sendto(socket_n, read_file, read_bytes + 1, 0, (struct sockaddr *)&client_st, sizeof(client_st));
		printf("\n\rPacket sent = %d", i++);
		nbytes = recvfrom(socket_n, client_response, 10, 0, (struct sockaddr *)&client_st, &addr_length);  
		if(nbytes < 0 && errno == EAGAIN){
			printf("\n\rACK timeout error");
			resend = 1;
			i--;	
		}
		else if(client_response[0] != ((i%100)-1)){
			resend = 1;
			printf("\n\rServer response = %d resending..", client_response[0]);
			i--;
		}
		else{
			printf("\n\rClient says %d\n\r", client_response[0]);
			//bzero(read_file,sizeof(read_file));
			bzero(client_response,sizeof(client_response));
			resend = 0;
		}
	}
	send_packet("eof", socket_n, client_st, 1);
//	do{
//		message_received = receive_packet(socket_n, client_st, 0);
//	}while(strcmp(message_received, "eof") != 0);
//	send_packet("done", socket_n, client_st, 1);		
	fclose(fp);
}


void user_interface(int socket_n, struct sockaddr_in client_st){
	char dir_path[50];
	char * message_received;
	int rv=0;
	do{
		strcpy(dir_path, "./");
		printf("\n\rReceiving command from the client\n\r");
		do{
			message_received = receive_packet(socket_n, client_st, 0);
		}while(strcmp(message_received, "apple") == 0);
		printf("\n\r%s\n\r", message_received);
		if(message_received != NULL){
			if(strcmp(message_received, "get") == 0){
				message_received = receive_packet(socket_n, client_st, 0);
				strcat(dir_path, message_received);
				printf("\n\rdir_path = %s\n\r", dir_path);
				send_file_to_client(dir_path, socket_n, client_st);
								
			}
			else if(strcmp(message_received, "put") == 0){
				message_received = receive_packet(socket_n, client_st, 0);
				strcat(dir_path, message_received);
				printf("\n\rdir_path = %s\n\r", dir_path);
				receive_file_from_client(dir_path, socket_n, client_st);			
			}
			else if(strcmp(message_received, "ls") == 0){
				list_files("./", socket_n, client_st);
			
			}
			else if(strcmp(message_received, "del") == 0){
				message_received = receive_packet(socket_n, client_st, 0);
				strcat(dir_path, message_received);
				printf("\n\rdir_path = %s\n\r", dir_path);
				rv = remove(dir_path);
				if(rv == 0) printf("\n\rFile deleted successfully\n\r");	
				else printf("\n\rUnable to delete the file\n\r");	
			
			}
			else if(strcmp(message_received, "exit") == 0){
				printf("\n\rExiting server!\n\r");
				break;
			}
		}
	}while(1);


}

int main (int argc, char * argv[] )
{


	int sock, i;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char * (*files_in_dir);
	if (argc != 2)
	{
		printf ("\n\rUSAGE:  <port>\n");
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
		printf("\n\runable to create socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("\n\runable to bind socket\n");
	}

	remote_length = sizeof(remote);

	//waits for an incoming message
	bzero(buffer,sizeof(buffer));
	nbytes = nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
	
	printf("\n\rThe client says %s\n", buffer);
	char * message_received;
	char msg[] = "orange";

	user_interface(sock, remote);
	

	printf("\n\rClosing socket\n\r");
	close(sock);
	printf("\n\rServer exited!\n\r");
}


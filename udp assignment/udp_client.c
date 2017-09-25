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
		if(packet1[bit_i] != packet2[bit_i]) break;
	}
	printf("\n\rbit_last=%d %d", packet1[PACKET_SIZE - 1], packet2[PACKET_SIZE - 1]);
	if(bit_i == PACKET_SIZE) return 0;
	else return -1;
}


/* You will have to modify the program below */
char * send_packet(char packet[],  int socket_n, struct sockaddr_in server_struct, char reliability){
	struct timeval tv;
	char server_response[10];
	int nbytes;
	static int packet_i = 0;
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
		printf("\n\rPacket sent = %d", packet_i++);
		nbytes = recvfrom(socket_n, server_response, 10, 0, (struct sockaddr *)&server_struct, &addr_length);  
		if(nbytes < 0 && errno == EAGAIN && reliability == 1){
			printf("\n\rACK timeout error");
			resend = 1;
			packet_i--;
		}
		else{
			printf("\n\rServer says %s", server_response);
			resend = 0;			
		}	
	}while(resend == 1);
	bzero(server_response,sizeof(server_response));	
}

char * receive_packet(int socket_n, struct sockaddr_in server_struct, char reliability){
	struct timeval tv;
	static char packet[PACKET_SIZE];
	static char packet_2[PACKET_SIZE];
	char recv_done = 0;
	static int packet_i = 0;
	char client_response[] = "apple";	
	int nbytes;
	int addr_length = sizeof(server_struct);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	int rv;
	rv = setsockopt(socket_n, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(tv));
	if(rv < 0){
		printf("\n\rSetsockopt error\n\r");
	}
	bzero(packet_2,sizeof(packet_2));
	nbytes = recvfrom(socket_n, packet_2, PACKET_SIZE, 0, (struct sockaddr *)&server_struct, &addr_length);	
	printf("\n\rPacket received = %d", packet_i++);
	if(reliability == 0 || strcmp(packet, "eof") == 0){
		bzero(packet, sizeof(packet));	
		packet_i = 0;
		nbytes = sendto(socket_n, client_response, strlen(client_response), 0, (struct sockaddr *)&server_struct, sizeof(server_struct));	
		return packet_2;
	}
	if(compare_packets(packet, packet_2) < 0){
		printf("\n\rValid New Packet received");
		recv_done = 1;
		for(int j=0; j<PACKET_SIZE; j++){
			packet[j] = packet_2[j];
		}		
	}
	else {
		printf("\n\rPacket same as previous packet. Packet discarded");
		packet_i--;
	}
	nbytes = sendto(socket_n, client_response, strlen(client_response), 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
	bzero(packet_2,sizeof(packet_2));
	if(recv_done == 1) return packet;
	else return NULL;
	
}

void send_file_to_server(unsigned char file_name[], int socket_n, struct sockaddr_in server_struct)
{
	struct timeval tv;
	char read_file[PACKET_SIZE];
	char read_file_2[PACKET_SIZE];
	char server_response[10];	
	size_t read_bytes;
	int nbytes;
	char * message_received;
	int addr_length = sizeof(server_struct);
	int rv;
	int i = 0;
	unsigned char id=0;
	char resend = 0;
	tv.tv_sec = 0;
	tv.tv_usec = 300000;
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
		nbytes = sendto(socket_n, read_file, read_bytes + 1, 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
		printf("\n\rPacket sent = %d", i++);
		nbytes = recvfrom(socket_n, server_response, 10, 0, (struct sockaddr *)&server_struct, &addr_length);  
		if(nbytes < 0 && errno == EAGAIN){
			printf("\n\rACK timeout error");
			resend = 1;
			i--;	
		}
		else if(server_response[0] != ((i%100)-1)){
			resend = 1;
			printf("\n\rServer response = %d resending..", server_response[0]);
			i--;
		}
		else{
			printf("\n\rServer says %d\n\r", server_response[0]);
			//bzero(read_file_2,sizeof(read_file) - 1);
			bzero(server_response,sizeof(server_response));
			resend = 0;			
		}	
	}
	send_packet("eof", socket_n, server_struct, 1);
//	do{
//		message_received = receive_packet(socket_n, server_struct, 0);
//	}while(strcmp(message_received, "eof") != 0);
//	send_packet("done", socket_n, server_struct, 1);	
	fclose(fp);
}

void receive_file_from_server(unsigned char file_name[], int socket_n, struct sockaddr_in server_struct){
	struct timeval tv;
	char read_file[PACKET_SIZE];
	char read_file_2[PACKET_SIZE];
	char recv_done = 0;
	char client_response[10];	
	size_t write_bytes;
	char * message_received;
	int nbytes;
	int addr_length = sizeof(server_struct);
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
		nbytes = recvfrom(socket_n, read_file_2, PACKET_SIZE, 0, (struct sockaddr *)&server_struct, &addr_length);
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
				client_response[0] = (i%100) - 1;
				nbytes = sendto(socket_n, client_response, 10, 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
			}
			else{
				printf("\n\rReceive done!"); 
				recv_done = 1; 
				nbytes = sendto(socket_n, "eof", 10, 0, (struct sockaddr *)&server_struct, sizeof(server_struct));
//				do{
//					send_packet("eof", socket_n, server_struct, 1);
//					message_received = receive_packet(socket_n, server_struct, 0);
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

void user_interface( int socket_n, struct sockaddr_in server_struct){
	char dir_path[50];
	char *message_received;
	char in[100];
	char files [400] = "\n\r";
	do{
		bzero(in,sizeof(in));
		printf("\n\rEnter your choice!\n\r");
		printf("\n\r1:Get file\n\r");
		printf("\n\r2:Put file\n\r");
		printf("\n\r3:List file\n\r");
		printf("\n\r4:Delete file\n\r");	
		printf("\n\r5:Exit\n\r");
		fgets(in, 3, stdin);
		if(in[0]=='1'){
			strcpy(dir_path, "/home/djdharmik/Downloads/udp/client_files/");
			send_packet("get", socket_n, server_struct, 1);
			printf("\n\rGet file selected. Enter the file name\n\r");
			fgets(in, 100, stdin);	
			in[strlen(in) - 1] = '\0';
			printf("\n\rYou enterred: %s\n\r", in);	
			send_packet(in, socket_n, server_struct, 1);
			strcat(dir_path, in);
			receive_file_from_server(dir_path, socket_n, server_struct);
		
						
		}
		else if(in[0]=='2'){
			strcpy(dir_path, "/home/djdharmik/Downloads/udp/");
			send_packet("put", socket_n, server_struct, 1);
			printf("\n\rPut file selected. Enter the file name\n\r");
			fgets(in, 100, stdin);	
			in[strlen(in) - 1] = '\0';
			printf("\n\rYou enterred: %s\n\r", in);	
			send_packet(in, socket_n, server_struct, 1);
			strcat(dir_path, in);
			send_file_to_server(dir_path, socket_n, server_struct);
		}
		else if(in[0]=='3'){
			send_packet("ls", socket_n, server_struct, 1);
			printf("\n\rList file selected\n\r");
			do{
				message_received = receive_packet(socket_n, server_struct, 1);
			}while(strcmp(message_received, "sof") != 0);
			do{
				message_received = receive_packet(socket_n, server_struct, 1);
				if(message_received != NULL && (strcmp(message_received, "apple") != 0)){
					if(strcmp(message_received, "eof") == 0) break;
					strcat(files,message_received);
					strcat(files, "\n\r");
				}	
			}while(1);
			printf("\n\rFiles in the directory:\n\r");
			printf("%s", files);			
				
		}
		else if(in[0]=='4'){
			send_packet("del", socket_n, server_struct, 1);
			printf("\n\rDelete file selected. Enter the file name\n\r");	
			fgets(in, 100, stdin);
			in[strlen(in) - 1] = '\0';
			printf("\n\rYou enterred: %s\n\r", in);	
			send_packet(in, socket_n, server_struct, 1);		
		}
		else if(in[0]=='5'){
			send_packet("exit", socket_n, server_struct, 1);
			printf("\n\rExiting the server\n\r");	
		}
		else{
			printf("\n\rYou enterred %c. Wrong choice\n\r", in[0]);	
		}
	strcpy(files, "\n\r");
	}while(1);

}


int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];

	struct sockaddr_in remote, server;              //"Internet socket address structure"
	
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

	user_interface(sock, remote);

	close(sock);

}


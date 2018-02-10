#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>


#define Length 99999

void client_handle(int, char*); 


int sock,newsocket,num,sin_size; 
int i =0;
char *dfs_config =	"dfs.conf";
char revbuf[Length]	    = "\0"; // Receiver buffer
char dirname[Length]    = "\0";
char dirname2[Length]   = "\0";
char filename1[Length]   = "\0";
char filename2[Length]  = "\0";
char filesize1[Length]   = "\0";
char filesize2[Length]   = "\0";
char subfolder[Length]   = "\0";
char usrname[20][40];
char passwrd[20][40];
char num_users;
char username[Length]    = "\0";
char password[Length]    = "\0";
char filebuffer[Length] = "\0";
char req_method[Length] = "\0";
char cmd [Length]       = "\0";
char cmd1 [Length]      = "\0";
char sendbuff[Length]   = "\0";
int clientlen,recvd;
double nbytes =0;
double nbytes1 =0;
char * ROOT;
char msg[] = "ACK";
char eKey[Length];	



void encryptFile(char * fileString, int fileSize){
	for(int i=0; i<fileSize; i++){
		fileString[i] = fileString[i] ^ eKey[i%strlen(eKey)];
	}
}

void decryptFile(char * fileString, int fileSize){
	for(int i=0; i<fileSize; i++){
		fileString[i] = fileString[i] ^ eKey[i%strlen(eKey)];
	}
}


void extractFileVer(char * files, char *s2, char res_arr[]){
	char s1[Length];
	char* p;
	strcpy(s1, files);
	p = strstr(s1, s2);
	if (p) {
		//	printf("String found\n");
		//	printf("First occurrence of string '%s' in '%s' is '%s'", s2, s1, p);
		//printf("\n\rstring:%s\n\r", p+strlen(s2)+1);	
		char *token;
		token = strtok(p+strlen(s2)+1, " \n\r");
		//printf("token:%s\n\r", token);
	    	//printf("\n\rversion:%d",atoi(token));
		res_arr[0] = atoi(token) + '0';
		do{
		    token = strtok(NULL, " \n\r");
		    printf("\n\rtoken:%s\n\r", token);
		}while(strstr(token, s2) == NULL);
		token = strrchr(token, '.');
		//printf("\n\rtoken:%s\n\r", token);
		//printf("\n\rversion2:%d\n\r", atoi(token+1));
		res_arr[1] = atoi(token+1) + '0';

	} else
		printf("String not found\n");

}



void filewrite(int socket, char* file_name1,char* file_name2)
{
	
	
	int filesize1,filesize2,nbytes;
	char sendbuff[Length] = "\0";
	char recvbuff[Length] = "\0";
	
	
	FILE *fp2 = fopen(file_name1,"r");
	fseek(fp2, 0L, SEEK_END);
	filesize1 = ftell(fp2);
	fseek(fp2, 0L, SEEK_SET);
	fclose(fp2);
	
	FILE *fp3 = fopen(file_name2,"r");
	fseek(fp3, 0L, SEEK_END);
	filesize2 = ftell(fp3);
	fclose(fp3);
	
	
	sprintf(sendbuff,"%s %d %s %d",file_name1,filesize1,file_name2,filesize2);
	
	printf("\n\r\n\r********SendBuff:%s\n\r",sendbuff);
	
	write(socket,sendbuff,strlen(sendbuff));	
	
		
	recv(socket,recvbuff,sizeof recvbuff,0);
	//printf("ACK RECEIVED:%s\n", recvbuff);
	

	fp2 = fopen(file_name1,"r");
	if (fp2)
	{
		while (!feof(fp2))
		{
			bzero(sendbuff,Length);
			nbytes = fread(sendbuff,1,Length,fp2);
			/**** encrypt file ****/
			encryptFile(sendbuff, nbytes);
			/**** decrypt file ****/
//			decryptFile(sendbuff, nbytes);
						
			if (send(socket,sendbuff,nbytes,0)<0)
			{
				printf("Error in sending the file");
				exit(1);
			}
	
	
			bzero (recvbuff,Length);
			recv(socket,recvbuff,Length,0);
			printf("%s\n",recvbuff);
			if (strcmp(recvbuff,"ACK")!=0)
			{
				printf("Error in receiving ACK");
				//exit(1);
			}
		
		}
		printf("File Sent.....\n");
		fclose(fp2);
	}
	else
	{
		printf("Unable to find the file...\n");
		exit(1);
		
	}
	fp3 = fopen(file_name2,"r");
	bzero(recvbuff,Length);
	
	if (fp3)
	{
		while (!feof(fp3))
		{
			//printf("Entered the Loop\n");
			bzero(sendbuff,Length);
			nbytes = fread(sendbuff,1,Length,fp2);
			//printf("FileRead:%s",sendbuff);
			//printf("Socket Descriptor:%d\n",socket);
			/**** encrypt file ****/
			encryptFile(sendbuff, nbytes);


			if (send(socket,sendbuff,nbytes,0)<0)
			{
				printf("Error in sending the file");
				exit(1);
			}
			//printf("Socket Descriptor:%d\n",socket);
			//printf("Reached here\n");
			bzero (recvbuff,Length);
			recv(socket,recvbuff,Length,0);
			//printf("Reached here\n");
			//printf("Bytes Received:%d\n",bytesrecvd);
			if (strcmp(recvbuff,"ACK")!=0)
			{
				printf("Error in receiving ACK\n");
				exit(1);
			}
		
		}
		printf("File Sent.....\n");
		fclose(fp3);
	}
	else
	{
		printf("Unable to find the file...\n");
		exit(1);
		
	}
	


}


int main (int argc, char *argv [])
{
	/* Defining Variables */
	struct sockaddr_in serverAddr; /* client addr */
	struct sockaddr_in clientAddr; /* server addr */
	int PORT = atoi(argv[2]);
	char parentdir[Length] = "\0";
	char * file_parse;
	
	
	
	FILE* fconfig = fopen(dfs_config,"r");
	if (fconfig == NULL)
	{
		printf("Cannot Read from Config File ");
		exit(0);
	}
	
	while(fgets(filebuffer,sizeof filebuffer,fconfig)!=NULL)
	{
	    /**********************
            * Finds usernames 
            /**********************/
            if(strncmp(filebuffer,"Username",8)==0) {
		static int username_i=0;
		file_parse = strstr(filebuffer,":");
		file_parse = file_parse + 1;
		strcpy(usrname[username_i++],file_parse);
		usrname[username_i - 1][strlen(file_parse)-1] = '\0';
                bzero(filebuffer, sizeof(filebuffer));
		num_users++;
            }

	    /**********************
            * Finds passwords 
            /**********************/
            if(strncmp(filebuffer,"Password",8)==0) {
		static int password_i=0;
		file_parse = strstr(filebuffer,":");
		file_parse = file_parse + 1;
		strcpy(passwrd[password_i++],file_parse);
		passwrd[password_i - 1][strlen(file_parse)-1] = '\0';
                bzero(filebuffer, sizeof(filebuffer));
            }

	    /**********************
            * Finds encryption key 
            /**********************/
            if(strncmp(filebuffer,"EncryptionKey",13)==0) {
		file_parse = strstr(filebuffer,":");
		file_parse = file_parse + 1;
		strcpy(eKey,file_parse);
		if(eKey[strlen(eKey) - 1] == '\n') eKey[strlen(eKey) - 1] = '\0';
                bzero(filebuffer, sizeof(filebuffer));
            }

		
	}
	 
//	usrname[strlen(usrname)-1] = '\0';
//	passwrd[strlen(passwrd)-1] = '\0';
	for(int i = 0; i<num_users; i++){
		printf("\n\rUser%d:%s %s", i, usrname[i], passwrd[i]);
	}
	//printf("\n\rEncryptionKey: %s of length %d", eKey, strlen(eKey));
	ROOT = getenv("PWD");					// GET Present Working Directory //
	strcpy(parentdir,argv[1]);
	
	/* Get the Socket file descriptor */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
	   printf("ERROR: Failed to obtain Socket Descriptor");
	   exit(1);
	}
	else 
	   printf("[Server] Obtaining socket descriptor successfully.\n");

	/* Fill the client socket address struct */ 
	serverAddr.sin_family = AF_INET; // Protocol Family
	serverAddr.sin_port = htons(PORT); // Port number
	serverAddr.sin_addr.s_addr = INADDR_ANY; // AutoFill local address
	bzero(&(serverAddr.sin_zero), 8); // Flush the rest of struct

	/* Bind a special Port */
	if( bind(sock, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr)) == -1 )
	{
	   printf("ERROR: Failed to bind Port.\n");
	   exit(1);
	}
	else 
	   printf("Binded tcp port %d sucessfully.\n",PORT);

	/* Listen remote connect/calling */ 
	if(listen(sock,10) == -1)
	{
	   printf("ERROR: Failed to listen Port.\n");
	   exit(1);
	}
	else
	   printf ("Listening on the port %d ....\n", PORT);
	   

	clientlen = sizeof(struct sockaddr_in);

	while(1)
	{
		newsocket = accept(sock, (struct sockaddr *) &clientAddr, &clientlen);
		if (newsocket == -1) printf("Error Accepting the connection");
	
		printf("Connection Accepted....\n");
		int pid = fork();
		//printf("\n Fork ID : %d\n", pid);
		if (pid <0)
		printf("Error on Fork !!");
		if (pid == 0)
		{
			close(sock);  			// child doesn't need Parent's Socket //
			client_handle(newsocket, parentdir);
			exit(0);
		}	// Child
		close(newsocket);
	} 							// While //
	close (sock);
	return 0;
}  // Main
	
	
	
	
	
	
	
	

void client_handle(int newsocket, char* parentdir)
{	
	int rcvd = recv(newsocket, revbuf,sizeof revbuf ,0);
	if (rcvd <= 0)
	{
		printf("No Data received from client, Exiting from the child Sever...\n");
		close(newsocket);
		exit(0);
	}

	//printf("Request from the Client:%s\n", revbuf);

	//bzero(filename,Length);

	send(newsocket,"ACK",3,0);    // Sending ACK to the client //

	sscanf(revbuf,"%s %s %s %s %s %s %s %s",filename1,filesize1,username,password,req_method,filename2,filesize2, subfolder);
	int size1 = atoi(filesize1);
	int size2 = atoi(filesize2);
	printf("\n\rsubfolder:%s", subfolder);
	bzero(revbuf,Length);
	recv(newsocket,revbuf,sizeof revbuf,0);
	//printf("ACK Blah Blah:%s\n",revbuf);
	bzero(revbuf,Length);
	//printf("%s %s\n\n", username,password);
	int i = 0;
	for(i=0; i<num_users; i++){
		if(strcmp(username,usrname[i]) || strcmp(password,passwrd[i]) ==0) break;	
	}
	if (i==num_users)
	{
		printf("Invalid Username/Password, Sending to Client...\n");
		char err_msg[] = "Invalid Username/Password, Please try again"; //not needed
		send(newsocket,err_msg,sizeof err_msg,0);
		close(newsocket);
		exit(0);
	}

	else
	{

		if (!strcmp(req_method,"PUT") || !strcmp(req_method,"put"))
		{
			send(newsocket,"ACK",3,0);
	
			strcpy(dirname,parentdir);					// Get Server Directory Name //
			//strcat(ROOT,dirname);
			//printf("Server Dire:%s\n", ROOT);
			strcat(dirname,username);
			strcat(dirname,"/");
			if(strlen(subfolder)>1) strcat(dirname, subfolder);
			sprintf(cmd,"mkdir -p %s",dirname);


			printf("Command:%s\n", cmd);
			system(cmd);

			sprintf(cmd1,"%s/%s%s", ROOT,dirname,filename1);
			//printf("Sprintf:%s\n",cmd1);

			//strcat(dirname,filename);
			//strcat(ROOT,"/");
			//strcat(ROOT,dirname);
			//printf("Final Root Dir:%s\n",ROOT);	



			bzero(revbuf,Length);
			FILE *fr = fopen(cmd1, "wb");

			if (fr)
			{
				//printf("receiving...\n");
				while (nbytes<size1)
				{
					nbytes1 = recv(newsocket,revbuf,sizeof revbuf,0);
					//printf("%s\n", revbuf);
					decryptFile(revbuf, nbytes1);
					fwrite(revbuf,nbytes1,1,fr);
					send(newsocket,msg,sizeof(msg),0);
					nbytes = nbytes + nbytes1;
	
	
				}

				//printf("File Received\n");
				nbytes = 0;
				fclose(fr);

			}
			else	printf("Unable to Locate the file\n");





			/*-------------------Receiving the Second File ---------------------------*/


			//printf("File Name Read:%s File Name Length:%ld\n",filename2,strlen(filename2));
			bzero(cmd1,Length);
			sprintf(cmd1,"%s/%s%s", ROOT,dirname,filename2);
			//printf("Opening the file2:%s\n",cmd1);


			bzero(revbuf,Length);
			nbytes 	=	0;
			nbytes1   = 	0;
			fr = fopen(cmd1, "wb");
			

			if (fr)
			{
				//printf("Size of the file receiving:%d\n",size2);
				//printf("receiving...\n");
				while (nbytes<size2)
				{
					//printf("Socket Descriptor:%d\n",newsocket);
					nbytes1 = recv(newsocket,revbuf,sizeof revbuf,0);
					//printf("Bytes Received%f\n", nbytes1);
					decryptFile(revbuf, nbytes1);
					fwrite(revbuf,nbytes1,1,fr);
					send(newsocket,msg,sizeof(msg),0);
					nbytes = nbytes + nbytes1;
	
	
				}

				//printf("File Received\n");
				nbytes = 0;
				fclose(fr);

			}
			else	printf("Unable to Locate the file\n");
		}  // PUT
	
	
	
	
		else if (!strcmp(req_method,"GET") || !strcmp(req_method,"get"))
		{

			
			char getlistdir1[Length] = "\0";
			char getlistdirname[Length] = "\0";
			char getdirValues[Length]   = "\0";
			DIR *dirget;
			char fver[2] = {0};
			char getfile1[Length] = {0};
			char getfile2[Length] = {0};
			struct dirent *getfile;
								
			
			strcpy(getlistdirname,parentdir);
			if(strlen(subfolder)>1) sprintf(getlistdir1,"%s/%s%s/%s",ROOT,getlistdirname,username,subfolder);
			else sprintf(getlistdir1,"%s/%s%s/",ROOT,getlistdirname,username);
			printf("\n\rdirectory:%s\n\r", getlistdir1);
			dirget = opendir(getlistdir1);
			
			
			if (dirget)
	    		{
	      			while ((getfile = readdir(dirget)) != NULL)
		      		{
	       			 	if (getfile->d_type == DT_REG)
	       			 	strcat(getdirValues,strcat(getfile->d_name," "));
	       			}
				printf("%s", getdirValues);
	      			closedir(dirget);
	      		}
			//printf("****************filename:%s strlen:%d\n\r", filename1, strlen(filename1));
			if(strstr(getdirValues, filename1) == NULL){
				send(newsocket, "File Not Found", 14, 0);
				recv(newsocket,revbuf,sizeof revbuf,0);
				close(newsocket);
				exit(0);	
			}
			else{
				extractFileVer(getdirValues, filename1, fver);//extract file versions
				//send(newsocket,fver,2,0);
				bzero(revbuf,Length);
				//recv(newsocket,revbuf,sizeof revbuf,0);
			
			//	strcpy(dirname,parentdir);	
			//	strcat(dirname,username);
			//	strcat(dirname,"/");
					
				sprintf(getfile1,"%s.%s.%c",getlistdir1,filename1,fver[0]);
				sprintf(getfile2,"%s.%s.%c",getlistdir1,filename1,fver[1]);
				printf("\n\r\n\r%s %s\n\r\n\r", getfile1, getfile2);
				filewrite(newsocket, getfile1, getfile2);
			}
				
								
		}  // GET
	
	
		else if (!strcmp(req_method,"LIST") || !strcmp(req_method,"list"))
		{
	
			//printf("RequestedMethod:%s\n",req_method);
			
			char listdir1[Length] = "\0";
			char listdirname[Length] = "\0";
			char dirValues[Length]   = "\0";
			DIR *dir;
			struct dirent *file;
			int nn=0,ii=0;
								
			
			strcpy(listdirname,parentdir);
			if(strlen(subfolder)>1) sprintf(listdir1,"%s/%s%s/%s",ROOT,listdirname,username,subfolder);
			else sprintf(listdir1,"%s/%s%s",ROOT,listdirname,username);
			printf("\n\rdirectory:%s\n\r", listdir1);
			dir = opendir(listdir1);
			
			
			if (dir)
	    		{
	      			while ((file = readdir(dir)) != NULL)
		      		{
	       			 	if (file->d_type == DT_REG)
	       			 	strcat(dirValues,strcat(file->d_name," "));
	       			}
				printf("%s", dirValues);
	      			closedir(dir);
	      		}
			
			
			
			send(newsocket,dirValues,sizeof dirValues,0);
			
		} // LIST
	
	
	}
	close(newsocket);
	
	
}

	


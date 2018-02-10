#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define splitnum 4
#define Length 99999

struct serverFiles{
	char fileName[Length];
	char fileVer;
};

struct validFiles{
	char fileName[Length];
	char fileVer;

};

struct serverFiles serFiles[1000];
struct validFiles valFiles[1000];

char eKey[Length];
char dirname[Length];
char parentdir[10] = "DFC/";
char cmd[Length];
char cmd1[Length];
char *ROOT;
int nbytes;

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

void storeFileinStruct(struct serverFiles *tempSerFiles, char file_n[]){
	char tempFile[Length] = {0};
	strcpy(tempFile, file_n);
	strcpy(tempSerFiles->fileName, strtok(tempFile, "."));
	tempSerFiles->fileVer = atoi(strtok(NULL, "."));	
}


int md5_modcalc(char *filename1)
{
   unsigned char c[MD5_DIGEST_LENGTH];
   char *filename = filename1;
   char buff[MD5_DIGEST_LENGTH]="\0";
   //char *buff;
   int i,digit,mod =0 ;
   //printf("Filename MD5_Hash:%s\n\n", filename);
  
   FILE *fp = fopen (filename,"rb");
   MD5_CTX mdContext;
   int bytes;
   unsigned char data[1024];
   
   if (fp == NULL)
   {
	printf("%s Cannot Open File\n", filename);
	exit(1);
	
   }
		
   MD5_Init(&mdContext);
   while ((bytes = fread(data,1,1024,fp)) != 0)
   MD5_Update (&mdContext, data, bytes);
   MD5_Final (c, &mdContext);
	for (i = 0; i< MD5_DIGEST_LENGTH; i++)
	{
		//printf("Hash Value: %02x",c[i]);
		sprintf(&buff[2*i],"%02x", c[i]);
	}
			
			
	//printf("Hash Value1:%s\n", buff);
				


	/* Convert Hex to integer  */ 
	for (i = 0; i<2*MD5_DIGEST_LENGTH; i++)
	{
		if (buff[i] >= '0' && buff[i] <= '9')
		{
			digit = buff[i] - '0';
			//printf("digit:%d Char:%c\n\n", digit, buff[i]);
		}
		else if (buff[i] >= 'a' && buff[i] <= 'f')
		{
			digit = buff[i] - 'a' +10;
			//printf("digit:%d Char:%c\n\n", digit, buff[i]);
		}
		else 
			digit = buff[i] - 'A' + 10;
			mod = ((mod*16 + digit) % 4);

	}
			
		//printf("Modulo:%d\n",mod); 


	fclose(fp);
	return (mod);
			
   
}

void filewrite(int socket, char* filename1,char* filename2, char* username, char* password, char* req_method, char* subfolder)
{
	
	
	int filesize1,filesize2,nbytes;
	char sendbuff[Length] = "\0";
	char recvbuff[Length] = "\0";
	
	
	FILE *fp2 = fopen(filename1,"r");
	fseek(fp2, 0L, SEEK_END);
	filesize1 = ftell(fp2);
	fseek(fp2, 0L, SEEK_SET);
	fclose(fp2);
	
	FILE *fp3 = fopen(filename2,"r");
	fseek(fp3, 0L, SEEK_END);
	filesize2 = ftell(fp3);
	fclose(fp3);
	
	
	sprintf(sendbuff,"%s %d %s %s %s %s %d %s",filename1,filesize1,username,password,req_method,filename2,filesize2, subfolder);
	
	//printf("SendBuff:%s",sendbuff);
	
	write(socket,sendbuff,strlen(sendbuff));	
	
		
	recv(socket,recvbuff,sizeof recvbuff,0);
	//printf("ACK RECEIVED:%s\n", recvbuff);
	
	send(socket,"ACK",3,0);
	bzero(recvbuff,Length);
	
	recv(socket,recvbuff,sizeof recvbuff,0); 
	
	
	if (strcmp(recvbuff,"Invalid Username/Password, Please try again") ==0)		//not needed
	{
		printf("Server Replied with Invalid Username Password Message\n\n");
		//send(socket,"ACK",3,0);
		exit(0);
	}
	else
	{
	bzero(recvbuff,Length);
	
	
	
	fp2 = fopen(filename1,"r");
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
			//printf("%s\n",recvbuff);
			if (strcmp(recvbuff,"ACK")!=0)
			{
				printf("Error in receiving ACK");
				//exit(1);
			}
		
		}
		//printf("File Sent.....\n");
		fclose(fp2);
	}
	else
	{
		printf("Unable to find the file...\n");
		exit(1);
		
	}
	
	
	
	/*	
	bzero(sendbuff,Length);	
	fp2 = fopen(filename2,"r");
	fseek(fp2, 0L, SEEK_END);
	filesize = ftell(fp2);
	fseek(fp2, 0L, SEEK_SET);
	
	
	
	sprintf(sendbuff,"%s %d",filename2,filesize);
	
	//printf("SendBuff:%s",sendbuff);
	
	write(socket,sendbuff,strlen(sendbuff));
	
	recv(socket,recvbuff,sizeof recvbuff,0);
	printf("ACk:%s\n", recvbuff);*/
	
	//printf("2nd File:%s\n\n",filename2);
	
	fp3 = fopen(filename2,"r");
	

	
	
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
	
	

}


int requestFile(int socket,char* filename, char* username, char* password, char* req_method, char* subfolder)
{
	char sendbuff[Length]  = "\0";
	char recvbuff[Length]  = "\0";
	char temp[Length]      = "\0";
	char filename1[Length] = {0};
	char filename2[Length] = {0};
	char file_size1[Length] = {0};
	char file_size2[Length] = {0};
	int nbytes1 = 0,nbytes = 0;
	int filesize = 909;
	int filesize2 = 909;
	char *temp_filen;
	

	sprintf(sendbuff,"%s %d %s %s %s %s %d %s",filename,filesize,username,password,req_method,"XYZ",filesize2, subfolder);
	//printf("1st Get Request:%s\n",sendbuff);
	write(socket,sendbuff,strlen(sendbuff));
	
		
	recv(socket,recvbuff,sizeof recvbuff,0);
	//printf("ACK RECEIVED:%s\n", recvbuff);
	
	send(socket,"ACK",3,0);
	
	bzero(recvbuff,Length);
	
	recv(socket,recvbuff,sizeof recvbuff,0); //received filenames n sizes
	
	//printf("Received From the Server:%s\n",recvbuff);
	
	if ((strcmp(recvbuff,"Invalid Username/Password, Please try again **********") ==0))
	{
		printf("********** Server Replied with Invalid Username Password Message\n\n");
		//send(socket,"ACK",3,0);
		exit(0);
	}
	else if((strcmp(recvbuff,"File Not Found") ==0)){
		printf("\n\r********** Server replied with File Not Found! **********\n\r");
		return -1;
	}
	else
	{
		sscanf(recvbuff, "%s %s %s %s", filename1, file_size1, filename2, file_size2);
		//printf()
		temp_filen = strstr(filename1, filename);
		strcpy(filename1, temp_filen);
		temp_filen = strstr(filename2, filename);
		strcpy(filename2, temp_filen);
		filesize = atoi(file_size1);
		filesize2 = atoi(file_size2);

	
		send(socket,"ACK",3,0);

		strcpy(dirname,parentdir);					// Get Server Directory Name //
		//strcat(ROOT,dirname);
		//printf("Server Dire:%s\n", ROOT);
		strcat(dirname,username);
		strcat(dirname,"/");
		if(strlen(subfolder)>1) strcat(dirname, subfolder);
		sprintf(cmd,"mkdir -p %s",dirname);


		//printf("Command:%s\n", cmd);
		system(cmd);
		ROOT = getenv("PWD");
		sprintf(cmd1,"%s/%s%s", ROOT,dirname,filename1);
		//printf("Sprintf:%s\n",cmd1);

		//strcat(dirname,filename);
		//strcat(ROOT,"/");
		//strcat(ROOT,dirname);
		//printf("Final Root Dir:%s\n",ROOT);	



		bzero(recvbuff,Length);
		FILE *fr = fopen(cmd1, "wb");

		if (fr)
		{
			//printf("receiving...\n");
			while (nbytes<filesize)
			{
				nbytes1 = recv(socket,recvbuff,sizeof recvbuff,0);
				//printf("%s\n", recvbuff);
				decryptFile(recvbuff, nbytes1);
				fwrite(recvbuff,nbytes1,1,fr);
				send(socket,"ACK",3,0);
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


		bzero(recvbuff,Length);
		nbytes 	=	0;
		nbytes1   = 	0;
		fr = fopen(cmd1, "wb");
		

		if (fr)
		{
			//printf("Size of the file receiving:%d\n",size2);
			//printf("receiving...\n");
			while (nbytes<filesize2)
			{
				//printf("Socket Descriptor:%d\n",socket);
				nbytes1 = recv(socket,recvbuff,sizeof recvbuff,0);
				//printf("Bytes Received%f\n", nbytes1);
				decryptFile(recvbuff, nbytes1);
				fwrite(recvbuff,nbytes1,1,fr);
				send(socket,"ACK",3,0);
				nbytes = nbytes + nbytes1;


			}

			//printf("File Received\n");
			nbytes = 0;
			fclose(fr);

		}
		else	printf("Unable to Locate the file\n");
	} // PASSWORD USERNAME CHECK ..
	return 0;
}

int main (int argc, char * argv[])
{
     /* Socket Descriptors */
while(1){
     	int newsocket1,newsocket2,newsocket3,newsocket4;
     	struct sockaddr_in serverAddr;
     
   /*  Read from Configuration file Parameters */ 
	//char *dfc_config = "dfc.conf";
	char username[Length] = "\0", password[Length] = "\0"; 
	int port[3],i;
	int kill1,kill2,kill3,kill4;
	char * file_parse;
	char config_buff[Length]; 
	char *filemod = ".mod.txt"; 
	FILE *fp1,*fpmod,*fpmod1,*fget1,*fget2,*fget3,*fget4;
	int length;
	char buffer[512]= "\0";
	char* req;


	fp1 = fopen(argv[1],"r");
	i = 0;
	while (fgets(config_buff,128,fp1) != NULL)
	{
		file_parse = strstr(config_buff,":");
		file_parse = file_parse + 1;
		if (i==0) port[i] = atoi(file_parse);
		if (i==1) port[i] = atoi(file_parse);
		if (i==2) port[i] = atoi(file_parse);
		if (i==3) port[i] = atoi(file_parse);
		if (i==4) strcpy(username,file_parse);
		if (i==5) strcpy(password,file_parse); 
		if (i==6) {strcpy(eKey,file_parse); break;}
		i++;
	
	}
		
	fclose(fp1);	
	username[strlen(username)-1] = '\0';
	password[strlen(password)-1] = '\0';
	if(eKey[strlen(eKey) - 1] == '\n') eKey[strlen(eKey) - 1] = '\0';
	//printf("\n\rEncryptionKey: %s of length %d", eKey, strlen(eKey));
     

	int mod,size,sizeby4,modread;
	char in_buffer [Length] = "\0";
	char filename  [Length] = "\0";
	char subfolder [Length]	= "\0";
	char filename1 [Length] = "\0";
	char filename2 [Length] = "\0";
	char filename3 [Length] = "\0";
	char filename4 [Length] = "\0";
	char req_method[Length] = "\0";
	char sendbuff  [Length] = "\0";
	char listrecv  [Length] = "\0";
	FILE *fp;
	char cmd [Length]= "\0";
	
	

	//    while(1){	    		
	//printf("Username:%s\nPassword:%s",username,password);
	char cmdreceived = 0;	
	do{
		printf("\n\n**************************** Distributed File Client (DFC) Running **************************\n\n");
		printf("DFC supports three option\n");
		printf("1. PUT/put Filename subfolder/\t This option puts the part of the file in to 4 server.\n\n");
		printf("2. GET/get Filename \t This option downloads the requested file from the servers.\n\n");
		printf("3. LIST/list \t \t This option displays the list of all the available files on the server.\n\n");
	


		fgets(in_buffer,sizeof in_buffer, stdin);

		sscanf(in_buffer, "%s %s %s",req_method,filename, subfolder);
		//printf("\n\rlength of subfolder = %d\n\r", strlen(subfolder));
		//if(subfolder[strlen(subfolder) - 1] == '/') printf("\n\r%d\n\r", subfolder[strlen(subfolder) - 1]);
		//int s = strcmp(req_method,"PUT")
		//while(1);
		if(!strcmp(req_method,"PUT") || !strcmp(req_method,"put") || !strcmp(req_method,"get") || !strcmp(req_method,"GET")){
			if(strlen(filename)>0){
				if(strlen(subfolder)>0 && ((subfolder[strlen(subfolder) - 1]) == '/')){
					cmdreceived = 1;
					printf("Request Method:%s Filename: %s Subfolder: %s\n",req_method, filename, subfolder);
				}
			}
		}
		else if(!strcmp(req_method,"list") || !strcmp(req_method,"LIST")){
			if(strlen(filename)>0 && (filename[strlen(filename) - 1] == '/')){
				cmdreceived = 1;
				printf("Request Method:%s Subfolder: %s\n",req_method, filename);
			}
		}
		if(cmdreceived == 0) printf("\n\r\n\r**************************** Input error! PLease insert in the given format. ****************************\n\r\n\r");
	}while(cmdreceived == 0);

     /*----------- DFS1 ----------------*/
     	serverAddr.sin_family = AF_INET;
     	serverAddr.sin_addr.s_addr = INADDR_ANY;
     	serverAddr.sin_port = htons(port[0]);
     
	newsocket1 = socket(AF_INET, SOCK_STREAM, 0);
	
	if (newsocket1 == -1)
		printf("Error Creating Socket");
				
	kill1 = connect(newsocket1, (struct sockaddr*) &serverAddr, sizeof(serverAddr)); 
	if (kill1<0)
	{
		printf("Error Connecting to DFS1\n");
		newsocket1 = -1;
	}
	serverAddr.sin_port = htons(port[1]);
	newsocket2 = socket(AF_INET, SOCK_STREAM, 0);
	
	if (newsocket2 == -1)
		printf("Error Creating Socket");
		
	kill2 = connect(newsocket2, (struct sockaddr*) &serverAddr, sizeof(serverAddr)); 
	
	if (kill2<0)
	{
		printf("Error Connecting to DFS2\n");
		newsocket2 = -1;
	}

 	serverAddr.sin_port = htons(port[2]);
	newsocket3 = socket(AF_INET, SOCK_STREAM, 0);
	
	if (newsocket3 == -1)
		printf("Error Creating Socket");
	
	kill3 = connect(newsocket3, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	
	if (kill3<0)
	{
		printf("Error Connecting to DFS3\n");
		newsocket3 = -1;
	}
	
	serverAddr.sin_port = htons(port[3]);
	newsocket4 = socket(AF_INET, SOCK_STREAM, 0);
	
	if (newsocket4 == -1)
		printf("Error Creating Socket");
			
	kill4 = connect(newsocket4, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	
	if (kill4<0)
	{
		printf("Error Connecting to DFS4\n");
		newsocket4 = -1;
	}
	

	length = strlen(filename);
	//while(1);


	//filename[strlen(filename)-1] = '\0'; 
	if (!strcmp(req_method,"PUT") || !strcmp(req_method,"put"))
	{
					
		mod = md5_modcalc(filename);    // Calculating mod 4 of MD5HASH value of the file.
		//printf("Mod:%d\n",mod);
		//fpmod = fopen(filemod,"a+");
		//if (fp1 == NULL)
		//	printf("Error");
	
		//sprintf(buffer,"%s%d\n",filename,mod);
		//fprintf(fp1,buffer);
		//fclose(fpmod);
				
		fp = fopen(filename, "r");
		fseek(fp, 0L, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		sizeby4 = size/4+1;
		//printf("\n%d",sizeby4);
	
		sprintf(cmd,"split -b %d -d -a 1 %s",sizeby4,filename);
		//printf("%s\n",cmd);
		system(cmd);
		int var =0;

		while (var<4)
		{
			sprintf(cmd,"cp x%d .%s.%d",var,filename,var+1);
			//printf("CMD:%s\n",cmd);
			system(cmd);
			sprintf(cmd,"rm x%d",var);
			system(cmd);
			var++;

		}

		sprintf(filename1, ".%s.1",filename);
		sprintf(filename2, ".%s.2",filename);
		sprintf(filename3, ".%s.3",filename);
		sprintf(filename4, ".%s.4",filename);	
		
		switch(mod) // Sending files to the server based on mod(MD5HASH,4) 
		{

			case 0:
				filewrite(newsocket1,filename1,filename2,username,password,req_method,subfolder);
				filewrite(newsocket2,filename2,filename3,username,password,req_method,subfolder);
				filewrite(newsocket3,filename3,filename4,username,password,req_method,subfolder);
				filewrite(newsocket4,filename4,filename1,username,password,req_method,subfolder);
				break;

			case 1:
				filewrite(newsocket1,filename4,filename1,username,password,req_method,subfolder);
				filewrite(newsocket2,filename1,filename2,username,password,req_method,subfolder);
				filewrite(newsocket3,filename2,filename3,username,password,req_method,subfolder);
				filewrite(newsocket4,filename3,filename4,username,password,req_method,subfolder);
				break;
	
			case 2:
				filewrite(newsocket1,filename3,filename4,username,password,req_method,subfolder);
				filewrite(newsocket2,filename4,filename1,username,password,req_method,subfolder);
				filewrite(newsocket3,filename1,filename2,username,password,req_method,subfolder);
				filewrite(newsocket4,filename2,filename3,username,password,req_method,subfolder);
				break;

			case 3:
				filewrite(newsocket1,filename2,filename3,username,password,req_method,subfolder);
				filewrite(newsocket2,filename3,filename4,username,password,req_method,subfolder);
				filewrite(newsocket3,filename4,filename1,username,password,req_method,subfolder);
				filewrite(newsocket4,filename1,filename2,username,password,req_method,subfolder);
				break;
	
		} // CASE END
		
	} // PUT END
	
	
	else if (!strcmp(req_method,"GET") || !strcmp(req_method,"get"))
	{
		
		char getFailed = 0;	
		
		//printf("**********filename:%s strlen:%d\n\r", filename, strlen(filename));
		/* Get the first part */
		/* Ask the first server if it is running */
		if(kill1 == 0 && kill3 == 0){		
				if(requestFile(newsocket3,filename,username,password,req_method,subfolder) == -1) getFailed = 1;
				if(getFailed == 0)requestFile(newsocket1,filename,username,password,req_method,subfolder);
		}
		else if(kill2 == 0 && kill4 == 0){		
				if(getFailed == 0)requestFile(newsocket4,filename,username,password,req_method,subfolder);
				if(getFailed == 0)requestFile(newsocket2,filename,username,password,req_method,subfolder);
		}
		else{
			getFailed = 1;
			printf("\n\r\n\r********** GET failed! Servers down **********\n\n");
		}

	//	while(1);
		
		if(getFailed == 0){	
			sprintf(filename1, "%s/%s%s.1",ROOT,dirname,filename);
			sprintf(filename2, "%s/%s%s.2",ROOT,dirname,filename);
			sprintf(filename3, "%s/%s%s.3",ROOT,dirname,filename);
			sprintf(filename4, "%s/%s%s.4",ROOT,dirname,filename);	
			

			//printf("\n\r********%s %s %s %s\n\r", filename1, filename2, filename3, filename4);
			char temp1[Length] = "\0";


			sprintf(cmd1, "%s/%s%s", ROOT, dirname, filename);
			
		
		
			FILE *fgetw =  fopen(cmd1,"w");
			if(fgetw == NULL) printf("\n\rERROR IN OPENING FILE\n");
			else{
				fget1 = fopen(filename1,"r");
				if (fget1)
				{
					while (!feof(fget1))
					{
						bzero(temp1,Length);
						nbytes = fread(temp1,1,Length,fget1);					
						fwrite (temp1,1,nbytes,fgetw);
					}
				}
				else printf("ERROR IN OPENING FILE");
				fget2 = fopen(filename2,"r");
				if (fget2)
				{
					while (!feof(fget2))
					{
						bzero(temp1,Length);
						nbytes = fread(temp1,1,Length,fget2);					
						fwrite (temp1,1,nbytes,fgetw);
					}
				}
				else printf("ERROR IN OPENING FILE");
				fget3 = fopen(filename3,"r");
				if (fget3)
				{
					while (!feof(fget3))
					{
						bzero(temp1,Length);
						nbytes = fread(temp1,1,Length,fget3);					
						fwrite (temp1,1,nbytes,fgetw);
					}
				}
				else printf("ERROR IN OPENING FILE");
				fget4 = fopen(filename4,"r");
				if (fget4)
				{
					while (!feof(fget4))
					{
						bzero(temp1,Length);
						nbytes = fread(temp1,1,Length,fget4);					
						fwrite (temp1,1,nbytes,fgetw);
					}
				}
				else printf("ERROR IN OPENING FILE");
				fclose(fgetw);
			}

			sprintf(cmd,"rm %s %s %s %s",filename1, filename2, filename3, filename4);
			system(cmd);
		}
			
	} // GET END
		
			
	else if (!strcmp(req_method,"LIST") || !strcmp(req_method,"list"))
	{
	
		bzero(sendbuff,Length);
		char temp_filename[5] = "\0";
		char temp_filesize[5] = "\0";
		//char dirValues[Length]   = "\0";
		char dirvalues1[Length] = "\0";
		char dirvalues2[Length] = "\0";
		char dirvalues3[Length] = "\0";
		char dirvalues4[Length] = "\0";
		//char templist2[50];
		char temp[50];
		char * token;
		int k =0, l = 0;
		int j,m;
		int complete = 0;
		char errormsg[] = "Invalid Username/Password, Please try again";
		strcpy(temp_filename,"HEY");
		strcpy(temp_filesize,"234");
		
		
		sprintf(sendbuff,"%s %s %s %s %s %s %s %s",temp_filename,temp_filesize,username,password,req_method,temp_filename,temp_filesize, filename);
		
		write(newsocket1,sendbuff,strlen(sendbuff)); // Sending Request to the server
		write(newsocket2,sendbuff,strlen(sendbuff));
		write(newsocket3,sendbuff,strlen(sendbuff));
		write(newsocket4,sendbuff,strlen(sendbuff));
		
		recv(newsocket1,listrecv,sizeof listrecv,0); // Receiving ACK
		recv(newsocket2,listrecv,sizeof listrecv,0);
		recv(newsocket3,listrecv,sizeof listrecv,0);
		recv(newsocket4,listrecv,sizeof listrecv,0);
				
		//printf("ACK RECEIVED:%s\n\n",listrecv);
	
		send(newsocket1,"ACK",3,0);	// Sending ACK
		send(newsocket2,"ACK",3,0);
		send(newsocket3,"ACK",3,0);
		send(newsocket4,"ACK",3,0);
		
		recv(newsocket1,dirvalues1,sizeof dirvalues1,0); // Receiving The directory list if Username and Password is correct
		recv(newsocket2,dirvalues2,sizeof dirvalues2,0);
		recv(newsocket3,dirvalues3,sizeof dirvalues3,0);
		recv(newsocket4,dirvalues4,sizeof dirvalues4,0);
		
		
		if(!strcmp(dirvalues1, errormsg) || !strcmp(dirvalues2, errormsg) || !strcmp(dirvalues1, errormsg) || !strcmp(dirvalues1, errormsg))
		{
			printf("Server Replied with Invaid Username and Password\n\n");
			exit(1);
		}
		printf("\n\r\n\r********** Files from servers: ********** \n%s\n%s\n%s\n%s\n", dirvalues1, dirvalues2, dirvalues3, dirvalues4);
		
		strcat(dirvalues1, " ");
		strcat(dirvalues1, dirvalues2);
		strcat(dirvalues1, " ");
		strcat(dirvalues1, dirvalues3);
		strcat(dirvalues1, " ");
		strcat(dirvalues1, dirvalues4);	
		char temp_val[Length];
		strcpy(temp_val, dirvalues1);
		//char *token;
		char *rest = temp_val;
		int i=0;
		memset(serFiles, 0, sizeof(serFiles));
		memset(valFiles, 0, sizeof(valFiles));
		while(token = strtok_r(rest, " \n\r", &rest)){
			storeFileinStruct(&serFiles[i], token);
			i++;	
		}
		serFiles[i].fileName[0] = '\0';
		i=0;
		do{
			for(int j=0; j<100; j++){
				if(valFiles[j].fileName[0] == 0){
					strcpy(valFiles[j].fileName, serFiles[i].fileName);
					valFiles[j].fileVer |= 1<<serFiles[i].fileVer;
					//printf("\n\r%d", valFiles[j].fileVer);
					break;
				}
				else if(strcmp(valFiles[j].fileName, serFiles[i].fileName) == 0){
					valFiles[j].fileVer |= 1<<serFiles[i].fileVer;
					//printf("\n\r%d", valFiles[j].fileVer);
					break;
				}
			}
			i++;	
		}while(serFiles[i].fileName[0] != 0);
		j=0;
		while(serFiles[j].fileName[0] != 0){
			//printf("%s %d\n\r", serFiles[j].fileName, serFiles[j].fileVer);
			j++;
		}
		j=0;
		printf("\n\r\n\r********* List of valid files *********\n\r\n");
		while(valFiles[j].fileName[0] != 0){
			//printf("%s %d\n\r", valFiles[j].fileName, valFiles[j].fileVer);
			if(valFiles[j].fileVer == 30) printf("%s [complete]\n\r", valFiles[j].fileName);
			else printf("%s [incomplete]\n\r", valFiles[j].fileName);
			j++;
		}
			
	} // END LIST
    
	
		close(newsocket1);
		close(newsocket2);
		close(newsocket3);
		close(newsocket4);
}    
		return 0;

	
	
}

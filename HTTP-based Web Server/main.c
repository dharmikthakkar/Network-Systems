#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>

#define MAX_CONNECTIONS 1000
#define BYTES 1024
#define MSG_SIZE 99999
#define wsMaxSize 1000

// Errors
#define GETADDRINFOERROR 1
#define BINDERROR        1
#define LISTENERROR      1
#define SLOTERROR        0
#define PORTERROR        1
#define WSCONFIGERROR    1
#define SOCKETCLOSE	 0

#define TRUE             1
#define FALSE            0
#define SUCCESS          1
#define FAIL             0

#define LINEBUFLENGTH    2000

unsigned char TIMEOUT=0;
char *ROOT_DIR;
int listenfd, clients[MAX_CONNECTIONS];
void error(char *);
void startWebServer();
void client_response(int);
char PORT[10];
char def_page[100];
struct itimerval timeout;
int ClientNumber;

/* Handles Time Out */
void alarm_handler(void)
{  
	printf("Entering alarm alarm_handler\n");
	printf("closing the socket %d\n", ClientNumber);
	shutdown (clients[ClientNumber], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[ClientNumber]);
	clients[ClientNumber]=-1;
   	exit(SOCKETCLOSE);
}
static unsigned int get_file_size (FILE * fileDescriptor)
{
    unsigned int size;

    fseek(fileDescriptor, 0L, SEEK_END);
    size = ftell(fileDescriptor);
    fseek(fileDescriptor, 0L, SEEK_SET);

    return size;
}

char *getExtension (char *filename) {
    char *extension = strrchr (filename, '.');
    return extension;
}

char* extractFileFormats(char *filetype)
{

    FILE *fp;
    char wsBuf[wsMaxSize];
    unsigned int formatIndex = 0;
    int file_supported = FAIL;
    char formats[20][100];
    char*file_format = NULL;

    char *wsConfigfile1 = getenv("PWD");
	if (wsConfigfile1 != NULL)
    	printf("Path to wsConfig file is in extractFileFormats is: %s \n", wsConfigfile1);

    fp=fopen(wsConfigfile1,"r");
    unsigned int wsConfigFileSize = get_file_size (fp);
    while(fgets(wsBuf,wsConfigFileSize,fp)!=NULL) {//read from the .conf file
        strcpy(formats[formatIndex],wsBuf);
        formatIndex++;
    }

    int k=0;

    
    for(k=0;k<formatIndex+1;k++) {
        if(strncmp(formats[k],filetype,4)==0) {//check if the file is supported
            file_supported = SUCCESS;//if supported then set file_supported.
	    file_format = strtok(formats[k], " \t\n");
	    file_format = strtok(NULL, " \t\n");
            break;
        }
    }

    fclose(fp);

    return file_format;
}


void readWSconfig()
{
    FILE *fp;
    char wsBuffer[wsMaxSize];
    char *val1;
    char *wsConffile = getenv("PWD");
	strncat(wsConffile,"/ws.conf", 8);
    printf("Path to ws.conf file is %s: \n", wsConffile);

    fp=fopen(wsConffile,"r");	//open ws.conf as read only

    if (fp == NULL)
    {

        perror("ws.conf");
        printf("Exiting the program\n");
        exit(WSCONFIGERROR);
    }

    else
    {
        unsigned int wsConfFileSize = get_file_size (fp);

        printf("ws.conf size n = %d, filename = %s\n", wsConfFileSize, wsConffile);


        while(fgets(wsBuffer,wsConfFileSize,fp)!=NULL) {

            /**********************
            * Finds Root directory
            /**********************/
            if(strncmp(wsBuffer,"DocumentRoot",12)==0) {
                printf("wsBuffer: %s",wsBuffer);
                val1=strtok(wsBuffer," \t\n"); //checks for space in wsBuffer
                val1 = strtok(NULL, " \t\n");
                ROOT_DIR=(char*)malloc(100);
                strcpy(ROOT_DIR,val1);
                printf("ROOT_DIR:%s",ROOT_DIR);
                bzero(wsBuffer, sizeof(wsBuffer));

            }

            /****************
            * Finds PORT NUM 
            /****************/
            if(strncmp(wsBuffer,"Listen",6)==0) {
                printf("wsBuffer: %s",wsBuffer);
                val1=strtok(wsBuffer," \t\n");	//checks for space in wsBuffer
                val1 = strtok(NULL, " \t\n");
                strcpy(PORT, val1);
                printf("PORT number: %s\n", PORT);
            	bzero(wsBuffer, sizeof(wsBuffer));
            }

            /****************
            * Finds Default page 
            /****************/
            if(strncmp(wsBuffer,"DirectoryIndex",14)==0) {
                printf("wsBuffer: %s",wsBuffer);
                val1=strtok(wsBuffer," \t\n");	//checks for space in wsBuffer
                val1 = strtok(NULL, " \t\n");
                strcpy(def_page, val1);
                printf("Directory Index: %s\n", def_page);
            	bzero(wsBuffer, sizeof(wsBuffer));
            }

            /****************
            * Finds timeout 
            /****************/
            if(strncmp(wsBuffer,"Keep-Alive time",15)==0) {
                printf("wsBuffer: %s",wsBuffer);
                val1=strtok(wsBuffer," \t\n");	//checks for space in wsBuffer
                val1 = strtok(NULL, " \t\n");
		val1 = strtok(NULL, " \t\n");
                TIMEOUT = atoi(val1);
                printf("TIMEOUT: %d\n", TIMEOUT);
            	bzero(wsBuffer, sizeof(wsBuffer));
            }
	

        }

        fclose(fp);
    }

}

int main(int argc, char* argv[])
{

    int connectionNumber=0;
    int i;

    struct sockaddr_in clientAddr;
    socklen_t addrlen;
    char c;    

    readWSconfig();     // reads the wsConfigFile
    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        clients[i]=-1;
    }

    int port_num = atoi(PORT);
    if (port_num < 1024)
    {
        fprintf(stderr, "The port number chosen is %d and is INVALID\n", port_num);
        exit(PORTERROR);
    }
    //if ()
    startWebServer();

    // ACCEPT connections
    while (1)
    {
        addrlen = sizeof(clientAddr);
	//printf("\n\raccepting now\n\r");
        clients[connectionNumber] = accept (listenfd, (struct sockaddr *) &clientAddr, &addrlen);
        if (clients[connectionNumber]<0)
            error ("accept() error");
        else
        {
            if ( fork()==0 )
            {
                client_response(connectionNumber);
                //exit(SLOTERROR);
            }
        }

        while (clients[connectionNumber]!=-1) 
            {
                connectionNumber = (connectionNumber+1)%MAX_CONNECTIONS;
            }
    }

    return 0;
}

//start server
void startWebServer(void)
{
    struct addrinfo webServerHints, *result, *rp;
    memset (&webServerHints, 0, sizeof(webServerHints)); // Making sure the struct is empty
    webServerHints.ai_family = AF_INET;                  // IPv4
    webServerHints.ai_socktype = SOCK_STREAM;            // TCP stream sockets
    webServerHints.ai_flags = AI_PASSIVE;


    /*int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **result);
    Given node and service, which identify an Internet host and a service, 
    getaddrinfo() returns one or more addrinfo structures, each of which contains an Internet address that can be specified in a call to bind(2) or connect(2). 
    */

    if (getaddrinfo( NULL, PORT, &webServerHints, &result) != 0)
    {
        perror ("ERROR in getaddrinfo()");
        exit(GETADDRINFOERROR);
    }
    /* getaddrinfo returns a list of address structures.
     * Try each address until we successfully bind. 
     * If socket() fails we try the next socket.
     */
    for (rp = result; rp!=NULL; rp=rp->ai_next)
    {
        if ((listenfd = socket (rp->ai_family, rp->ai_socktype, 0)) == -1)	//socket() failed, try next address
        {
            continue;
        }
        if (bind(listenfd, rp->ai_addr, rp->ai_addrlen) == 0) break;		//bind() success
    }
    /* No address succeedded */
    if (rp==NULL)
    {
        perror ("socket() or bind() creation failed");
        exit(BINDERROR);
    }

    freeaddrinfo(result); //No longer needed

    // listen for incoming connections
    if ( listen (listenfd, MAX_CONNECTIONS) != 0 )
    {
        perror("listen() error");
        exit(LISTENERROR);
    }
}

int COUNT = 0;
void client_response(int n)
{
	char mesgFromClient[MSG_SIZE], *requestline[3], data_to_send[BYTES], path[MSG_SIZE];
	int rcv, fd, bytes_read;
	FILE *fp;
	char status_line[LINEBUFLENGTH];
	char conn_stat[50];
	ClientNumber = n;
	int post_req_check=0;
	signal(SIGALRM, (void(*) (int)) alarm_handler);  /* Creating SIGALRM interrupt to implement timeout */
	timeout.it_value.tv_sec = TIMEOUT;
	timeout.it_value.tv_usec = 0;
	timeout.it_interval = timeout.it_value;

	printf("\n\r********************** ClientNumber : %d ************************\n",ClientNumber);
	while(1)
	{
		post_req_check = 0;  // make it zero before every post request
		/* Initialize arrays to zero */
		bzero(mesgFromClient, sizeof(mesgFromClient));
		bzero(requestline, sizeof(requestline));
		bzero(data_to_send, sizeof(data_to_send));
		bzero(path, sizeof(path));
		bzero(status_line, sizeof(status_line));
		bzero(conn_stat, sizeof(conn_stat));

		/* initialize mesgFromClient to all null characters */
		memset( (void*)mesgFromClient, (int)'\0', MSG_SIZE );
		//printf("\n\rreceiving now\n\r");
		rcv=recv(clients[n], mesgFromClient, MSG_SIZE, 0);	//receive message from client
		//printf("\n\rreceived message = %s", mesgFromClient);

		char filename[50] = "storeMsg";
		char count_str[50];
		char line_copy[99999];
		sprintf(count_str,"%d", COUNT);
		strcat(filename, count_str);
		FILE *fp_clientMsg = fopen(filename, "w");
		if (fp_clientMsg != NULL)
		{
			fputs(mesgFromClient, fp_clientMsg);
			fclose(fp_clientMsg);
		}
		if (!strstr(mesgFromClient,"Connection: Close"))    // capturing the last string from the received message
		{
			strncpy(conn_stat, "Connection: Keep-alive", strlen("Connection: Keep-alive"));
		}
		else    /* -- If Keep-alive is not found, close the connection --- */
		{
			strncpy(conn_stat, "Connection: Close",strlen("Connection: Close"));
			//exit(0);
			//break;
		}

		bzero(status_line, sizeof(status_line));
		if (rcv<0)    // receive error
			fprintf(stderr,("recv() error\n"));
		else if (rcv==0)    // receive socket closed
			rcv = 0;
		//fprintf(stderr,"Client disconnected upexpectedly.\n");

		else    // message received
		{
		// if another request occpurs pertaining to the same socket then TIMEOUT value has to be reset
			timeout.it_value.tv_sec = TIMEOUT;
			timeout.it_value.tv_usec = 0;
			timeout.it_interval = timeout.it_value;
			if (!strstr(mesgFromClient,"Connection: Close"))    // capturing the last string from the received message
			{
				strncpy(conn_stat, "Connection: Keep-alive", strlen("Connection: Keep-alive"));
			}
			else    /* -- If Keep-alive is not found, close the connection --- */
			{
				strncpy(conn_stat, "Connection: Close",strlen("Connection: Close"));
			}
			
			printf("\n## printing mesgFromClient %s\n", mesgFromClient);
			printf("\n## conn state %s\n", conn_stat);
			// Now breaking the incoming strng into three different paths
			requestline[0] = strtok (mesgFromClient, " \t\n");
			if ((strncmp(requestline[0], "GET\0", 4)==0) || (strncmp(requestline[0], "POST\0", 5)==0))
			{
			    if (strncmp(requestline[0], "POST\0", 5)==0)
			    {
			    //	printf("\n\r&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&Com\n");
			    	post_req_check = TRUE;
			    }

			    requestline[1] = strtok (NULL, " \t");
			    requestline[2] = strtok (NULL, " \t\n");
			    

			    char http_version[8];
			    if (strncmp(requestline[2], "HTTP/1.1", 8) == 0)
				strcpy(http_version, "HTTP/1.1");
			    else
				strcpy(http_version, "HTTP/1.0");


			    if ( strncmp( requestline[2], "HTTP/1.0", 8)!=0 && strncmp( requestline[2], "HTTP/1.1", 8)!=0 || strstr(requestline[1], " ") != NULL)
			    {
				strncat(status_line,http_version,strlen(http_version));
				strncat(status_line," 400 Bad Request",strlen(" 400 Bad Request"));
				strncat(status_line,"\n",strlen("\n"));//strncat(status_line,"\r\n",strlen("\r\n"));
				strncat(status_line,"Content-Type:",strlen("Content-type:"));
				strncat(status_line,"NONE",strlen("NONE"));
				strncat(status_line,"\n",strlen("\n"));
				strncat(status_line,"Content-Length:",strlen("Content-Length:"));
				strncat(status_line,"NONE",strlen("NONE"));
				strncat(status_line,"\n",strlen("\n"));
				strncat(status_line,conn_stat,strlen(conn_stat));
				strncat(status_line,"\r\n",strlen("\r\n"));
				strncat(status_line,"\r\n",strlen("\r\n"));
				strncat(status_line,"<HEAD><TITLE>400 Bad Request Reason</TITLE></HEAD>",strlen("<HEAD><TITLE>400 Bad Request Reason</TITLE></HEAD>"));
				if(strncmp( requestline[2], "HTTP/1.0", 8)!=0 && strncmp( requestline[2], "HTTP/1.1", 8)!=0){
					strncat(status_line,"<html><BODY>>400 Bad Request Reason: Invalid URL :",strlen("<BODY>>400 Bad Request Reason: Invalid URL :"));
				}
				else{
					strncat(status_line,"<html><BODY>>400 Bad Request Reason: Invalid URL :",strlen("<BODY>>400 Bad Request Reason: Invalid URL :"));
				}
				strncat(status_line,"HTTP",strlen("HTTP"));
				strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
				strncat(status_line,"\r\n",strlen("\r\n"));
				printf("%s\n",status_line);
				write(clients[n], status_line, strlen(status_line));
			    }
			    else //good URL
			    {
				if ( strncmp(requestline[1], "/\0", 2)==0 )
				    strcat(requestline[1], def_page);
				strcpy(path, ROOT_DIR);
				strcpy(&path[strlen(ROOT_DIR)], requestline[1]);
				printf("file: %s\n", path);
				char* formatChk;
				char *ext = getExtension(path);
				if (ext == NULL)
				{
				    formatChk = NULL;
				}
				else
				{
				    formatChk = extractFileFormats(ext);
				    printf("\n\rformatchk = %s\n\r", formatChk);
				}
				char size_array[20];
				char tempp = 0;
				if (formatChk != NULL)
				{
				    if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
				    {
					fp = fopen(path,"r");
					int size= get_file_size(fp);
					sprintf(size_array,"%d",size);
					char msgRecvPost[99999];
					if (post_req_check)
					{
					    FILE * fp_chck_storeMsg;
					    char * line = NULL;
					    size_t len = 0;
					    ssize_t read;
					    int emptyLineFound = FALSE;
					    unsigned int con_len=0;
					    fp_chck_storeMsg = fopen(filename, "r");
					    if (fp_chck_storeMsg == NULL)
						exit(EXIT_FAILURE);
					    while ((read = getline(&line, &len, fp_chck_storeMsg)) != -1) {
					    	if(strncmp(line, "Content-Length", 14) == 0){
							char *temp = strtok(line, " \n\r");
							temp = strtok(NULL, " \n\r");
							con_len = atoi(temp);
							printf("\n\rCon len = %d", con_len);
							break;
						}
					    }
					    while ((read = getline(&line, &len, fp_chck_storeMsg)) != -1) {
						if(read>=2){
							printf("\n\rRead bytes = %d", (int)read);
							printf("\n\rLine read: %s", line);
							tempp++;
							if(tempp>1) strncat(msgRecvPost, line, strlen(line));					
						}
					    }
					    
					    if(tempp<2){
						memset( (void*)mesgFromClient, (int)'\0', MSG_SIZE );
						rcv=recv(clients[n], mesgFromClient, MSG_SIZE, 0);	//receive message from client
					    	*(mesgFromClient + con_len) = '\0';
					   	fp_clientMsg = fopen(filename, "w");
					    	if (fp_clientMsg != NULL)
					    	{
					     		fputs(mesgFromClient, fp_clientMsg);
					    		fclose(fp_clientMsg);
					   	}
					    	fp_chck_storeMsg = fopen(filename, "r");
					    	if (fp_chck_storeMsg == NULL)
							exit(EXIT_FAILURE);


					   	 while ((read = getline(&line, &len, fp_chck_storeMsg)) != -1) {
					    	
							strncat(msgRecvPost, line, strlen(line));
	
					  	  }

					    }
					    COUNT++;
					    printf("\n\rcoming into post loop\n");
					    printf("						%s\n", msgRecvPost);	
					    fclose(fp_chck_storeMsg);
					    remove(filename);
					    if (line)
					    free(line);
					}
					char msg[99999];
					strncat(status_line,http_version,strlen(http_version));
					strncat(status_line," 200 OK",strlen(" 200 OK"));
					strncat(status_line,"\n",strlen("\n"));
					strncat(status_line,"Content-Type:",strlen("Content-type:"));
					strncat(status_line,formatChk,strlen(formatChk));
					strncat(status_line,"\n",strlen("\n"));
					strncat(status_line,"Content-Length:",strlen("Content-Length:"));
					strncat(status_line,size_array,strlen(size_array));
					strncat(status_line,"\n",strlen("\n"));
					strncat(status_line,conn_stat,strlen(conn_stat));
					if (post_req_check){
						strncat(status_line,"\r\n\r\n",strlen("\r\n\r\n"));
						sprintf(msg,"<html><body><pre><h1>%s</h1></pre>",msgRecvPost);
					}
					else{
						strncat(status_line,"\r\n",strlen("\r\n"));
						strncat(status_line,"\r\n",strlen("\r\n"));
					}
					printf("printing status_line %s\n",status_line);
					send(clients[n], status_line, strlen(status_line), 0);
					if(post_req_check) write(clients[n], msg, strlen(msg));
					while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
					    write (clients[n], data_to_send, bytes_read);

					fclose(fp);
					bzero(msgRecvPost,sizeof(msgRecvPost));
					bzero(msg,sizeof(msg));
				    }
		
				    else{  // file not found loop

				    	strncat(status_line,http_version,strlen(http_version));
					strncat(status_line," 404 Not Found",strlen(" 404 Not Found"));
					strncat(status_line,"\n",strlen("\n"));
					strncat(status_line,"Content-Type:",strlen("Content-type:"));
					strncat(status_line,"Invalid",strlen("Invalid"));
					strncat(status_line,"\n",strlen("\n"));
					strncat(status_line,"Content-Length:",strlen("Content-Length:"));
					strncat(status_line,"Invalid",strlen("Invalid"));
					strncat(status_line,"\n",strlen("\n"));
					strncat(status_line,conn_stat,strlen(conn_stat));                     
					strncat(status_line,"\r\n",strlen("\r\n"));
					strncat(status_line,"\r\n",strlen("\r\n"));
					strncat(status_line,"<html><BODY>404 Not Found: URL does not exist:",strlen("<BODY>404 Not Found: URL does not exist:"));
					strncat(status_line,path,strlen(path));
					strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
					strncat(status_line,"\r\n",strlen("\r\n"));
					printf("%s\n",status_line);
					write(clients[n], status_line, strlen(status_line)); //FILE NOT FOUND
					    
				    }    
				
				}

				else // file not supported
				{
				    printf("**********************************************************************	\n");
				    strncat(status_line,http_version,strlen(http_version));
				    strncat(status_line," 501 Not Implemented",strlen(" 501 Not Implemented"));
				    strncat(status_line,"\n",strlen("\n"));//strncat(status_line,"\r\n",strlen("\r\n"));
				    strncat(status_line,"Content-Type:",strlen("Content-type:"));
				    strncat(status_line,"NONE",strlen("NONE"));
				    strncat(status_line,"\n",strlen("\n"));
				    strncat(status_line,"Content-Length:",strlen("Content-Length:"));
				    strncat(status_line,"NONE",strlen("NONE"));
				    strncat(status_line,"\n",strlen("\n"));
				    strncat(status_line,conn_stat,strlen(conn_stat));                    
				    strncat(status_line,"\r\n",strlen("\r\n"));
				    strncat(status_line,"\r\n",strlen("\r\n"));
				    strncat(status_line,"<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>",strlen("<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>"));
				    strncat(status_line,"<BODY>501 Not Implemented: File format not supported:",strlen("<BODY>501 Not Implemented: File format not supported:"));
				    strncat(status_line,http_version,strlen(http_version));
				    strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
				    strncat(status_line,"\r\n",strlen("\r\n"));
				    write(clients[n], status_line, strlen(status_line)); //FILE NOT FOUND   
				}

			    }
			}
			/* HEAD, PUT< CONNECT< OPTIONS, TRACE, PATCH not implemented */
			else if ((strncmp(requestline[0], "HEAD\0", 4)==0) || (strncmp(requestline[0], "PUT\0", 3)==0) || (strncmp(requestline[0], "DELETE\0", 6)==0)|| (strncmp(requestline[0], "CONNECT\0", 7)==0)|| (strncmp(requestline[0], "OPTIONS\0", 6)==0)|| (strncmp(requestline[0], "TRACE\0", 5)==0)|| (strncmp(requestline[0], "PATCH\0", 5)==0))
			{
				strncat(status_line,"HTTP/1.1",strlen("HTTP/1.1"));
				strncat(status_line,"\n",strlen("\n"));//strncat(status_line,"\r\n",strlen("\r\n"));
				strncat(status_line,"Content-Type:",strlen("Content-type:"));
				strncat(status_line,"NONE",strlen("NONE"));
				strncat(status_line,"\n",strlen("\n"));
				strncat(status_line,"Content-Length:",strlen("Content-Length:"));
				strncat(status_line,"NONE",strlen("NONE"));
				strncat(status_line,"\r\n",strlen("\r\n"));
				strncat(status_line,"\r\n",strlen("\r\n"));
				strncat(status_line,"<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>",strlen("<HEAD><TITLE>501 Not Implemented</TITLE></HEAD>"));
				strncat(status_line,"<BODY>501 Not Implemented: Method not implemented:",strlen("<BODY>501 Not Implemented: Method not implemented:"));
				strncat(status_line,"HTTP/1.1",strlen("HTTP/1.1"));
				strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
				strncat(status_line,"\r\n",strlen("\r\n"));
				write(clients[n], status_line, strlen(status_line)); //FILE NOT FOUND   
			 }
			/* Server Error */
			else
			{
				strncat(status_line,"HTTP/1.1",strlen("HTTP/1.1"));
				strncat(status_line,"\n",strlen("\n"));
				strncat(status_line,"Content-Type:",strlen("Content-type:"));
				strncat(status_line,"NONE",strlen("NONE"));
				strncat(status_line,"\n",strlen("\n"));
				strncat(status_line,"Content-Length:",strlen("Content-Length:"));
				strncat(status_line,"NONE",strlen("NONE"));
				strncat(status_line,"\r\n",strlen("\r\n"));
				strncat(status_line,"\r\n",strlen("\r\n"));
				strncat(status_line,"<HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>",strlen("<HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>"));
				strncat(status_line,"<BODY>500 Internal Server Error:cannot allocate memory",strlen("<BODY>500 Internal Server Error:cannot allocate memory"));
				strncat(status_line,"HTTP/1.1",strlen("HTTP/1.1"));
				strncat(status_line,"</BODY></html>",strlen("</BODY></html>"));
				strncat(status_line,"\r\n",strlen("\r\n"));
				write(clients[n], status_line, strlen(status_line)); //FILE NOT FOUND   
			 }

		}//message received end
		if (strstr(conn_stat,"Connection: Close") == NULL)    // capturing the last string from the received message
		{
		    setitimer(ITIMER_REAL,&timeout,NULL);
		 //   printf("\n\rSetting timer\n\r");
		}
		else    /* -- If Keep-alive is not found, close the connection --- */
		{
		    shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
		    close(clients[n]);
		    clients[n]=-1;
		    exit(0);
		}
    }

}

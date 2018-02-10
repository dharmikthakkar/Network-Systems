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
#include <openssl/md5.h>
#include <time.h>

// defines for connection
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


#define MAX_BUFFER_SIZE 9999

char PORT[10];
char filebuffer[MAX_BUFFER_SIZE];
char status_line[MAX_BUFFER_SIZE];
int listenfd, clients[MAX_CONNECTIONS];

void error(char *);
void startWebServer();
void respond(int sockfd, char *timeout, char *pwd);
char * checkContentType(char *);
int cachePresent(char *timeoutStr, char *pwd, int sockfd);


/************************************************
starting TCP socket and binding it
************************************************/
void startWebServer()
{
    struct addrinfo webServerHints, *res, *p;
    memset (&webServerHints, 0, sizeof(webServerHints)); // Making sure the struct is empty
    webServerHints.ai_family = AF_INET;                  // IPv4
    webServerHints.ai_socktype = SOCK_STREAM;            // TCP stream sockets
    webServerHints.ai_flags = AI_PASSIVE;
    int yes = 1;
    // getting address
    if (getaddrinfo( NULL, PORT, &webServerHints, &res) != 0)
    {
        perror ("ERROR in getaddrinfo()");
        exit(GETADDRINFOERROR);
    }
    // Bind the sock address
    for (p = res; p!=NULL; p=p->ai_next)
    {
        if ((listenfd = socket (p->ai_family, p->ai_socktype, 0)) == -1)
        {
            continue;
        }
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("setsockopt");
		exit(1);
		
	}
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
    }

    if (p==NULL)
    {
        perror ("socket() or bind() creation failed");
        exit(BINDERROR);
    }

    freeaddrinfo(res);
    
    // listen for incoming connections
    if ( listen (listenfd, MAX_CONNECTIONS) != 0 )
    {
        perror("listen() error");
        exit(LISTENERROR);
    }
}

/************************************************
Finds the creation time of the file passed 
Input arguments
    pwd: filename
    timecreated: returns the time value created for the file
************************************************/
void getFileCreationTime(char *pwd, char timeCreated[1000]) {
    struct stat attr;
    stat(pwd, &attr);
    char date[10];
    sprintf(timeCreated,"%s",ctime(&attr.st_mtime));
}


/************************************************
Computed the md5sum of the given file
Input arguments
    str: filename
    length of the md5sum request 
************************************************/
char *computeMD5Sum(const char *str, int length) 
{
    int n;
    MD5_CTX mdc;
    unsigned char digest[16];
    char *out = (char*)malloc(33);
    MD5_Init(&mdc);
    while (length > 0) {
        if (length > 512) {
            MD5_Update(&mdc, str, 512);
        } 
	else {
            MD5_Update(&mdc, str, length);
        }
        length -= 512;
        str += 512;
    }
    MD5_Final(digest, &mdc);
    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }
    return out;
}



int cachePresent(char *timeoutStr, char *pwd, int sockfd)
{
    int timeout = atoi(timeoutStr);
    char timeCreated[1000];
    int nbytes;
    char bufferTosend[MAX_BUFFER_SIZE];
    getFileCreationTime(pwd, timeCreated);
    printf("timeCreated %s\n", timeCreated);
    FILE *sendFilefd;
    /* Check if file is available */
    if(access(pwd, F_OK) != -1 ) 
    {
        // file available
        char *hrs, *mins,*sec;
        hrs = strtok(timeCreated,":") ;
        mins = strtok(NULL,":") ;
        sec = strtok(NULL,":") ;
        sec = strtok(sec, " ");
        hrs = strtok(hrs," ") ;
        hrs = strtok(NULL," ");
        hrs = strtok(NULL," ");
        hrs = strtok(NULL," ");
        int fileCreatTime = atoi(hrs)*3600 + atoi(mins)*60 + atoi(sec); 
        time_t curr_time;
        time(&curr_time);
        bzero(timeCreated,sizeof(timeCreated));
        //strcpy(timeCreated,ctime(&curr_time));
        sprintf(timeCreated,"%s", ctime(&curr_time));
        printf("presentTime %s\n", timeCreated);
        hrs = strtok(timeCreated,":") ;
        mins = strtok(NULL,":") ;
        sec = strtok(NULL,":") ;
        sec = strtok(sec, " ");
        hrs = strtok(hrs," ") ;
        hrs = strtok(NULL," ");
        hrs = strtok(NULL," ");
        hrs = strtok(NULL," ");
        int presTime = atoi(hrs)*3600 + atoi(mins)*60 + atoi(sec); 

        printf("Present time %d\n", presTime);
        printf("File time %d \n", fileCreatTime);
        printf("Time difference %d\n", presTime - fileCreatTime);
	/* If timeout has excedded return false */
        if ((presTime - fileCreatTime) > timeout)
        {
            printf("TIMEOUT EXPIRED\n");
            return FAIL;
        }
	/* else send file from cache */
        else
        {
            printf("\n\r\n\r********************* SENDING FILE FROM CACHE MEMORY *********************\n");
            bzero(bufferTosend,sizeof(bufferTosend));
            printf("Sending cached file\n");
            sendFilefd=fopen(pwd,"r");
            do{
                nbytes = fread(bufferTosend,1,MAX_BUFFER_SIZE,sendFilefd);
                send(sockfd,bufferTosend,nbytes,0);
            }
            while(nbytes>0);
            fclose(sendFilefd);

//            close(sockfd);
            return SUCCESS;
        }
    }
    /* Send FAIL if file is not available */
    else
    {
        return FAIL;
    }
}


/*************************************************************************
This function performs the link prefetching operation
****************************************************************************/

#define FILE_SIZE 1000000

void linkPrefetching(char * pwd, int sockfd)
{
    printf("**********     Entering prefetching    **********\n");
    char buffToSend[FILE_SIZE];
    char filePath[MAX_BUFFER_SIZE];
    char prefetchReq[MAX_BUFFER_SIZE];
    char buff[MAX_BUFFER_SIZE];
    int readFile,n;
    char chk[MAX_BUFFER_SIZE];
    int i,openFile;
    char *md5Sum;  
    FILE *fp;   
    char newVar[MAX_BUFFER_SIZE];
    char * ret;
    char *retVal;
    char linkpath[MAX_BUFFER_SIZE];
    char ipaddress[MAX_BUFFER_SIZE];
    int len;
    int hostfd;
    int on=1;
    struct sockaddr_in hostAddr;
    struct hostent *hostToconnect;
    bzero(buffToSend,sizeof(buffToSend));
    bzero(filePath,sizeof(filePath));
    bzero(prefetchReq,sizeof(prefetchReq));
    bzero(chk,sizeof(chk));
    bzero(newVar,sizeof(newVar));
    bzero(linkpath,sizeof(linkpath));
    bzero(buff,sizeof(buff));

    // opening the file from current pwd
    openFile=open(pwd,O_RDONLY);
    if(openFile==-1){
        printf("\n error in reading the file");
    }

    // reading file
    readFile = read(openFile,buffToSend, sizeof buffToSend);
    if (readFile < 0)
    {
        printf("FILE reading error\n");
    }

    char *newPtr;

    // check if href is present
    if((ret=strstr(buffToSend,"href=\"http://"))!= NULL){
        while((ret=strstr(ret,"href=\"http://")) ){
            ret = ret+13;
            i=0;

            // extracting the http url after href command
            while(*ret!='"')
                {
                    newVar[i] = *ret;   
                    printf("%c",*ret);
                    ret++;
                    i++;
                }
            newPtr=ret;

            newVar[i]='\0';

            // computing the md5sum of the file 
	    strcpy(chk,"http://");
            strcat(chk, newVar);
	   // strcat(chk,linkpath);
	    printf("\n\r\n\r&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& chk=%s&&&&&&&\n\r\n\r\n\r", chk);
            md5Sum = computeMD5Sum(chk,strlen(chk));
            

            printf("\n md5 sum of the link prefetching file:%s",md5Sum);
            strcpy(filePath,"./cache/");
            strcat(filePath,md5Sum);
            strcat(filePath,".html");
            printf("filepath: %s",filePath);
            retVal = strstr(newVar,"/");
            if ( retVal == NULL){
              continue;
            }
            if(retVal!=NULL){
                strcpy(linkpath,retVal);
            }
            
            *retVal='\0';
            ret =newPtr +1;
				//newVar contains website. LinkPath contains path
	/***********begins**************/
                fp=fopen(filePath,"ab");
                if (fp < 0)
                {
                    perror("FILE ERROR in RESPOND");
                }
		/* Check if IP address for the given website already exists */
		FILE *fh=fopen("hostnames", "r");
		    if(fh == NULL){
			printf("\n\rCannot read from the file\n\r");
			exit(0);
		    }	
		bzero(ipaddress, sizeof(ipaddress));
		while(fgets(filebuffer, sizeof(filebuffer), fh) != NULL){
			filebuffer[strlen(filebuffer)-1] = '\0';
			if(strcmp(filebuffer, newVar) == 0){
				fgets(ipaddress, sizeof(ipaddress), fh);
				ipaddress[strlen(ipaddress)-1] = '\0';
				break;
			}
		}
		fclose(fh);
		//if IP address already exists
		if(ipaddress[0] != 0){
			printf("\n\r\n\r****************Website Exists with IP addr = %s****************\n\r\n\r", ipaddress);
		        bzero(&hostAddr,sizeof(hostAddr));                    //zero the struct
		        hostAddr.sin_family = AF_INET;                   //address family
		        hostAddr.sin_port = htons(80);        //htons() sets the port # to network byte order
			inet_pton(AF_INET, ipaddress, &(hostAddr.sin_addr));
			//strcpy(filebuffer, ipaddress);
			len = sizeof(hostAddr);
		}
		//Else do DNS Query
		else{
			printf("\n\r\n\r****************DNS Query for %s****************\n\r\n\r", newVar);
                	hostToconnect = gethostbyname(newVar);
		        if (!hostToconnect)
		        {
		            perror("Inavlid host address");
		            exit(1);
		        }

		        // parameters to connect to host server
		        bzero(&hostAddr,sizeof(hostAddr));                    //zero the struct
		        hostAddr.sin_family = AF_INET;                   //address family
		        hostAddr.sin_port = htons(80);        //htons() sets the port # to network byte order
		        memcpy(&hostAddr.sin_addr, hostToconnect->h_addr, hostToconnect->h_length);
		        len = sizeof(hostAddr);
			sprintf(ipaddress, "%s", inet_ntoa(*((struct in_addr *)hostToconnect->h_addr)));
			fh = fopen("hostnames","ab");
			if(fh == NULL){
				printf("\n\rCannot read from the file\n\r");
				exit(0);
			}
			fwrite(newVar, 1, strlen(newVar), fh);
			fwrite("\n", 1, 1, fh);
			fwrite(ipaddress, 1, strlen(ipaddress), fh);
			fwrite("\n", 1, 1, fh);
			fclose(fh);
		}

		/* Check if the extracted IP address is to be blocked */
		    FILE *fblock=fopen("blockedSites", "r");
		    if(fblock == NULL){
			printf("\n\rCannot read from the file\n\r");
			exit(0);
		    }	
		    while(fgets(filebuffer, sizeof(filebuffer), fblock) != NULL){
			filebuffer[strlen(filebuffer)-1] = '\0';
		//printf("********************filebuffer=%s of len %d, website=%s of len %d", filebuffer, strlen(filebuffer), website, strlen(website));
			if(strcmp(filebuffer, ipaddress) == 0 || (strcmp(filebuffer, newVar) == 0)){
			//	write(sockfd,Website_Blocked,strlen(Website_Blocked));
				printf("\n\r\n\r****************Website Blocked****************\n\r\n\r");
				continue;
			//	shutdown(sockfd, SHUT_RDWR);
			//	close(sockfd);
			//	exit(1);
			}
		    }
		    fclose(fblock);

                hostfd = socket(AF_INET, SOCK_STREAM, 0);
                if (hostfd<0)
                {
                    perror("HOST socket creation failed");
                }
                setsockopt(hostfd, SOL_SOCKET, SO_REUSEADDR, &on, 4);   //set option for reuse address

                int sckt = connect(hostfd, (struct sockaddr *) &hostAddr, len);
                if (sckt < 0) 
                {
                    printf("Connection problem\n");
                    close(hostfd);
                }

                // creating url to send to host
             //   if (linkpath != 0)
          //  printf("\nGET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",linkpath,newVar);
                    sprintf(prefetchReq,"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",linkpath,newVar);
               // else
                 //   sprintf(reqToHost,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",httpVer,website);
                printf("requesToHost %s\n", prefetchReq);
      
                // sending request to host server- Ex www.google.com
                n = send(hostfd,prefetchReq,strlen(prefetchReq),0);
                if (n < 0)
                {
                    perror("Host send failed");
                }

                else
                {
                    printf("\n\n*****Sarted sending file from server*****\n\n");
                    do{
                        bzero((char*)buff,sizeof(buff));
                        n=recv(hostfd,buff,sizeof(buff),0);        // receiving from host server
                        if(!(n<=0))
                        {
                            fwrite(buff,1,n,fp);
                        }  
                    }while(n>0);
                }
                fclose(fp);


	/*************ends***********/
        }
    }


}


/*************************************************************************
This function receives request from client
Parses the request in appropriate format
****************************************************************************/
void respond(int sockfd, char *timeout, char *pwd)
{
    char readBufFrmClient[MAX_BUFFER_SIZE];
    char readBufFrmServer[MAX_BUFFER_SIZE];
    char method[MAX_BUFFER_SIZE];
    char path[MAX_BUFFER_SIZE];
    char httpVer[MAX_BUFFER_SIZE];
    char reqToHost[MAX_BUFFER_SIZE];
    char ipaddress[MAX_BUFFER_SIZE];
    char md5sum[100];
    char website[MAX_BUFFER_SIZE];
    char *url = NULL;
    char *webWithSlash = NULL;
    char *buffer;
    struct hostent *hostToconnect;
    struct sockaddr_in hostAddr;
    int on = 1;
    int findCache;
    int hostfd;    //connects to host
    int nbytes;      
    int len;
    char fileName[MAX_BUFFER_SIZE];
    FILE *fileProxy;


    bzero(readBufFrmClient, sizeof(readBufFrmClient));
    bzero(method, sizeof(method));
    bzero(path, sizeof(path));
    bzero(httpVer, sizeof(httpVer));
    bzero(website, sizeof(website));
    bzero(md5sum, sizeof(md5sum));


    char Invalid_Method[MAX_BUFFER_SIZE] = "<html><body><H1>Error 400 Bad Request: Invalid Method </H1></body></html>";
    char Invalid_version[MAX_BUFFER_SIZE] =  "<html><body><H1>Error 400 Bad Request: Invalid HTTP Version</H1></body></html>";
    char Website_Blocked[MAX_BUFFER_SIZE] =  "<html><body><H1>Error 403 Forbidden Reason: Blocked Website</H1></body></html>";    
    if (read(sockfd, readBufFrmClient, MAX_BUFFER_SIZE)<0)
    {
        printf("recieve error\n");
    }


    // On receiving valid data
    else
    {
        sscanf(readBufFrmClient,"%s %s %s",method,path,httpVer);
        printf("readBufFrmClient: %s %s %s\n",method,path,httpVer); 

        // checking validity of request method
        if (strncmp(method,"GET",strlen("GET")) != 0)
        {
            write(sockfd,Invalid_Method,strlen(Invalid_Method));
            printf("Invalid Request method\n");
	    shutdown(sockfd, SHUT_RDWR);
	    close(sockfd);
            exit(1);
        }

        // checking validity of HTTP method
        else if ((strncmp(httpVer,"HTTP/1.0",strlen("HTTP/1.0")) != 0)  && (strncmp(httpVer,"HTTP/1.1",strlen("HTTP/1.1")) != 0))
        {
            write(sockfd,Invalid_version,strlen(Invalid_version));
            printf("Invalid HTTP Version");
	    shutdown(sockfd, SHUT_RDWR);
	    close(sockfd);
            exit(1);
        }

        else
        {
            int i=0;
            webWithSlash= strstr(path,"//");
            webWithSlash+=2;

            for(i=0;i<strlen(webWithSlash);i++)
            {
                if(webWithSlash[i]=='/')
                    break;
                website[i]=webWithSlash[i];
            }  
            url=strstr(webWithSlash,"/"); 
            printf("\n website: %s\n", website);
            printf("\n path to url: %s\n", url);
	/*Check if webiste is to be blocked */
	    FILE *fblock=fopen("blockedSites", "r");
	    if(fblock == NULL){
		printf("\n\rCannot read from the file\n\r");
		    shutdown(sockfd, SHUT_RDWR);
		    close(sockfd);

		exit(0);
	    }	
	    while(fgets(filebuffer, sizeof(filebuffer), fblock) != NULL){
		filebuffer[strlen(filebuffer)-1] = '\0';
		//printf("********************filebuffer=%s of len %d, website=%s of len %d", filebuffer, strlen(filebuffer), website, strlen(website));
		if(strcmp(filebuffer, website) == 0){
			write(sockfd,Website_Blocked,strlen(Website_Blocked));
			printf("\n\r\n\r****************Website Blocked****************\n\r\n\r");
			shutdown(sockfd, SHUT_RDWR);
			close(sockfd);
			exit(1);
		}
	    }
	    fclose(fblock);
	    printf("\n\r\n\r\n\r@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@path=%s@@@@@@@@@@@@@@@@@@@@@@@\n\r\n\r\n\r", path);
            /*
	    MD5_CTX mdContext;
            MD5_Init(&mdContext);
            MD5_Update (&mdContext, path, strlen(path));
            MD5_Final (md5sum, &mdContext);

            // computes the md5sum of the file
            for (i = 0; i< MD5_DIGEST_LENGTH; i++)
            {
                sprintf(&buffer[2*i],"%02x", md5sum[i]);
            }
	    */
	    buffer = computeMD5Sum(path,strlen(path));
            sprintf(fileName,"%s.html",buffer);
            printf("md5sum %s\n", buffer);
            sprintf(pwd,"%s%s",pwd, fileName);
            printf("pwd with filename%s\n", pwd);

            // find if the file is present in cache or not
            findCache = cachePresent(timeout, pwd, sockfd);
            printf("Printing the status of cache %d\n", findCache);

            // if file doesn't exit on cache get it from server
            if (findCache == FAIL)
            {
                printf("********* NO CACHE FOUND *********\n\n");
                printf("Extracting file from host server\n");

                fileProxy = fopen(pwd,"ab");
                if (fileProxy < 0)
                {
                    perror("FILE ERROR in RESPOND");
                }
		/* Check if IP address for the given website already exists */
		FILE *fh=fopen("hostnames", "r");
		    if(fh == NULL){
			printf("\n\rCannot read from the file\n\r");
	 	 	  shutdown(sockfd, SHUT_RDWR);
	 		   close(sockfd);

			exit(0);
		    }	
		bzero(ipaddress, sizeof(ipaddress));
		while(fgets(filebuffer, sizeof(filebuffer), fh) != NULL){
			filebuffer[strlen(filebuffer)-1] = '\0';
			if(strcmp(filebuffer, website) == 0){
				fgets(ipaddress, sizeof(ipaddress), fh);
				ipaddress[strlen(ipaddress)-1] = '\0';
				break;
			}
		}
		fclose(fh);
		//if IP address already exists
		if(ipaddress[0] != 0){
			printf("\n\r\n\r****************Website Exists with IP addr = %s****************\n\r\n\r", ipaddress);
		        bzero(&hostAddr,sizeof(hostAddr));                    //zero the struct
		        hostAddr.sin_family = AF_INET;                   //address family
		        hostAddr.sin_port = htons(80);        //htons() sets the port # to network byte order
			inet_pton(AF_INET, ipaddress, &(hostAddr.sin_addr));
			//strcpy(filebuffer, ipaddress);
			len = sizeof(hostAddr);
		}
		//Else do DNS Query
		else{
			printf("\n\r\n\r****************DNS Query for %s****************\n\r\n\r", website);
                	hostToconnect = gethostbyname(website);
		        if (!hostToconnect)
		        {
		            perror("Inavlid host address");
			    shutdown(sockfd, SHUT_RDWR);
			    close(sockfd);
		            exit(1);
		        }

		        // parameters to connect to host server
		        bzero(&hostAddr,sizeof(hostAddr));                    //zero the struct
		        hostAddr.sin_family = AF_INET;                   //address family
		        hostAddr.sin_port = htons(80);        //htons() sets the port # to network byte order
		        memcpy(&hostAddr.sin_addr, hostToconnect->h_addr, hostToconnect->h_length);
		        len = sizeof(hostAddr);
			sprintf(ipaddress, "%s", inet_ntoa(*((struct in_addr *)hostToconnect->h_addr)));
			fh = fopen("hostnames","ab");
			if(fh == NULL){
				printf("\n\rCannot read from the file\n\r");
	   			 shutdown(sockfd, SHUT_RDWR);
	  			  close(sockfd);
				exit(0);
			}
			fwrite(website, 1, strlen(website), fh);
			fwrite("\n", 1, 1, fh);
			fwrite(ipaddress, 1, strlen(ipaddress), fh);
			fwrite("\n", 1, 1, fh);
			fclose(fh);
		}

		/* Check if the extracted IP address is to be blocked */
		    fblock=fopen("blockedSites", "r");
		    if(fblock == NULL){
			printf("\n\rCannot read from the file\n\r");
	 		   shutdown(sockfd, SHUT_RDWR);
			    close(sockfd);

			exit(0);
		    }	
		    while(fgets(filebuffer, sizeof(filebuffer), fblock) != NULL){
			filebuffer[strlen(filebuffer)-1] = '\0';
		//printf("********************filebuffer=%s of len %d, website=%s of len %d", filebuffer, strlen(filebuffer), website, strlen(website));
			if(strcmp(filebuffer, ipaddress) == 0){
				write(sockfd,Website_Blocked,strlen(Website_Blocked));
				printf("\n\r\n\r****************Website Blocked****************\n\r\n\r");
				shutdown(sockfd, SHUT_RDWR);
				close(sockfd);
				exit(1);
			}
		    }
		    fclose(fblock);

                hostfd = socket(AF_INET, SOCK_STREAM, 0);
                if (hostfd<0)
                {
                    perror("HOST socket creation failed");
                }
                setsockopt(hostfd, SOL_SOCKET, SO_REUSEADDR, &on, 4);   //set option for reuse address

                int sckt = connect(hostfd, (struct sockaddr *) &hostAddr, len);
                if (sckt < 0) 
                {
                    printf("Connection problem\n");
                    close(hostfd);
                }

                // creating url to send to host
                if (url != 0)
                    sprintf(reqToHost,"GET %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",url,httpVer,website);
                else
                    sprintf(reqToHost,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",httpVer,website);
                printf("requesToHost %s\n", reqToHost);
      
                // sending request to host server- Ex www.google.com
                nbytes = send(hostfd,reqToHost,sizeof(reqToHost),0);
                if (nbytes < 0)
                {
                    perror("Host send failed");
                }

                else
                {
                    printf("\n\n*****Sarted sending file from server*****\n\n");
                    do{
                        bzero((char*)readBufFrmServer,MAX_BUFFER_SIZE);
                        nbytes=recv(hostfd,readBufFrmServer,sizeof(readBufFrmServer),0);        // receiving from host server
                        if(!(nbytes<=0))
                        {
                            send(sockfd,readBufFrmServer,nbytes,0);                                 // sending to client 
                            fwrite(readBufFrmServer,1,nbytes,fileProxy);
                        }  
                    }while(nbytes>0);

                    
                    printf("\n\n********************* Starting the Prefeting operation *********************\n\n");
                    linkPrefetching(pwd,hostfd);
                    printf("****************************** linkPrefetching Done ********************************\n" );
                }
                fclose(fileProxy);
            }
        }

    }
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    close(hostfd);
    printf("Closing the sockets........................\n");
    exit(0);
}



/**********************************************
The socket creation and fork operaion of main() and a part of respond() is 
referenced from the follwing link
Ref : http://blog.abhijeetr.com/2010/04/very-simple-http-server-writen-in-c.html
**********************************************/

int main(int argc, char* argv[])
{


    char pwd[MAX_BUFFER_SIZE];  // Current working direcroty
    char timeout[10];
    int sock_n;
    if (argc<3)
    {
        printf("Invalid argumants: ./webproxy <port_num> <Cache Timeout>\n");
        exit(1);
    }

    printf("\n\nPORT number is: %s\n\n", argv[1]);
    printf("\n\nCache Timeout is: %s\n\n", argv[2]);
    strcpy(PORT,argv[1]);
    strcpy(timeout,argv[2]);

    int pid;
    int connNum=0;
    int i;

    struct sockaddr_in clientAddr;
    socklen_t addrlen;
    char c;    

    char parentdir[MAX_BUFFER_SIZE];
    char cmdCreateCache[MAX_BUFFER_SIZE];


    // Finds the current working directory
    if (getcwd(parentdir, sizeof(parentdir)) != NULL)
    {
        printf("Current working dir: %s\n", parentdir);
        sprintf(pwd,"%s/cache/",parentdir);
        sprintf(cmdCreateCache,"mkdir -p %s",pwd);
        system(cmdCreateCache);
    }
    printf("Current working Directory: %s\n",pwd);

    for (i=0; i<MAX_CONNECTIONS; i++)
    {
        clients[i]=-1;
    }
    int port_num = atoi(PORT);

    // Exit if port number less than 1024
    if (port_num < 1024)
    {
        fprintf(stderr, "The port number chosen is %d and is INVALID\n", port_num);
        exit(PORTERROR);
    }
    
    //start the server
    startWebServer();

    int connCount = 0;
    // ACCEPT connections
    while (1)
    {
        addrlen = sizeof(clientAddr);
        // accepting the connections and forking it into a child 
        
        clients[connNum] = accept (listenfd, (struct sockaddr *) &clientAddr, &addrlen);
        //printf("Coming here again\n");
        if (clients[connNum]<0)
            error ("accept() error");
        else
        {
            printf("\n\n***************    Connection Accepted: %d    ***************\n\n", connCount++); 
        }
            pid = fork();               
            if (pid <0)
                printf("Error on Fork !!");
        
            if (pid == 0)
            {
                // The follwing function handles the incoming connections
                respond(clients[connNum], timeout, pwd);    
                close(listenfd);
                exit(0);
            } 
            
    }

    return 0;
}

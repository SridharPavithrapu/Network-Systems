/***************************************************************************************
* FILENAME: server.c                                                               *
* OWNER: Sridhar Pavithrapu.				                     			       *
* FILE DESCRIPTION: This file consists the function's of udp_client.c                  *
***************************************************************************************/


/* Headers section */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>      
#include <sys/types.h>        
#include <sys/wait.h>         
#include <arpa/inet.h>        
#include <unistd.h>           
#include <dirent.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

/* Macros */
#define KEEP_ALIVE 1
#define BUFFER	(1024)

int    listener, conn;
pid_t  pid;

/* Structure for defining the time-out */
struct timeval tv;

/* Structure for file parsing commands */
typedef struct 
{
	int port;
	int timeOut;
	char * rootPath;
	char * directoryIndex;
	char * fileFormats[7];
	
} MainStruct;

/* Structure for storing the contents received by client */
typedef struct {
    char reqMethod[BUFFER];
    char reqUri[BUFFER];
    char reqVersion[BUFFER]; 
}ReqInfo;

/* Enum for invalid types */
typedef enum {

	InvalidMethod=0,
	InvalidURL,
	InvalidHTTP
	
}BadRequestError;

/* Definition of 'send_notFoundError' function */
void send_notFoundError(int new_socket,ReqInfo * reqinf)
{
	printf("\nIn send_notFoundError function\n");
	char httpText[BUFFER];
	char contentMessage[BUFFER];
	bzero(httpText,sizeof(httpText));
	bzero(contentMessage,sizeof(contentMessage));


	printf("\nSending not found error:\n");
	printf("**************************************\n");

	/* Constructing the HTTP response */
	sprintf(httpText, "%s %s %s",reqinf->reqVersion,"404","Not Found\n\n");
	/* Sending response back to the client */
	write(new_socket, httpText, strlen(httpText));
	printf("httpText: %s\n",httpText);
	
	sprintf(contentMessage, "<html><body><H1>404 Not Found Reason URL does not exist : %s </H1></body></html>",reqinf->reqUri);
	write(new_socket, contentMessage, strlen(contentMessage));
	printf("contentMessage: %s\n",contentMessage);

	printf("**************************************\n\n");
}

/* Definition of 'send_fileInformation' function */
void send_fileInformation(int new_socket,char *fileName, ReqInfo * reqinf,char * contentTypeText)
{
	printf("\nIn send_fileInformation function\n");

	/* Check for file existance */ 
	FILE *fp;
	fp = fopen(fileName,"r");
	if(fp == NULL)
	{
		printf("File does not exist\n");
		send_notFoundError(new_socket,reqinf);
	}
	else
	{
		char httpText[BUFFER];
		char contentType[BUFFER];
		char contentLength[BUFFER];
		bzero(httpText,sizeof(httpText));
		bzero(contentType,sizeof(contentType));
		bzero(contentLength,sizeof(contentLength));

		printf("\nSending requested file content:\n");
		printf("**************************************\n");

		/* Sending header information to client */
		sprintf(httpText, "%s %s %s",reqinf->reqVersion,"200","Document Follows\n");
		printf("httpText: %s\n",httpText);
		write(new_socket, httpText, strlen(httpText));
		
		char extension[BUFFER];
		bzero(extension, sizeof(extension));
		char * ext = strrchr(contentTypeText, ' ');
		if (!ext) 
		{
			printf("No extension\n");
		} 
		else 
		{
			printf("extension is %s\n", ext);
			strcpy(extension,ext);
		}

		sprintf(contentType, "%s %s %s","Content-Type:",extension,"\n");
		printf("contentType: %s\n",contentType);
		write(new_socket, contentType, strlen(contentType));

		/* Sending file content */
		ssize_t fileSize,size_check=0;
		fseek(fp, 0, SEEK_END);
   		fileSize = ftell(fp);
		rewind(fp);

		sprintf(contentLength, "%s %ld %s","Content-length:",fileSize,"\n\n");
		printf("contentLength: %s\n",contentLength);
		write(new_socket, contentLength, strlen(contentLength));		
		
		char buffer[BUFFER];
		while(size_check < fileSize)
		{
				bzero(buffer,sizeof(buffer));
				int bytes_read = fread(buffer,sizeof(char),BUFFER,fp);
				write(new_socket, buffer, bytes_read);
				size_check += bytes_read;
		}

		printf("**************************************\n\n");
	}
}

/* Definition of 'send_badRequestError' function */
void send_badRequestError(int new_socket,BadRequestError errorNumber,ReqInfo * reqinf)
{
	printf("\nIn send_badRequestError function\n");

	char httpText[BUFFER];
	char contentMessage[BUFFER];
	bzero(httpText,sizeof(httpText));
	bzero(contentMessage,sizeof(contentMessage));

	printf("\nSending badrequest error:\n");
	printf("**************************************\n");

	sprintf(httpText, "%s %s %s",reqinf->reqVersion,"400","Bad Request\n\n");
	printf("httpText: %s\n",httpText);
	write(new_socket, httpText, strlen(httpText));

	/* Check for Invalid type, and sending the respective content to client */	
	if(errorNumber == InvalidMethod)
	{
		sprintf(contentMessage, "<html><body><H1>400 Bad Request Reason: Invalid Method : %s </H1></body></html>",reqinf->reqMethod);
		printf("contentMessage: %s\n",contentMessage);
		write(new_socket, contentMessage, strlen(contentMessage));
	}
	else if(errorNumber == InvalidURL)
	{
		sprintf(contentMessage, "<html><body><H1>400 Bad Request Reason: Invalid URL : %s </H1></body></html>",reqinf->reqUri);
		printf("contentMessage: %s\n",contentMessage);
		write(new_socket, contentMessage, strlen(contentMessage));
	}
	else if(errorNumber == InvalidHTTP)
	{
		sprintf(contentMessage, "<html><body><H1>400 Bad Request Reason: Invalid HTTP-Version : %s </H1></body></html>",reqinf->reqVersion);
		printf("contentMessage: %s\n",contentMessage);
		write(new_socket, contentMessage, strlen(contentMessage));
	}
	else
	{
		printf("Invalid Input\n");
	}	

	printf("**************************************\n\n");
}

/* Definition of 'send_notImplementedError' function */
void send_notImplementedError(int new_socket,ReqInfo * reqinf,char * fileExtensionType)
{
	printf("\nIn send_notImplementedError function\n\n");
	
	char httpText[BUFFER];
	char contentMessage[BUFFER];
	bzero(httpText,sizeof(httpText));
	bzero(contentMessage,sizeof(contentMessage));

	printf("Sending not implementation header:\n");
	printf("**************************************\n");

	/* Sending not implemented error to client */
	sprintf(httpText, "%s %s %s",reqinf->reqVersion,"501","Not Implemented\n\n");
	printf("httpText: %s\n",httpText);
	write(new_socket, httpText, strlen(httpText));
	
	
	sprintf(contentMessage, "<html><body><H1>501 Not Implemented, %s : %s </H1></body></html>",fileExtensionType,reqinf->reqUri);
	printf("contentMessage: %s\n",contentMessage);
	write(new_socket, contentMessage, strlen(contentMessage));

	printf("**************************************\n\n");		
}

/* Definition of 'send_postFileInformation' function */
void send_postFileInformation(int new_socket,char *fileName, ReqInfo * reqinf,char * contentTypeText,char * postData)
{
	printf("\nIn send_postFileInformation function\n");

	/* Check for file existance */ 
	FILE *fp;
	fp = fopen(fileName,"r");
	if(fp == NULL)
	{
		printf("File does not exist\n");
		send_notFoundError(new_socket,reqinf);
	}
	else
	{
		char httpText[BUFFER];
		char contentType[BUFFER];
		char contentLength[BUFFER];
		bzero(httpText,sizeof(httpText));
		bzero(contentType,sizeof(contentType));
		bzero(contentLength,sizeof(contentLength));

		printf("\nSending post request content:\n");
		printf("**************************************\n");

		sprintf(httpText, "%s %s %s",reqinf->reqVersion,"200","Document Follows\n");
		printf("httpText: %s\n",httpText);
		write(new_socket, httpText, strlen(httpText));
		
		char extension[BUFFER];
		bzero(extension, sizeof(extension));
		char * ext = strrchr(contentTypeText, ' ');
		if (!ext) 
		{
			printf("No extension\n");
		} 
		else 
		{
			printf("extension is %s\n", ext);
			strcpy(extension,ext);
		}
		sprintf(contentType, "%s %s %s","Content-Type:",extension,"\n");
		printf("contentType: %s\n",contentType);
		write(new_socket, contentType, strlen(contentType));

		/* Sending file content */
		ssize_t fileSize,size_check=0;
		fseek(fp, 0, SEEK_END);
   		fileSize = ftell(fp);
		rewind(fp);

		
		
		/*Sending post Data */	
		char *tempData;
		int count;
		char received_string[BUFFER];
		tempData = strtok (postData, "\n");
		while(tempData != NULL)
		{	
			if((strlen(tempData) == 1))
			{
				count = 1;
			}
			tempData = strtok (NULL, "\n");
			if(count == 1)
			{
				bzero(received_string,sizeof(received_string));
				sprintf(received_string,"%s",tempData);
				
				count = 0;
			}		
		}
		int received_stringsize = 0;
		received_stringsize = strlen(received_string);
		sprintf(contentLength, "%s %ld %s","Content-length:",fileSize+received_stringsize,"\n\n");
		printf("contentLength: %s\n",contentLength);
		write(new_socket, contentLength, strlen(contentLength));

		char contentMessage[BUFFER];
		bzero(contentMessage,sizeof(contentMessage));
		sprintf(contentMessage, "<html><body><pre><H1>%s ",received_string);
		printf("contentMessage: %s\n",contentMessage);
		write(new_socket, contentMessage, strlen(contentMessage));
		printf("Sending file content\n");
		char buffer[BUFFER];
		while(size_check < fileSize)
		{
				bzero(buffer,sizeof(buffer));
				int bytes_read = fread(buffer,sizeof(char),BUFFER,fp);
				write(new_socket, buffer, bytes_read);
				size_check += bytes_read;
		}
		write(new_socket,"</H1></pre></body></html>",strlen("</H1></pre></body></html>"));

		printf("**************************************\n\n");
	}
}

/* Definition of 'service_request' function */
int service_request(int conn,MainStruct * inf)
{
	printf("\nIn service_request function\n");	

	ReqInfo  reqinfo={0};
	char buffer[BUFFER] = {0};
	char postBuffer[BUFFER] = {0};
	char pipelineBuffer[BUFFER] = {0};
	bzero(buffer,sizeof(buffer));
	bzero(pipelineBuffer,sizeof(pipelineBuffer));
	bzero(postBuffer,sizeof(postBuffer));
	char *requestLine;
	int count,index=0;
	
	/* Receiving HTTP request */
	int status = recv(conn, buffer, BUFFER, 0);
	/* receive error */
	if (status < 0)   
	{ 
		fprintf(stderr,("recv() error\n"));
	}	
	/* receive socket closed */
	else if (status == 0)    
	{	
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	}
	/* message received */	
	else    
	{
		/* Received request from client */
		printf("\nReceived request content:\n");
		printf("**************************************\n");
		printf("%s",buffer);
		printf("**************************************\n\n");
		strncpy(postBuffer,buffer,strlen(buffer));
		strncpy(pipelineBuffer,buffer,strlen(buffer));
		requestLine = strtok (buffer, "\n");
		sscanf(requestLine, "%s %s %s", reqinfo.reqMethod,  reqinfo.reqUri,  reqinfo.reqVersion);
		//printf("reqinfo.reqMethod:%s, reqinfo.reqUri:%s,reqinfo.reqVersion:%s\n", reqinfo.reqMethod,reqinfo.reqUri,reqinfo.reqVersion);
		
		/* Check for request method */
		if(strcmp(reqinfo.reqMethod,"GET")==0)
		{
			printf("Inside GET method\n");
			if(strcmp(reqinfo.reqVersion,"HTTP/1.0")==0 || strcmp(reqinfo.reqVersion,"HTTP/1.1")==0)
			{
				/* File Name extraction */
				char * fileName = strrchr(reqinfo.reqUri, 0X2F);
				printf("fileName:%s\n",fileName+1);
			
				/* Check for file name empty */
				if(strcmp(fileName,"/\0") == 0)
				{	
					sprintf(fileName,"/%s",inf->directoryIndex+1);
					
				}
				printf("fileName:%s\n",fileName);

				/* Check for extension support */
				char contentTypeText[BUFFER];
				bool fileExist = 0;
				
				bzero(contentTypeText,sizeof(contentTypeText));
				char * ext = strrchr(fileName, '.');
				/* If received file extension is not valid */
				if (!ext) 
				{
					printf("No extension\n");
					char httpText[BUFFER];
					bzero(httpText,sizeof(httpText));
					printf("\nSending error message:\n");
					printf("**************************************\n");

					sprintf(httpText, "%s 500 Internal Server Error: cannot allocate memory\n\n",reqinfo.reqVersion);
					printf("httpText: %s\n",httpText);
					write(conn, httpText, strlen(httpText));

					char contentMessage[BUFFER];
					bzero(contentMessage,sizeof(contentMessage));
					sprintf(contentMessage, "<html><body><H1>500 Internal Server Error: cannot allocate memory</H1></body></html>");
					printf("ContentMessage: %s\n",contentMessage);
					write(conn, contentMessage, strlen(contentMessage));
					printf("**************************************\n\n");
				} 
				else 
				{
					printf("Received file extension is: %s\n", ext);
					
					char *ret;
					for(int temp =0;temp<8;temp++)
					{
						ret = strstr(inf->fileFormats[temp],ext);
						if(ret !=NULL)
						{
							strcpy(contentTypeText,inf->fileFormats[temp]);
						}
					}
					printf("ContentTypeText is %s\n", contentTypeText);
					if( (strstr(inf->fileFormats[0],ext)!=NULL) || (strstr(inf->fileFormats[1],ext)!=NULL) || (strstr(inf->fileFormats[2],ext)!=NULL) || (strstr(inf->fileFormats[3],ext)!=NULL)
						|| (strstr(inf->fileFormats[4],ext)!=NULL) || (strstr(inf->fileFormats[5],ext)!=NULL) ||(strstr(inf->fileFormats[6],ext)!=NULL) ||(strstr(inf->fileFormats[7],ext)!=NULL) )
					{
						/* Creating path for the file */
						char filePath[BUFFER];
						bzero(filePath,sizeof(filePath));
						char rootPathTemp[BUFFER];
						bzero(rootPathTemp,sizeof(rootPathTemp));
						if(sscanf(inf->rootPath,"%*[^\"]\"%[^\"]\"",rootPathTemp) == 1)
						{
							printf("Rootpath:%s\n",rootPathTemp);
							sprintf(filePath,"%s%s",rootPathTemp,fileName);
							printf("FilePath:%s\n",filePath);
							send_fileInformation(conn,filePath,&reqinfo,contentTypeText);
						}
						else
						{
							printf("Error in rootpath\n");
						}
				
					}
					else
					{
						/* Sending not implemented error to client */
						send_notImplementedError(conn,&reqinfo,ext);
					}
				}
	
			}
			else
			{
				printf("Inside invalid http method\n");
				send_badRequestError(conn,InvalidHTTP,&reqinfo);
			}
       			
		}
		/* If input request method is POST */
		else if(strcmp(reqinfo.reqMethod,"POST")==0)
		{
			printf("Inside POST method\n");
				
			if(strcmp(reqinfo.reqVersion,"HTTP/1.0")==0 || strcmp(reqinfo.reqVersion,"HTTP/1.1")==0)
			{
				/* File Name extraction */
				char * fileName = strrchr(reqinfo.reqUri, 0X2F);
			
				/* Check for file name empty */
				if(strcmp(fileName,"/\0") == 0)
				{	
					sprintf(fileName,"/%s",inf->directoryIndex+1);
					
				}
				printf("FileName received is:%s\n",fileName);

				/* Check for extension support */
				
				char contentTypeText[BUFFER];
				bool fileExist = 0;
				
				bzero(contentTypeText,sizeof(contentTypeText));
				char * ext = strrchr(fileName, '.');
				if (!ext) 
				{
					printf("No extension\n");
					char httpText[BUFFER];
					bzero(httpText,sizeof(httpText));
					printf("\nSending error message:");
					printf("\n**************************************\n");
	
					sprintf(httpText, "%s 500 Internal Server Error: cannot allocate memory\n\n",reqinfo.reqVersion);
					printf("httpText: %s\n",httpText);
					write(conn, httpText, strlen(httpText));

					char contentMessage[BUFFER];
					bzero(contentMessage,sizeof(contentMessage));
					sprintf(contentMessage, "<html><body><H1>500 Internal Server Error: cannot allocate memory</H1></body></html>");
					printf("contentMessage: %s\n",contentMessage);
					write(conn, contentMessage, strlen(contentMessage));
					printf("**************************************\n\n");
				} 
				else 
				{
					
					char *ret;
					for(int temp =0;temp<8;temp++)
					{
						ret = strstr(inf->fileFormats[temp],ext);
						if(ret !=NULL)
						{
							strcpy(contentTypeText,inf->fileFormats[temp]);
						}
					}

					printf("ContentTypeText is %s\n", contentTypeText);
					if( (strstr(inf->fileFormats[0],ext)!=NULL) || (strstr(inf->fileFormats[1],ext)!=NULL) || (strstr(inf->fileFormats[2],ext)!=NULL) || (strstr(inf->fileFormats[3],ext)!=NULL)
						|| (strstr(inf->fileFormats[4],ext)!=NULL) || (strstr(inf->fileFormats[5],ext)!=NULL) ||(strstr(inf->fileFormats[6],ext)!=NULL) ||(strstr(inf->fileFormats[7],ext)!=NULL) )
					{
						/* Creating path for the file */
						char filePath[BUFFER];
						bzero(filePath,sizeof(filePath));
						char rootPathTemp[BUFFER];
						bzero(rootPathTemp,sizeof(rootPathTemp));
						if(sscanf(inf->rootPath,"%*[^\"]\"%[^\"]\"",rootPathTemp) == 1)
						{
							printf("Rootpath:%s\n",rootPathTemp);
							sprintf(filePath,"%s%s",rootPathTemp,fileName);
							printf("FilePath:%s\n",filePath);
							send_postFileInformation(conn,filePath,&reqinfo,contentTypeText,postBuffer);
						}
						else
						{
							printf("Error in rootpath\n");
						}
				
					}
					else
					{
						/* Sending not implemented error to client */
						send_notImplementedError(conn,&reqinfo,ext);
					}
				}
	
			}
			else
			{
				printf("Inside invalid http method\n");
				send_badRequestError(conn,InvalidHTTP,&reqinfo);
			}
		}
		else
		{
			printf("Inside invalid request method\n");
			send_badRequestError(conn,InvalidMethod,&reqinfo);
		}

	}

	/* Check for connection alive */
	if( KEEP_ALIVE == 1){	
		char *temppipelineData;
		char pipelineSss[BUFFER];
		temppipelineData = strtok (pipelineBuffer, "\n");
		while(temppipelineData != NULL)
		{	
			if(strstr(temppipelineData, "Connection") != NULL)
			{
				bzero(pipelineSss,sizeof(pipelineSss));
				sprintf(pipelineSss,"%s",temppipelineData+12);
			}
			temppipelineData = strtok (NULL, "\n");
			
		}
		printf("Keep-Alive check message:%s\n",pipelineSss);
		if(strstr(pipelineSss,"keep-alive") != NULL)
		{
			printf("Inside time-out check condition with time_out:%d\n",inf->timeOut);
			fd_set fd;
	
			FD_ZERO(&fd);
			FD_SET(conn, &fd);
			int n = conn+1;
			tv.tv_sec = inf->timeOut;
			tv.tv_usec = 0;
			int rv=0; 
			printf("\n*****Timer Started*****\n");
			rv = select(n,&fd,NULL, NULL, &tv); 
			if(rv == -1)
			{
				perror("Select error\n");
				close(conn);
				return(0);
			}
			else if(rv == 0)			
			{
				printf("\n*****Not received request within timeout****\n");        
				close(conn);
				return(0);
			}
			else
			{          

				printf("\n*****Received request within timeout*****\n");
				if(FD_ISSET(conn,&fd))
				{
					service_request(conn,inf);
				}		
			}
		}	
	}
	return(0);
}

/* Definition of 'parse_config' function */
int parse_config(MainStruct * inf) 
{
	/* Used variables */
	FILE *file;
	char * line = NULL;
	char *addr;
	size_t len = 0;
	ssize_t read;

	/* Open file pointer */
	file = fopen("ws.conf", "r");
	if(file != NULL)
	{
     	/* Line-by-line read cache file */
     	while ((read = getline(&line, &len, file)) != -1) 
     	{
          	if(strstr(line, "Listen") != NULL) 
          	{
               	/* Get port */
               	inf->port = atoi(line+6);
				
            	}
	    		else if(strstr(line, "DocumentRoot") != NULL) 
            	{
               	/* Get DocumentRoot */
               	inf->rootPath = strdup(line+12);
				inf->rootPath[strlen(inf->rootPath)-1] = 0X00;
            	}
            	else if(strstr(line, "DirectoryIndex") != NULL) 
            	{
               	/* Get DirectoryIndex */
               	inf->directoryIndex = strdup(line+14);
				inf->directoryIndex[strlen(inf->directoryIndex)-1] = 0X00;
				            	
			}
			else if(strstr(line, ".html") != NULL)
			{
				inf->fileFormats[0] = strdup(line);
				inf->fileFormats[0][strlen(inf->fileFormats[0])-1] = 0X00;
			}
			else if(strstr(line, ".txt") != NULL)
			{
				inf->fileFormats[1] = strdup(line);
				inf->fileFormats[1][strlen(inf->fileFormats[1])-1] = 0X00;
			}
			else if(strstr(line, ".png") != NULL)
			{
				inf->fileFormats[2] = strdup(line);
				inf->fileFormats[2][strlen(inf->fileFormats[2])-1] = 0X00;
			}
			else if(strstr(line, ".gif") != NULL)
			{
				inf->fileFormats[3] = strdup(line);
				inf->fileFormats[3][strlen(inf->fileFormats[3])-1] = 0X00;
			}
			else if(strstr(line, ".jpg") != NULL)
			{
				inf->fileFormats[4] = strdup(line);
				inf->fileFormats[4][strlen(inf->fileFormats[4])-1] = 0X00;
			}
			else if(strstr(line, ".css") != NULL)
			{
				inf->fileFormats[5] = strdup(line);
				inf->fileFormats[5][strlen(inf->fileFormats[5])-1] = 0X00;
			}
			else if(strstr(line, ".js") != NULL)
			{
				inf->fileFormats[6] = strdup(line);
				inf->fileFormats[6][strlen(inf->fileFormats[6])-1] = 0X00;
			}
			else if(strstr(line, ".ico") != NULL)
			{
				inf->fileFormats[7] = strdup(line);
				inf->fileFormats[7][strlen(inf->fileFormats[7])-1] = 0X00;
				printf("inf->fileFormats[7]:%s\n",inf->fileFormats[7]);
			}
			else if(strstr(line, "Keep-Alive") != NULL)
			{
				/* Get timeout */
				inf->timeOut = atoi(line+10);
			}
			
        	}
        	/* Close file */
        	fclose(file);
	} else return 0;

	return 1;
}

/* Definition of 'signal_callback_handler' function */
void signal_callback_handler(int signum)
{
   printf("Caught signal %d\n",signum);
   // Cleanup and close up stuff here

   // Terminate program
   exit(signum);
}

/* Definition of 'main' function */
int main() 
{

	/* Used variables */
	MainStruct val;
	int status;
	
	struct sockaddr_in servaddr;

	/* Register signal and signal handler */
   	signal(SIGINT, signal_callback_handler);

	/* Parse config */
	status = parse_config(&val);

	/* Check status */
	if (status) printf("File parsed succssesfuly\n");
	else printf("Error while parsing file\n\n");

	printf("\nws_conf file information:\n");
	printf("**************************************\n");

	/* Print parsed values */
	printf("Port: %d\n", val.port);
	printf("DocumentRoot: %s\n", val.rootPath);
	printf("DirectoryIndex: %s\n", val.directoryIndex);
	for(int index =0;index<8;index++)
	{
		printf("fileformats[%d]: %s\n",index, val.fileFormats[index]);
	}
	printf("timeOut: %d\n", val.timeOut);
	printf("**************************************\n");

	/*  Create socket  */
	if ( (listener = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		printf("Couldn't create listening socket.\n");
	}
	
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(val.port);

	
	
	if ( bind(listener, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 )
	{	
		printf("Couldn't bind listening socket.\n");
	}

	

	if ( listen(listener, 1024) < 0 )
	{	
		printf("Call to listen failed.\n");
	}

	/*  Loop infinitely to accept and service connections  */

	while ( 1 ) 
	{
		printf("\n**************************************\n");
		printf("Waiting for a new connection\n");
		printf("**************************************\n");

		if ( (conn = accept(listener, NULL, NULL)) < 0 )
		{
		    printf("Error calling accept():%d\n",conn);
		}
		
		
		pid = fork();
		if(pid < 0)
		{
			perror("Fork Failed\n");
			exit(0);
		}
	
		if ( pid == 0 ) 
		{

			/*  This is now the forked child process, so
			close listening socket and service request   */

			if ( close(listener) < 0 )
				printf("Error closing listening socket in child.\n");

			printf("Connection Established successfully\n");
			exit(service_request(conn,&val));			
			  			
		}
		/* Close the connection */
		if(close(conn)<0)
		{
			printf("Error closing socket\n");
		}
		
	}   
	return 0; 

}

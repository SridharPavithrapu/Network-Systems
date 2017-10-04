#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <sys/wait.h>         /*  for waitpid()             */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */
#include <dirent.h>
#include <stdbool.h>

#define BUFFER	(1024)


typedef struct 
{
	int port;
	char * rootPath;
	char * directoryIndex;
	char * fileFormats[7];
} MainStruct;

typedef struct {
    char reqMethod[BUFFER];
    char reqUri[BUFFER];
    char reqVersion[BUFFER]; 
}ReqInfo;

typedef enum {

	InvalidMethod=0,
	InvalidURL,
	InvalidHTTP
	
}BadRequestError;


void send_notFoundError(int new_socket,ReqInfo * reqinf)
{
	char httpText[BUFFER];
	char contentMessage[BUFFER];
	bzero(httpText,sizeof(httpText));
	bzero(contentMessage,sizeof(contentMessage));

	sprintf(httpText, "%s %s %s",reqinf->reqVersion,"404","Not Found\n");
	printf("httpText: %s",httpText);
	write(new_socket, httpText, strlen(httpText));
	
	
	sprintf(contentMessage, "<html><body>404 Not Found Reason URL does not exist : %s </body></html>\n",reqinf->reqUri);
	printf("contentMessage: %s",contentMessage);
	write(new_socket, contentMessage, strlen(contentMessage));
}

void send_fileInformation(int new_socket,char *fileName, ReqInfo * reqinf,char * contentTypeText)
{
	/* Check for file existance */ 
	FILE *fp;
	fp = fopen(fileName,"r");
	if(fp == NULL)
	{
		printf("File does not exist\n");
	}
	else
	{
		char httpText[BUFFER];
		char contentType[BUFFER];
		char contentLength[BUFFER];
		bzero(httpText,sizeof(httpText));
		bzero(contentType,sizeof(contentType));
		bzero(contentLength,sizeof(contentLength));

		sprintf(httpText, "%s %s %s",reqinf->reqVersion,"200","Document Follows\n");
		printf("httpText: %s",httpText);
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
		printf("contentType: %s",contentType);
		write(new_socket, contentType, strlen(contentType));

		/* Sending file content */
		ssize_t fileSize,size_check=0;
		fseek(fp, 0, SEEK_END);
   		fileSize = ftell(fp);
		rewind(fp);

		sprintf(contentLength, "%s %ld %s","Content-length:",fileSize,"\n\n");
		printf("contentLength: %s",contentLength);
		write(new_socket, contentLength, strlen(contentLength));		
		
		char buffer[BUFFER];
		while(size_check < fileSize)
		{
				bzero(buffer,sizeof(buffer));
				int bytes_read = fread(buffer,sizeof(char),BUFFER,fp);
				write(new_socket, buffer, bytes_read);
				size_check += bytes_read;
		}
	}
}

void send_badRequestError(int new_socket,BadRequestError errorNumber,ReqInfo * reqinf)
{
	char httpText[BUFFER];
	char contentMessage[BUFFER];
	bzero(httpText,sizeof(httpText));
	bzero(contentMessage,sizeof(contentMessage));

	sprintf(httpText, "%s %s %s",reqinf->reqVersion,"400","Bad Request\n\n");
	printf("httpText: %s",httpText);
	write(new_socket, httpText, strlen(httpText));
	
	if(errorNumber == InvalidMethod)
	{
		sprintf(contentMessage, "<html><body><H1>400 Bad Request Reason: Invalid Method : %s </H1></body></html>",reqinf->reqMethod);
		printf("contentMessage: %s",contentMessage);
		write(new_socket, contentMessage, strlen(contentMessage));
	}
	else if(errorNumber == InvalidURL)
	{
		sprintf(contentMessage, "<html><body><H1>400 Bad Request Reason: Invalid URL : %s </H1></body></html>\n",reqinf->reqUri);
		printf("contentMessage: %s",contentMessage);
		write(new_socket, contentMessage, strlen(contentMessage));
	}
	else if(errorNumber == InvalidHTTP)
	{
		sprintf(contentMessage, "<html><body><H1>400 Bad Request Reason: Invalid HTTP-Version : %s </H1></body></html>\n",reqinf->reqVersion);
		printf("contentMessage: %s",contentMessage);
		write(new_socket, contentMessage, strlen(contentMessage));
	}
	else
	{
		printf("Invalid Input\n");
	}	
}

void send_notImplementedError(int new_socket,ReqInfo * reqinf,char * fileExtensionType)
{
	char httpText[BUFFER];
	char contentMessage[BUFFER];
	bzero(httpText,sizeof(httpText));
	bzero(contentMessage,sizeof(contentMessage));

	sprintf(httpText, "%s %s %s",reqinf->reqVersion,"501","Not Implemented\n");
	printf("httpText: %s",httpText);
	write(new_socket, httpText, strlen(httpText));
	
	
	sprintf(contentMessage, "<html><body>501 Not Implemented, %s : %s </body></html>\n",fileExtensionType,reqinf->reqUri);
	printf("contentMessage: %s",contentMessage);
	write(new_socket, contentMessage, strlen(contentMessage));
		
}


int service_request(int conn,MainStruct * inf)
{
	ReqInfo  reqinfo={0};
	char buffer[BUFFER] = {0};
	bzero(buffer,sizeof(buffer));
	char *requestLine;
	int count,index=0;
	
	printf("Inside service_request method\n");
	/* Receiving HTTP request */
	int status = recv(conn, buffer, BUFFER, 0);
	if (status < 0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (status == 0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
		printf("buffer:%s\n", buffer);
		requestLine = strtok (buffer, "\r\n");
		sscanf(requestLine, "%s %s %s", reqinfo.reqMethod,  reqinfo.reqUri,  reqinfo.reqVersion);
		printf("reqinfo.reqMethod:%s, reqinfo.reqUri:%s,reqinfo.reqVersion:%s\n", reqinfo.reqMethod,reqinfo.reqUri,reqinfo.reqVersion);
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
					sprintf(fileName,"/%s",inf->directoryIndex);
				}
				printf("fileName:%s\n",fileName);

				/* Check for extension support */
				char extension[BUFFER];
				char contentTypeText[BUFFER];
				bool fileExist = 0;
				bzero(extension,sizeof(extension));
				bzero(contentTypeText,sizeof(contentTypeText));
				char * ext = strrchr(reqinfo.reqUri, '.');
				if (!ext) 
				{
					printf("No extension\n");
					char httpText[BUFFER];
					bzero(httpText,sizeof(httpText));

					sprintf(httpText, "%s 500 Internal Server Error: cannot allocate memory\n\n",reqinfo.reqVersion);
					printf("httpText: %s",httpText);
					write(conn, httpText, strlen(httpText));
				} 
				else 
				{
					printf("extension is %s\n", ext);
					strcpy(extension,ext);

					for(int temp =0;temp<8;temp++)
					{
						if(strstr(inf->fileFormats[temp],ext)!=NULL)
						{
							strcpy(contentTypeText,inf->fileFormats[temp]);
						}
					}
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
							printf("rootpath:%s\n",rootPathTemp);
							sprintf(filePath,"%s%s",rootPathTemp,fileName);
							printf("filePath:%s\n",filePath);
							send_fileInformation(conn,filePath,&reqinfo,contentTypeText);
						}
						else
						{
							printf("Error in rootpath\n");
						}
				
					}
					else
					{
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
}

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
            	}
            	else if(strstr(line, "DirectoryIndex") != NULL) 
            	{
               	/* Get DirectoryIndex */
               	inf->directoryIndex = strdup(line+14);
            	}
			else if(strstr(line, ".html") != NULL)
			{
				inf->fileFormats[0] = strdup(line);
			}
			else if(strstr(line, ".txt") != NULL)
			{
				inf->fileFormats[1] = strdup(line);
			}
			else if(strstr(line, ".png") != NULL)
			{
				inf->fileFormats[2] = strdup(line);
			}
			else if(strstr(line, ".gif") != NULL)
			{
				inf->fileFormats[3] = strdup(line);
			}
			else if(strstr(line, ".jpg") != NULL)
			{
				inf->fileFormats[4] = strdup(line);
			}
			else if(strstr(line, ".css") != NULL)
			{
				inf->fileFormats[5] = strdup(line);
			}
			else if(strstr(line, ".js") != NULL)
			{
				inf->fileFormats[6] = strdup(line);
			}
			else if(strstr(line, ".ico") != NULL)
			{
				inf->fileFormats[7] = strdup(line);
			}
        	}
        	/* Close file */
        	fclose(file);
	} else return 0;

	return 1;
}

int main() 
{
	/* Used variables */
	MainStruct val;
	int status;
	int    listener, conn;
	pid_t  pid;
	struct sockaddr_in servaddr;

	/* Parse congig */
	status = parse_config(&val);

	/* Check status */
	if (status) printf("File parsed succssesfuly\n");
	else printf("Error while parsing file\n");

	/* Print parsed values */
	printf("Port: %d\n", val.port);
	printf("DocumentRoot: %s\n", val.rootPath);
	printf("DirectoryIndex: %s\n", val.directoryIndex);
	for(int index =0;index<8;index++)
	{
		printf("fileformats[%d]: %s\n",index, val.fileFormats[index]);
	}
	

	/*  Create socket  */
	if ( (listener = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		printf("Couldn't create listening socket.");

	
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(val.port);

	
	
	if ( bind(listener, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 )
		printf("Couldn't bind listening socket.");
	

	

	if ( listen(listener, 1024) < 0 )
		printf("Call to listen failed.");
	

	/*  Loop infinitely to accept and service connections  */

	while ( 1 ) 
	{

		if ( (conn = accept(listener, NULL, NULL)) < 0 )
		    printf("Error calling accept():%d\n",conn);

		pid = fork();
		if ( pid == 0 ) 
		{

			/*  This is now the forked child process, so
			close listening socket and service request   */
			int bufsize = BUFFER;    
   			char *buffer = malloc(bufsize);

			if ( close(listener) < 0 )
				printf("Error closing listening socket in child.");

			printf("Connection Established successfully\n");
			service_request(conn,&val);			
			    
			if(close(conn)<0)
			{
				printf("Error closing socket\n");
			}
			_exit(0);
		}
		else 
		{		
			if(close(conn)<0)
			{
				printf("Error closing socket\n");
			}
		}
		waitpid(-1, NULL, WNOHANG);
	}   
	return 0; 

}

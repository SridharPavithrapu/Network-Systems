#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <sys/wait.h>         /*  for waitpid()             */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */

typedef struct 
{
	int port;
	char * rootPath;
	char * directoryIndex;
} MainStruct;

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
               	/* Get port */
               	inf->rootPath = strdup(line+12);
            	}
            	else if(strstr(line, "DirectoryIndex") != NULL) 
            	{
               	/* Get port */
               	inf->directoryIndex = strdup(line+14);
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

	//while ( 1 ) {

		if ( (conn = accept(listener, NULL, NULL)) < 0 )
		    printf("Error calling accept()");

		
		if ( (pid = fork()) == 0 ) {

			/*  This is now the forked child process, so
			close listening socket and service request   */

			if ( close(listener) < 0 )
				printf("Error closing listening socket in child.");
			printf("Connection Established successfully");
		}
	//}

}

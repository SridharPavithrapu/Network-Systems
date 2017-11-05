/***************************************************************************************
* FILENAME: server.c                                                                   *
* OWNER: Sridhar Pavithrapu.				                     			       *
* FILE DESCRIPTION: This file consists the function's of dfs1.c                  *
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
#include <sys/stat.h>
#include <sys/types.h>


#define BUFFER	(1024)
#define NUM_CLIENTS 100

char rootPath[BUFFER];
int portNumber;
int listener, clients[NUM_CLIENTS];

typedef struct 
{
	char recv_command[BUFFER];
	char recv_user_name[BUFFER];
	char recv_user_password[BUFFER];
	
} CommandStruct;

typedef struct 
{
	char user_name[BUFFER];
	char user_password[BUFFER]; 
	
} UserStruct;

UserStruct users[5];

/* Structure for file parsing commands */
typedef struct 
{
	char message[BUFFER];
	int message_length; 
	
} MessageStruct;


/* Definition of 'parse_config' function */
int parse_config(char * server_config_file) 
{
	FILE *fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int temp_count = 0;

	printf("In parse_config\n");
	if ((fp = fopen(server_config_file,"r")) == NULL)					
	{
		printf("Client configuration file not found.\n So, exiting\n");
		return 0;
	}
	else
	{
		for(int index =0; index<5; index++)
		{
			memset(&users[index], 0 , sizeof(users[index]));
		}
		/* Line-by-line read config file */
		while ((read = getline(&line, &len, fp)) != -1) 
		{
			char *string_token = strtok(line," ");
			strncpy(users[temp_count].user_name,string_token,strlen(string_token));
				
			string_token = strtok(NULL,"\0");
			strncpy(users[temp_count].user_password,string_token,strlen(string_token));
			temp_count++;
		}
	}	
}

/* Definition of 'server_put_file' function */
int server_put_file(char *file_name,int connection,char *user_directory_name)
{
	printf("In server_put_file function with file_name:%s\n",file_name);
	FILE *fp;

	char file_exist_buffer[BUFFER];
	
	MessageStruct *temp = malloc(sizeof(MessageStruct));
	int status = recv(clients[connection], temp, (sizeof(*temp)), 0);
	strncpy(file_exist_buffer,temp->message,temp->message_length);
	free(temp);

	/* receive error */
	if (status < 0)   
	{ 
		fprintf(stderr,("recv() error\n"));
		return(0);
	}	
	/* receive socket closed */
	else if (status == 0)    
	{	
		fprintf(stderr,"Client disconnected upexpectedly.\n");
		return(0);	
	}
	/* message received */	
	else    
	{
		printf("Received message from client:%s\n",file_exist_buffer);
		if(strstr(file_exist_buffer,"File exists") != NULL)
		{
			char chunk_buffer[BUFFER];
			char chunk_size[BUFFER];
			char chunk_number[BUFFER];
			/* Receive data of the file from client */
			for(int loop=0; loop<2; loop++)
			{
				/* First receive dataof chunk size and chunk number */
				bzero(chunk_buffer,sizeof(chunk_buffer));
				bzero(chunk_size,sizeof(chunk_size));
				bzero(chunk_number,sizeof(chunk_number));
				
				MessageStruct *temp = malloc(sizeof(MessageStruct));
				recv(clients[connection], temp, (sizeof(*temp)), 0);
				strncpy(chunk_buffer,temp->message,temp->message_length);
				free(temp);
				
				sscanf(chunk_buffer,"%s %s",chunk_number,chunk_size);
				int int_chunk_size = atoi(chunk_size);
				int int_chunk_number = atoi(chunk_number);
				printf("Received int_chunk_size:%d,int_chunk_number:%d\n",int_chunk_size,int_chunk_number);

				/* creating file for storing */
				char file_name_buffer[BUFFER];
				bzero(file_name_buffer,sizeof(file_name_buffer));
				sprintf(file_name_buffer,"%s/.%s.%d",user_directory_name,file_name,int_chunk_number);
				printf("Part file name:%s\n",file_name_buffer);

				/* Opening part file for writing */		
				fp = fopen(file_name_buffer,"w");
				if(fp != NULL)
				{
					printf("Part file opened\n");
					/* Receiving the contents of the file */				
					int temp_chunk_size = 0;
					char actual_file_buffer[BUFFER];
					while(temp_chunk_size < int_chunk_size )
					{
						int bytes_received = 0;
						bzero(file_name_buffer,sizeof(file_name_buffer));

						MessageStruct *temp = malloc(sizeof(MessageStruct));
						bytes_received = recv(clients[connection], temp, (sizeof(*temp)), 0);
						strncpy(file_name_buffer,temp->message,temp->message_length);	
		
						fwrite(file_name_buffer, 1,temp->message_length,fp);
						temp_chunk_size += temp->message_length;
						free(temp);
					}
					printf("Done writing to file\n");
					fclose(fp);
				}	
				else
				{
					printf("Part file not opened\n");
				}				
			}
		}
		else
		{
			return(0);
		}
	}
}

int service_request(int connection)
{
	printf("\nIn service_request function\n");
	
	char user_directory_name[BUFFER];
	/* Waiting for command to receive */
	CommandStruct *temp = malloc(sizeof(CommandStruct));
	int status = recv(clients[connection], temp, sizeof(*temp), 0);
	/* receive error */
	if (status < 0)   
	{ 
		fprintf(stderr,("recv() error\n"));
		return(0);
	}	
	/* receive socket closed */
	else if (status == 0)    
	{	
		fprintf(stderr,"Client disconnected upexpectedly.\n");
		return(0);	
	}
	/* message received */	
	else    
	{
		printf("buffer:%s\n",temp->recv_command);
		printf("username:%s\n",temp->recv_user_name);
		printf("password:%s\n",temp->recv_user_password);
		char validationBuffer[BUFFER];
		if((strstr(temp->recv_user_name,users[0].user_name)!=NULL && strstr(temp->recv_user_password,users[0].user_password)!=NULL) ||
			(strstr(temp->recv_user_name,users[1].user_name)!=NULL && strstr(temp->recv_user_password,users[1].user_password)!=NULL) ||
			(strstr(temp->recv_user_name,users[2].user_name)!=NULL && strstr(temp->recv_user_password,users[2].user_password)!=NULL) ||
			(strstr(temp->recv_user_name,users[3].user_name)!=NULL && strstr(temp->recv_user_password,users[3].user_password)!=NULL) )
		{
			printf("Sending user availablility message to client\n");
			strcpy(validationBuffer,"User is available");

			MessageStruct *temp_command = malloc(sizeof(MessageStruct));
			temp_command->message_length = strlen(validationBuffer);
			strncpy(temp_command->message,validationBuffer,strlen(validationBuffer));
			write(clients[connection], temp_command, (sizeof(*temp_command)));
			free(temp_command);
			
			sprintf(user_directory_name,"%s/%s",rootPath,temp->recv_user_name);
			printf("Directory name:%s\n",user_directory_name);
			int result = mkdir(user_directory_name,0777);
			printf("Make directory result:%d\n",result);
		}
		else
		{
			printf("Sending user unavailablility message to client\n");
			strcpy(validationBuffer,"User is not available");

			MessageStruct *temp = malloc(sizeof(MessageStruct));
			temp->message_length = strlen(validationBuffer);
			strncpy(temp->message,validationBuffer,strlen(validationBuffer));
			write(clients[connection], temp, (sizeof(*temp)));
			free(temp);
			return(0);
		}
		
		/* Parsing the received operation */
		char temp_operation[BUFFER];
		int count = 0;
		char *temp_cmd,*words;
		char *final_words,*final_temp;
		strcpy(temp_operation,temp->recv_command);
		temp_cmd = temp->recv_command;
		words = strtok(temp_cmd," ");
		while(words != NULL)
		{
			if(strlen(words) > 0)
			{			
				count++;
			}		
			words = strtok(NULL," ");
		}
		/* Checking the number ofo words in the input string */
		if(count > 2)
		{
			printf("In error case\n");
			printf("Error in the input string\n");
			return(0);
		}
		else
		{
			printf("In success case\n");
			int final_count = 0;
			if(count == 2)
			{
				char first_word[BUFFER]="",second_word[BUFFER]="";
				final_temp = temp_operation;
				final_words = strtok(final_temp," ");
			
				while(final_words != NULL)
				{
					if(final_count == 0)
					{
						strcpy(first_word,final_words);
					}
					if(final_count == 1)
					{
						strcpy(second_word,final_words);
					}				
					if(strlen(final_words) > 0)
					{			
						final_count++;
					}		
					final_words = strtok(NULL," ");
				}
		
			
				/* Calling required functions depending on the command received */
				if(strcmp(first_word,"GET")==0)
				{
					//client_get_file(second_word);
				}
				else if(strcmp(first_word,"PUT")==0)
				{
					server_put_file(second_word,connection,user_directory_name);
				}
				else
				{
					printf("Error in the input message given by user\n");
				}
			}
			else if(count == 1)
			{

				/* Calling required functions depending on the command received */
				if(strcmp(temp_operation,"LIST")==0)
				{
					//client_list_directory(udp_socket, remote);
				}
				else if(strcmp(temp_operation,"EXIT")==0)
				{
					//client_exit_server(udp_socket, remote);
				}
				else
				{
					printf("Error in the input message given by user\n");
				}
			}
			else
			{
				printf("Error in the input message given by user\n");
			}
			return(0);
		}
		
		
		


	}
}

/* Definition of 'main' function */
int main(int argc, char * argv[]) 
{

	if(argc != 3)
	{
		printf("Number of command line arguments are incorrect\n");
		exit(1);
	}

	
	bzero(rootPath,sizeof(rootPath));
	strcpy(rootPath,argv[1]);
	portNumber = atoi(argv[2]);

	/* Parse config */
	parse_config("dfs.conf");
	
	printf("\nconf file information:\n");
	printf("**************************************\n");
	for(int count =0; count<5; count++)
	{
		if(users[count].user_name != NULL )
		{
			printf("user-%d information:\n",count);
			printf("\tuser-%d name:%s\n",count,users[count].user_name);
			printf("\tuser-%d password:%s\n",count,users[count].user_password);	
		}	
	}
	printf("**************************************\n");
	printf("\nrootPath:%s\n",rootPath);
	printf("portNumber:%d\n",portNumber);
	printf("**************************************\n");

	/* Initisalizing the clients with -1*/
	for (int index = 0; index < NUM_CLIENTS; index++)								
	{
		clients[index] = -1;
	}

	struct sockaddr_in servaddr;

	/*  Create socket  */
	if ( (listener = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		printf("Couldn't create listening socket.\n");
		exit(1);
	}
	int tempval = 1;
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const void *)&tempval , sizeof(int));

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(portNumber);

	if ((bind(listener, (struct sockaddr *)&servaddr, sizeof(servaddr)))	 < 0)		
	{
		perror("Couldn't bind listening socket.\n ");
		exit(1);
	}
	
	if(listen(listener, 1024) != 0)							
	{
		printf("Call to listen failed.\n");
		exit(1);
	}

	struct sockaddr_in clientaddr;
	socklen_t len;
	int slot = 0;

	/*  Loop infinitely to accept and service connections  */

	while ( 1 ) 
	{
		printf("\n**************************************\n");
		printf("Waiting for a new connection\n");
		printf("**************************************\n");
		len = sizeof(clientaddr);
		if ( (clients[slot] = accept(listener, (struct sockaddr *)&clientaddr, &len)) < 0 )
		{
		    printf("Error calling accept():%d\n",clients[slot]);
		}
		else
		{
	
			if ( fork() == 0 ) 
			{

				/*  This is now the forked child process, so
				close listening socket and service request   */

				if ( close(listener) < 0 )
					printf("Error closing listening socket in child.\n");

				printf("Connection Established successfully\n");
				int ss = service_request(slot);
				exit(1);			
				  			
			}
		}
		close(clients[slot]);
		while (clients[slot]!=-1) 								
		{
			slot = (slot+1)%NUM_CLIENTS;
		}
		
	}   

	return(0);
}

/***************************************************************************************
* FILENAME: udp_client.c                                                               *
* OWNER: Sridhar Pavithrapu.							       *
* FILE DESCRIPTION: This file consists the function's of udp_client.c                  *
***************************************************************************************/


/* Headers section */
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
#include <time.h>

/* Global variables */
#define MAXBUFSIZE 1000


/* Structure for defining the time-out */
struct timeval tv,tv_1,tv_2,end;

/* Structure for the packet */
struct Datagram
{

	unsigned int datagram_id;
	char datagram_message[MAXBUFSIZE];
	unsigned int datagram_length;
};

/* Definition of 'get_file' function */
void get_file(int socket_id, char *file_name, struct sockaddr_in remote_addr)
{
	printf("In get_file case with file name : %s\n",file_name);

	/* Buffers for storing the file confirmation message and size of the file */
	char file_confirmation_message[MAXBUFSIZE]="";
	char request_size[MAXBUFSIZE]="";
	bzero(file_confirmation_message,sizeof(file_confirmation_message));
	bzero(request_size,sizeof(request_size));

	int file_confirmation_received = 0;
	int sent_bytes = 0;
	ssize_t fileSize = 0;
	ssize_t encodedFileSize;
	ssize_t size_check=0;
	int bytes_received = 0;
	int file_size_bytes = 0;
	int received_sequence_count = 0;
	int encoded_id =0;

	/* Receiving the required file confiramtion */
	file_confirmation_received = recvfrom( socket_id, file_confirmation_message, sizeof(file_confirmation_message), 0, NULL,NULL);
	printf("File confirmation received with number of bytes : %d\n",file_confirmation_received);
	printf("File confirmation received as : %s\n",file_confirmation_message);

	/* Check for file existance */
	if(strcmp(file_confirmation_message,"File exist") == 0)
	{
		printf("File exists on server\n");

		/* Requesting size of the file from the server */
		strncpy(request_size,"Requesting Size",strlen("Requesting Size"));
		sent_bytes = sendto( socket_id, request_size, strlen(request_size), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
		printf("Number of bytes of the file name sent : %d\n",sent_bytes);

		/* Receiving the size of the required file */
		file_size_bytes = recvfrom( socket_id, &fileSize, sizeof(fileSize), 0, NULL,NULL);
		printf("Number of bytes of file size received : %d\n",file_size_bytes);
		encodedFileSize = ntohl(fileSize);
		printf("File size received : %ld\n",encodedFileSize);

		/* Create new file where the data to be stored */
		char new_file[MAXBUFSIZE];
		strcpy(new_file,file_name);
		FILE *fp;
		fp = fopen(new_file,"w");

		/* Key for decrypting message */
		char key = 10;
	
		
		if(NULL == fp)
		{
			printf("Error opening the file\n");
			exit(0);
		}
		else
		{
			/* Loop till the entire file is received */
			while(size_check < encodedFileSize)
			{
				/* Structure for storing the packet received */
				struct Datagram *temp = malloc(sizeof(struct Datagram));
				if(temp != NULL)
				{
					bytes_received = recvfrom( socket_id, temp, sizeof(*temp), 0, NULL,NULL);
					/* Check for acknowlegement */
					if(received_sequence_count == temp->datagram_id)
					{
						/* Loop for decrypting the message */
						for(long int i=0;i<temp->datagram_length;i++)
						{

							temp->datagram_message[i] ^= key;
						}
						
						fwrite(temp->datagram_message,1,temp->datagram_length,fp);
						printf("\nReceived sequence id : %d\n",temp->datagram_id);
						printf("size_check : %ld, encodedFileSize : %ld, bytes_received : %d\n",size_check,encodedFileSize,bytes_received);

						encoded_id = ntohl(temp->datagram_id);
						sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
						printf("Encoded sequenced sent with number of bytes : %d\n",sent_bytes);
						/* Incrementing the sequence id and the local variable for size of the file */
						received_sequence_count++;
						size_check += sizeof(temp->datagram_message);
					}
					else
					{
						encoded_id = ntohl(temp->datagram_id);
						sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
						printf("Encoded sequenced sent with number of bytes : %d\n",sent_bytes);
					}
					free(temp);
				}
			}
			printf("Done\n");
			fclose(fp);
		}
	}
	else
	{
		printf("File does not exists on server\n");
	}
}

/* Definition of 'put_file' function */
void put_file(int socket_id, char *file_name, struct sockaddr_in remote_addr)
{
	printf("In put_file case with file name : %s\n",file_name);

	int file_exist_confirmation = 0;
	int bytes_read = 0;
	int bytes_sent = 0;
	int actual_sequence_count = 0;
	int received_sequence_count = 0;
	int decoded_sequence_count = 0;
	ssize_t fileSize,encodedFileSize,size_check;
	char file_exist_buffer[MAXBUFSIZE];

	/* Creating a file pointer to the file to be sent to server */
	FILE *fp;
	fp = fopen(file_name,"r");
	if(fp == NULL)
	{
		printf("File does not exist, sending file doesn't exist information to server\n");
		
		bzero(file_exist_buffer,sizeof(file_exist_buffer));
		strcpy(file_exist_buffer,"File does not exist");
		file_exist_confirmation = sendto(socket_id, file_exist_buffer, strlen(file_exist_buffer), 0, (struct sockaddr*)&remote_addr,  sizeof(remote_addr));
		printf("File exist confirmation message : %s\n",file_exist_buffer);
	}
	else
	{
		printf("File exists, sending file exists information to server\n ");

		bzero(file_exist_buffer,sizeof(file_exist_buffer));
		strcpy(file_exist_buffer,"File exist");
		file_exist_confirmation = sendto(socket_id, file_exist_buffer, strlen(file_exist_buffer), 0, (struct sockaddr*)&remote_addr,  sizeof(remote_addr));
		printf("File exist confiramtion message: %s\n",file_exist_buffer);
		
		printf("Sending the size of the file to server\n");
		fseek(fp, 0, SEEK_END);
   		fileSize = ftell(fp);
  		encodedFileSize = htonl(fileSize);
		rewind(fp);
	
		int file_size = sendto(socket_id, &encodedFileSize, sizeof(encodedFileSize), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
		printf("File size sent with number of bytes : %d\n",file_size);
		printf("File size: %ld\n",fileSize);
		size_check = 0;

		/* Setting the timeout for recvfrom function */
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		    perror("Error");
		}

		/* Key for encrypting message */
		char key = 10;

		/* Loop till the entire file is sent */
		while(size_check < fileSize)
		{
			/* Structure for storing the packet to be sent */
			struct Datagram *temp = malloc(sizeof(struct Datagram));
			if(temp != NULL)
			{
				temp->datagram_id = actual_sequence_count;
				bytes_read = fread(temp->datagram_message,sizeof(char),MAXBUFSIZE,fp);
			
				/* Encrypting the message */
				for(long int i=0;i<bytes_read;i++)
				{
					temp->datagram_message[i] ^= key;
				}
				
				temp->datagram_length = bytes_read;
				printf("\nNumber of bytes read : %d\n",bytes_read);
				printf("Sequence count : %d\n",temp->datagram_id);
			
				bytes_sent = sendto(socket_id, temp, (sizeof(*temp)), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
				printf("Number of bytes sent : %d\n",bytes_sent);
				
				/* Check for the acknowledgement from server */
				if(recvfrom( socket_id, &received_sequence_count, sizeof(received_sequence_count),0, NULL,NULL)>0)
				{
					printf("The server received sequence %d\n", htonl(received_sequence_count));
					decoded_sequence_count = htonl(received_sequence_count);
					if(decoded_sequence_count == actual_sequence_count)
					{
						/* Incrementing the sequence count and local file size variable */
						actual_sequence_count++;
						size_check = size_check + bytes_read;
						printf("size_check : %ld\n",size_check);
					}
					else
					{
						printf("Sending the same sequence '%d' again\n",actual_sequence_count);
						fseek(fp, size_check, SEEK_SET);	
					}
				}
				else
				{
					printf("Sending the same sequence '%d' again\n",actual_sequence_count);
					fseek(fp, size_check, SEEK_SET);
				}
				free(temp);
			}
		}
		printf("Done\n");
		fclose(fp);
	}
}

/* Definition of 'delete_file' function */
void delete_file(int socket_id, char *file_name, struct sockaddr_in remote_addr)
{
	printf("In delete_file case with file name : %s\n",file_name);
	
	/* Sending information of the required file */
	int sent_bytes = sendto( socket_id, file_name, strlen(file_name), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
	printf("Number of bytes sent of the file name deletion : %d\n",sent_bytes);
}

/* Definition of 'list_directory' function */
void list_directory(int socket_id, struct sockaddr_in remote_addr)
{
	printf("In list_directory case\n");

	/* Variable for receiving the file size */
	ssize_t fileSize,encodedFileSize,size_check=0;
	int bytes_received=0;	
	int encoded_id;
	int sent_bytes=0;
	
	/* A buffer to store the file content to be sent */           
	char *file_buffer;
	int file_size_bytes = recvfrom( socket_id, &fileSize, sizeof(fileSize), 0, NULL,NULL);
	printf("Number of bytes of file size received : %d\n",file_size_bytes);
	encodedFileSize = ntohl(fileSize);
	printf("File size received : %ld\n",encodedFileSize);
	int received_sequence_count = 0;
	
	/* Create new file where the data to be stored */
	char new_file[] = "list_file";
	FILE *fp;

	/* Key for decrypting message */
	char key = 10;
	int i;
	fp = fopen(new_file,"w+");
	if(NULL == fp)
	{
		printf("Error opening the file\n");
		exit(0);
	}
	else
	{
		/* Loop till the entire file is received */
		while(size_check < encodedFileSize)
		{
			/* Structure for storing the received packet */
			struct Datagram *temp = malloc(sizeof(struct Datagram));
			if(temp != NULL)
			{
				bytes_received = recvfrom( socket_id, temp, sizeof(*temp), 0, NULL,NULL);
				if(received_sequence_count == temp->datagram_id)
				{
					/* Decrypting the received message */
					for(long int count=0; count<temp->datagram_length; count++)
					{

						temp->datagram_message[count] ^= key;
					}
					
					fwrite(temp->datagram_message,1,temp->datagram_length,fp);
					printf("\nReceived sequence id : %d\n",temp->datagram_id);
					printf("size_check : %ld, bytes_received : %d\n",size_check,bytes_received);
					printf("temp->datagram_length : %d\n",temp->datagram_length);
					
					/* Sending the acknowledgement as sequence id */
					encoded_id = ntohl(temp->datagram_id);
					sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
					printf("Encoded sequenced sent with number of bytes : %d\n",sent_bytes);
					/* Incrementing the sequence count and local file size variable */
					received_sequence_count++;
					size_check += sizeof(temp->datagram_message);
				}
				else
				{
					/* Sending the acknowledgement as sequence id */
					encoded_id = ntohl(temp->datagram_id);
					sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
					printf("Encoded sequenced sent with number of bytes : %d\n",sent_bytes);
				}
				free(temp);
			}
		}
		printf("Done\n");
		fclose(fp);
	}
}

/* Definition of 'exit_server' function */
void exit_server(int socket_id, struct sockaddr_in remote_addr)
{
	printf("In exit_server case\n");

	/* Buffer for storing the exit confirmation message */
	char exit_confirmation_message[MAXBUFSIZE]="";
	bzero(exit_confirmation_message,sizeof(exit_confirmation_message));

	int exit_confirmation_received = recvfrom( socket_id, exit_confirmation_message, sizeof(exit_confirmation_message), 0, NULL,NULL);
	printf("Exit confirmation received with number of bytes : %d\n",exit_confirmation_received);
	printf("Exit confirmation received as : %s\n",exit_confirmation_message);

	/* Check for server exit */
	if(strcmp(exit_confirmation_message,"Exit") == 0)
	{
		printf("Server exited successfully\n");
	}
	else
	{
		printf("Error in exiting the server\n");
	}
	/* Exiting the client after exiting the server */
	exit(0);
	
}

/* Main Function definition */
int main (int argc, char * argv[])
{
 
	/* Creating socket name */                          
	int udp_socket;
	/* Creating internet socket address structure */  
	struct sockaddr_in remote;              
	
	/* Check for input paramaters during execution */
	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	 
	/* Populating the sockaddr_in struct with information of server */
	/* Clearing the struct */	
	bzero(&remote,sizeof(remote));               
	/* Creating the address family */	
	remote.sin_family = AF_INET;                 
	/* Sets port number to network byte order */	
	remote.sin_port = htons(atoi(argv[2]));      
	/* Sets remote IP address */	
	remote.sin_addr.s_addr = inet_addr(argv[1]); 

	/* Creating a generic socket of type UDP (datagram) */
	if ((udp_socket = socket(PF_INET,SOCK_DGRAM,0)) < 0)
	{
		printf("unable to create socket\n");
	}

	/* Number of bytes sent in the message */
	int bytes_sent; 
	/* Local variables for parsing the input given by user */
	char operation[MAXBUFSIZE];
	char temp_operation[MAXBUFSIZE];
	bzero(operation,sizeof(operation));
	bzero(temp_operation,sizeof(temp_operation));
	int count = 0,final_count =0;
	char *delimiter = " "; 
	char *words,*temp;
	char *final_words,*final_temp;	

	/* Loop for getting input from the user for different operation */
	while(1)
	{
		/* Initialising the timeout to be infinite for 'recvfrom' function */
		tv_1.tv_sec = 0;
		tv_1.tv_usec = 0;
		if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO,&tv_1,sizeof(tv_1)) < 0) 
		{
		    perror("Error\n");
		}		
		
		/* Menu for the user */
		printf("\n/*****************************************************************/\n");
		printf("You can perform the following operations using this client:\n");
		printf("1) Get File from the server. [Eg: get 'file_name']\n");
		printf("2) Put File in the server. [Eg: put 'file_name']\n");
		printf("3) Delete File in the server. [Eg: delete 'file_name']\n");
		printf("4) Get server contents by 'ls' command. [Eg: ls]\n");
		printf("5) Exit the server. [Eg: exit]\n");
		printf("/*****************************************************************/\n");
	
		/* Getting input from the user */
		printf("\nPlease enter the operation you want to perform:\n");
		scanf(" %[^\n]s",operation);
		printf("\noperation:%s\n",operation);

		/* Sending the entire operation to the server */
		printf("First sending the entire operation to the server : %s\n",operation);
		bytes_sent = sendto( udp_socket, operation, strlen(operation), 0, (struct sockaddr*)&remote, sizeof(remote));
		printf("Number of bytes of the operation sent : %d\n",bytes_sent);

		/* Parsing the received operation */
		strcpy(temp_operation,operation);
		temp = operation;
		words = strtok(temp,delimiter);
		while(words != NULL)
		{
			printf("Received words : %s\n",words);
			if(strlen(words) > 0)
			{			
				count++;
			}		
			words = strtok(NULL,delimiter);
		}

		printf("Number of words in the received operation : %d \n",count);
		if(count > 2)
		{
			printf("In error case\n");
			printf("Error in the input string\n");
		}
		else
		{
			printf("In success case\n");
			if(count == 2)
			{
				printf("In count == 2 case\n");
				printf("%s\n",temp_operation);
				char first_word[MAXBUFSIZE]="",second_word[MAXBUFSIZE]="";
				final_temp = temp_operation;
				final_words = strtok(final_temp,delimiter);
				printf("final_temp:%s,final_words : %s\n",final_temp,final_words);
				
				while(final_words != NULL)
				{
					if(final_count == 0)
					{
						printf("words : %s\n",final_words);
						strcpy(first_word,final_words);
					}
					if(final_count == 1)
					{
						printf("words : %s\n",final_words);
						strcpy(second_word,final_words);
					}				
					if(strlen(final_words) > 0)
					{			
						final_count++;
					}		
					final_words = strtok(NULL,delimiter);
				}
				printf("Firstword : %s, Second_word : %s\n",first_word,second_word);
			
				
				/* Calling required functions depending on the command received */
				if(strcmp(first_word,"get")==0)
				{
					get_file(udp_socket, second_word, remote);
				}
				else if(strcmp(first_word,"put")==0)
				{
					put_file(udp_socket, second_word, remote);
				}
				else if(strcmp(first_word,"delete")==0)
				{
					delete_file(udp_socket, second_word, remote);
				}
				else
				{
					printf("Error in the input message given by user\n");
				}
			}
			else if(count == 1)
			{
				printf("In count == 1 case\n");
				printf("operation: %s\n",temp_operation);

				/* Calling required functions depending on the command received */
				if(strcmp(temp_operation,"ls")==0)
				{
					list_directory(udp_socket, remote);
				}
				else if(strcmp(temp_operation,"exit")==0)
				{
					exit_server(udp_socket, remote);
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
		}
		
		/* Resetting the local variables */
 		bzero(operation,sizeof(operation));
		bzero(temp_operation,sizeof(temp_operation));
		count = 0;
		final_count =0;
	}

	/* Closing the socket */
	close(udp_socket);
}

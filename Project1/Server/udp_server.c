/***************************************************************************************
* FILENAME: udp_server.c                                                               *
* OWNER: Sridhar Pavithrapu.							       *
* FILE DESCRIPTION: This file consists the function's of udp_server.c                  *
***************************************************************************************/


/* Headers section */
#include <sys/types.h>
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
#include <string.h>
#include <dirent.h>
#include <time.h>


/* GLobal variables */
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
void get_file(int socket_id, char *file_name, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	printf("In get_file case\n");

	/* A buffer to store our command received */    
	char command_buffer[MAXBUFSIZE],size_buffer[MAXBUFSIZE]; 
	/* A buffer to store the file content to be sent */           
	char *file_buffer;
	int temp_bytes;
	int file_exist_confirmation = 0;
	ssize_t fileSize;
	ssize_t encodedFileSize;
	ssize_t size_check;
	int bytes_read = 0;
	int bytes_sent = 0;
	int actual_sequence_count = 0;
	int received_sequence_count = 0;
	int decoded_sequence_count = 0;
	int file_size = 0;

	printf("The client requires the file : %s\n", file_name);

	/* Creating a file pointer to the requested file from the client */
	FILE *fp;
	fp = fopen(file_name,"r");
	if(fp == NULL)
	{
		printf("Sending File does not exist confirmation to client\n");

		bzero(command_buffer,sizeof(command_buffer));
		strcat(command_buffer,"File does not exist");
		file_exist_confirmation = sendto(socket_id, command_buffer, strlen(command_buffer), 0, (struct sockaddr*)&remote_addr, remote_len);
		printf("File exist confiramtion sent 'no' with number of bytes : %d\n",file_exist_confirmation);
	}
	else
	{		
		printf("Sending File exist confirmation to client\n");

		bzero(command_buffer,sizeof(command_buffer));
		strcpy(command_buffer,"File exist");
		file_exist_confirmation = sendto(socket_id, command_buffer, strlen(command_buffer), 0, (struct sockaddr*)&remote_addr, remote_len);
		printf("File exist confiramtion : %s\n",command_buffer);
		printf("File exist confiramtion sent 'yes' with number of bytes : %d\n",file_exist_confirmation);
	
		bzero(size_buffer,sizeof(size_buffer));
		recvfrom( socket_id, size_buffer, MAXBUFSIZE, 0, (struct sockaddr*)&remote_addr, &remote_len);
		printf("The client says : %s\n", command_buffer);

		printf("Sending the size of the file\n");
		fseek(fp, 0, SEEK_END);
   		fileSize = ftell(fp);
  		encodedFileSize = htonl(fileSize);
		rewind(fp);

		file_size = sendto(socket_id, &encodedFileSize, sizeof(encodedFileSize), 0, (struct sockaddr*)&remote_addr, remote_len);
		printf("File size sent with number of bytes : %d\n",file_size);
		printf("File size : %ld\n",fileSize);
		size_check = 0;

		/* Key for encrypting message */
		char key = 10;
		
		/* Setting the timeout for recvfrom function */
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		    perror("Error");
		}
	
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

				bytes_sent = sendto(socket_id, temp, (sizeof(*temp)), 0, (struct sockaddr*)&remote_addr, remote_len);
				printf("Number of bytes sent : %d\n",bytes_sent);
				
				/* Check for the acknowledgement from client */
				if(recvfrom( socket_id, &received_sequence_count, sizeof(received_sequence_count), 0, (struct sockaddr*)&remote_addr, &remote_len)>0)
				{	
					printf("The client received sequence %d\n", htonl(received_sequence_count));
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
						printf("Sending the same sequence inside receive from %d again",actual_sequence_count);
						fseek(fp, size_check, SEEK_SET);	
					}
				}
				else
				{
					printf("Sending the same sequence %d again",actual_sequence_count);
					fseek(fp, size_check, SEEK_SET);
				}
				free(temp);
			}
		}
		printf("Done\n");
		fclose(fp);	
	}
}

/* Definition of 'put_file' function */
void put_file(int socket_id, char *file_name, struct sockaddr_in remote_addr, unsigned int remote_len)
{	
	printf("In put_file case\n");	

	char file_exist_buffer[MAXBUFSIZE];
	int bytes_received = 0;
	int file_exist_bytes = 0;
	ssize_t fileSize=0;
	ssize_t encodedFileSize=0;
	ssize_t size_check=0;
	int received_sequence_count = 0;
	int encoded_id = 0;
	int sent_bytes = 0;
	int file_size_bytes = 0;
	bzero(file_exist_buffer,sizeof(file_exist_buffer));
	
	/* Receiving the file existance confirmation from the client */
	file_exist_bytes = recvfrom( socket_id, file_exist_buffer, sizeof(file_exist_buffer), 0, (struct sockaddr*)&remote_addr, &remote_len);
	printf("Number of bytes of file exists received : %d\n",file_exist_bytes);
	
	/* Check for file existance */
	if(strcmp(file_exist_buffer,"File exist") == 0)
	{

		/* Receiving the file size from the client */
		file_size_bytes = recvfrom( socket_id, &fileSize, sizeof(fileSize), 0, (struct sockaddr*)&remote_addr, &remote_len);
		printf("Number of bytes of file size received : %d\n",file_size_bytes);
		encodedFileSize = ntohl(fileSize);
		printf("File size received : %ld\n",encodedFileSize);
	
		/* A buffer to store the file content received */                               
		char new_file[MAXBUFSIZE];
		strcpy(new_file,file_name);
		FILE *fp;
		fp = fopen(new_file,"w+");

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
					bytes_received = recvfrom( socket_id, temp, sizeof(*temp), 0, (struct sockaddr*)&remote_addr, &remote_len);
					/* Check for acknowlegement */					
					if(received_sequence_count == temp->datagram_id)
					{
						/* Loop for decrypting the message */
						for(long int i=0; i<temp->datagram_length; i++)
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
		printf("Requested file does not exist on server\n");
	}
}

/* Definition of 'delete_file' function */
void delete_file(int socket_id, char *file_name, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	printf("In delete_file case\n");

	/* A buffer to store our command received */    
	char command_buffer[MAXBUFSIZE]; 
	int temp_bytes;
	int ret = 0;
	/* Receiving file name to be deleted from the client */
	bzero(command_buffer,sizeof(command_buffer));
	temp_bytes = recvfrom( socket_id, command_buffer, MAXBUFSIZE, 0, (struct sockaddr*)&remote_addr, &remote_len);
	printf("The client requires the file '%s' to be removed\n", command_buffer);
	
	/* Creating a file pointer to the requested file from the client */
	FILE *fp;
	fp = fopen(command_buffer,"r");
	if(fp == NULL)
	{
		printf("Requested file does not exist, so not deleting any file\n");
	}
	else
	{
		printf("Requested file exist, so deleting the file : %s\n",command_buffer);
		ret = remove(command_buffer);
		if(ret == 0)
		{
			printf("File deleted successfully\n");
		}
		else
		{
			printf("Error in deleting file\n");
		}
	}
}

/* Definition of 'list_directory' function */
void list_directory(int socket_id, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	printf("In list_directory case\n");

	char new_file[] = "list_file";
	FILE *fp;
	int bytes_sent,bytes_read;
	ssize_t fileSize,encodedFileSize,size_check;
	int file_size = 0;
	int actual_sequence_count = 0;
	int received_sequence_count = 0;
	int decoded_sequence_count;

	/* Opening the file */
	fp = fopen(new_file,"w+");
	if(NULL == fp)
	{
		printf("Error opening the list file\n");
		exit(0);
	}
	else
	{
		DIR *d;
		struct dirent *dir;
		d = opendir(".");
		if(d)
		{
			printf("Listing the contents of the present directory\n");
			while( (dir = readdir(d)) != NULL)
			{
				printf("%s\n",dir->d_name);
				fprintf(fp,"%s\n",dir->d_name);
			}
			closedir(d);
		}
		/* Sending file size to the client */
		fileSize = ftell(fp);
		encodedFileSize = ntohl(fileSize);
		rewind(fp);
		file_size = sendto(socket_id, &encodedFileSize, sizeof(encodedFileSize), 0, (struct sockaddr*)&remote_addr, remote_len);
		printf("\nFile size sent with number of bytes : %d\n",file_size);
		printf("File size : %ld\n",fileSize);
		size_check = 0;
		
		/* Key for encrypting message */
		char key = 10;
	
		/* Setting the timeout for recvfrom function */
		tv_2.tv_sec = 0;
		tv_2.tv_usec = 100000;
		if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv_2,sizeof(tv_2)) < 0) {
		    perror("Error");
		}
		
		/* Loop till the entire file is sent */
		while(size_check < fileSize)
		{
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
				printf("temp->datagram_length : %d\n",temp->datagram_length);
				bytes_sent = sendto(socket_id,  temp, (sizeof(*temp)), 0, (struct sockaddr*)&remote_addr, remote_len);
				printf("Number of bytes sent : %d\n",bytes_sent);
				
				/* Check for the acknowledgement from server */
				if(recvfrom( socket_id, &received_sequence_count, sizeof(received_sequence_count), 0, (struct sockaddr*)&remote_addr, &remote_len)>0)
				{				
					printf("The client received sequence : %d\n", htonl(received_sequence_count));
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
						printf("Sending the same sequence '%d' again",actual_sequence_count);
						fseek(fp, size_check, SEEK_SET);	
					}
				}
				else
				{
					printf("Sending the same sequence '%d' again",actual_sequence_count);
					fseek(fp, size_check, SEEK_SET);
				}
				free(temp);
			}
		}
		printf("Done\n");
		fclose(fp);	
	}
}

/* Definition of 'exit_server' function */
void exit_server(int socket_id, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	printf("In exit_server case\n");
	
	char exit_buffer[MAXBUFSIZE];
	int exit_confirmation = 0;

	/* Sending server exit confirmation to client */
	bzero(exit_buffer,sizeof(exit_buffer));
	strcat(exit_buffer,"Exit");
	printf("Exit_buffer : %s\n",exit_buffer);
	exit_confirmation = sendto(socket_id, exit_buffer, strlen(exit_buffer), 0, (struct sockaddr*)&remote_addr, remote_len);
	printf("Server exit confirmation sent to client with number of bytes : %d\n",exit_confirmation);
	exit(0);
}

/* Main Function definition */
int main(int argc, char *argv[])
{
	/* Creating socket name */
	int udp_socket,client_socket; 
	/* Creating internet socket address structure */                          
	struct sockaddr_in sin, remote;     
	/* Length of the sockaddr_in structure */
	unsigned int remote_length;        
	/* Number of bytes received in the message */ 
	int nbytes = 0;                    
	  

	/* Check for input paramaters during execution */
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/* Creating sockaddr_in struct with the information about our socket */
	/* Zero the struct */
	bzero(&sin,sizeof(sin));                   
	/* Assingn the address family */
	sin.sin_family = AF_INET;                  
	/* Set the input port number to network byte order using htons() function */	
	sin.sin_port = htons(atoi(argv[1]));        
	/* Supplies the IP address of the local machine */
	sin.sin_addr.s_addr = INADDR_ANY;          


	/* Creating a generic socket of type UDP (datagram) */
	if ((udp_socket =socket(PF_INET,SOCK_DGRAM,0)) < 0)
	{
		printf("unable to create socket\n");
	}


	/* Binding the socket created to local address and port supplied in the sockaddr_in struct */
	if (bind(udp_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	/* Finding length of the socket address */
	remote_length = sizeof(remote);

	/* Variables for parsing the command rececived from the client */
	int count = 0,final_count =0;
	char *delimiter = " "; 
	char *words,*temp;
	char *final_words,*final_temp;	
	char operation[MAXBUFSIZE];
	char temp_operation[MAXBUFSIZE];
	bzero(operation,sizeof(operation));
	bzero(temp_operation,sizeof(temp_operation));

	/* Loop for different operations received from the client */
	while(1)
	{
		/* Initialising the timeout to be infinite for 'recvfrom' function */
		tv_1.tv_sec = 0;
		tv_1.tv_usec = 0;
		if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO,&tv_1,sizeof(tv_1)) < 0) 
		{
		    perror("Error");
		}

		/* Waits for an incoming message */
		printf("\nFirst receiving the entire operation from the client\n");
		nbytes = recvfrom( udp_socket, operation, MAXBUFSIZE, 0, (struct sockaddr*)&remote, &remote_length);
		printf("Operation received from the client : %s\n", operation);

		/* Parsing the received operation */
		strcpy(temp_operation,operation);
		temp = operation;
		words = strtok(temp,delimiter);
		while(words != NULL)
		{
			printf("words: %s\n",words);
			if(strlen(words) > 0)
			{			
				count++;
			}		
			words = strtok(NULL,delimiter);
		}

		printf("Number of words in the string : %d\n",count);
		if(count > 2)
		{
			printf("In error case\n");
			printf("Error in the received operation\n");
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
				printf("final_temp : %s, final_words : %s\n",final_temp,final_words);

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
				printf("Firstword : %s,Second_word : %s\n",first_word,second_word);
			
				
				/* Calling required functions depending on the command received */
				if(strcmp(first_word,"get")==0)
				{
					get_file(udp_socket, second_word, remote,remote_length);
				}
				else if(strcmp(first_word,"put")==0)
				{
					put_file(udp_socket, second_word, remote,remote_length);
				}
				else if(strcmp(first_word,"delete")==0)
				{
					delete_file(udp_socket, second_word, remote,remote_length);
				}
				else
				{
					printf("Error in the input message received from the client\n");
				}
			}
			else if(count == 1)
			{
				printf("In count == 2 case\n");
				printf("operation: %s\n",temp_operation);

				/* Calling required functions depending on the command received */
				if(strcmp(temp_operation,"ls")==0)
				{
					list_directory(udp_socket, remote,remote_length);
				}
				else if(strcmp(temp_operation,"exit")==0)
				{
					exit_server(udp_socket, remote,remote_length);
				}
				else
				{
					printf("Error in the input message received from the client\n");
				}
			}
			else
			{
				printf("Error in the input message received from the client\n");
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

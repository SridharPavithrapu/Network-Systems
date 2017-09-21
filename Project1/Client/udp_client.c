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

/* GLobal variables */
#define MAXBUFSIZE 1000

struct timeval tv,tv_1,tv_2,end;

struct Datagram
{

	unsigned int datagram_id;
	char datagram_message[MAXBUFSIZE];
	unsigned int datagram_length;
};


void get_file(int socket_id, char *file_name, struct sockaddr_in remote_addr)
{
	char file_confirmation_message[MAXBUFSIZE]="",request_size[MAXBUFSIZE]="";
	usleep(1);
	bzero(file_confirmation_message,sizeof(file_confirmation_message));
	bzero(request_size,sizeof(request_size));

	/* Sending information of the required file */
	printf("\nNow sending the file name to the server:%s\n",file_name);
	int sent_bytes = sendto( socket_id, file_name, strlen(file_name), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
	printf("Number of bytes of the file name sent : %d\n",sent_bytes);	
	
	int file_confirmation_received = recvfrom( socket_id, file_confirmation_message, sizeof(file_confirmation_message), 0, NULL,NULL);
	printf("File confirmation received with number of bytes : %d\n",file_confirmation_received);
	printf("File confirmation received as:%s\n",file_confirmation_message);

	

	if(strcmp(file_confirmation_message,"File exist") == 0)
	{
		printf("File exists on server\n");

		strncpy(request_size,"Requesting Size",strlen("Requesting Size"));
		sent_bytes = sendto( socket_id, request_size, strlen(request_size), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
		printf("Number of bytes of the file name sent : %d\n",sent_bytes);

		ssize_t fileSize,encodedFileSize,size_check=0;
		int bytes_received;
		int file_size_bytes = recvfrom( socket_id, &fileSize, sizeof(fileSize), 0, NULL,NULL);
		printf("Number of bytes of file size received: %d\n",file_size_bytes);
		encodedFileSize = ntohl(fileSize);
		printf("File size received: %ld\n",encodedFileSize);

		/* Create new file where the data to be stored */
		char new_file[] = "new_file";
		FILE *fp;
		fp = fopen(new_file,"w+");
		int received_sequence_count = 0;
		if(NULL == fp)
		{
			printf("Error opening the file\n");
			exit(0);
		}
		else
		{
			

			while(size_check < encodedFileSize)
			{
				struct Datagram *temp = malloc(sizeof(struct Datagram));
				if(temp != NULL)
				{
					bytes_received = recvfrom( socket_id, temp, sizeof(*temp), 0, NULL,NULL);
					if(received_sequence_count == temp->datagram_id)
					{
						
						fwrite(temp->datagram_message,1,temp->datagram_length,fp);
						printf("Received sequence id:%d",temp->datagram_id);
						printf("size_check:%ld,encodedFileSize:%ld,bytes_received:%d\n",size_check,encodedFileSize,bytes_received);

						int encoded_id = ntohl(temp->datagram_id);
						int sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
						printf("Encoded sequenced sent with number of bytes : %d\n",sent_bytes);
						free(temp);
						received_sequence_count++;
						size_check += sizeof(temp->datagram_message);
					}
					else
					{
						int encoded_id = ntohl(temp->datagram_id);
						int sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
						printf("Encoded sequenced sent with number of bytes : %d\n",sent_bytes);
					}
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


void put_file(int socket_id, char *file_name, struct sockaddr_in remote_addr)
{
	printf("In put_file case with file name:%s\n",file_name);
	/* Creating a file pointer to the file to be sent to server */
	FILE *fp;
	int temp_bytes;
	int bytes_read, bytes_sent;
	fp = fopen(file_name,"r");
	if(fp == NULL)
	{
		printf("File does not exist, so not sending any file to server\n");
	}
	else
	{
		ssize_t fileSize,encodedFileSize,size_check;
		printf("File exists, so sending the file to server:%s\n",file_name);
		printf("Sending the size of the file\n");
		fseek(fp, 0, SEEK_END);
   		fileSize = ftell(fp);
  		encodedFileSize = htonl(fileSize);
		rewind(fp);
		int file_size = sendto(socket_id, &encodedFileSize, sizeof(encodedFileSize), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
		printf("File size sent with number of bytes : %d\n",file_size);
		printf("File size: %ld\n",fileSize);
		size_check = 0;
		int actual_sequence_count = 0;
		int received_sequence_count = 0;
		int decoded_sequence_count;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		    perror("Error");
		}

		
		while(size_check < fileSize)
		{
			struct Datagram *temp = malloc(sizeof(struct Datagram));
			if(temp != NULL)
			{
				temp->datagram_id = actual_sequence_count;
				bytes_read = fread(temp->datagram_message,sizeof(char),MAXBUFSIZE,fp);
				temp->datagram_length = bytes_read;
				printf("Number of bytes read: %d\n",bytes_read);
				printf("Sequence count: %d\n",temp->datagram_id);
			
				bytes_sent = sendto(socket_id, temp, (sizeof(*temp)), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
				printf("Number of bytes sent : %d\n",bytes_sent);
				free(temp);

				if(recvfrom( socket_id, &received_sequence_count, sizeof(received_sequence_count),0, NULL,NULL)>0)
				{
					printf("The server received sequence %d\n", htonl(received_sequence_count));
					decoded_sequence_count = htonl(received_sequence_count);
					if(decoded_sequence_count == actual_sequence_count)
					{
						actual_sequence_count++;
						size_check = size_check + bytes_read;
						printf("size_check : %ld\n",size_check);
					}
					else
					{
						printf("Sending the same sequence %d again",actual_sequence_count);
						fseek(fp, size_check, SEEK_SET);	
					}
					
				}
				else
				{
					printf("Sending the same sequence %d again",actual_sequence_count);
					fseek(fp, size_check, SEEK_SET);
				}
			}
			
		}
		printf("Done\n");
		fclose(fp);
	}

}

void delete_file(int socket_id, char *file_name, struct sockaddr_in remote_addr)
{
	printf("In delete_file case with file name:%s\n",file_name);
	/* Sending information of the required file */
	int sent_bytes = sendto( socket_id, file_name, strlen(file_name), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
	printf("Number of bytes of the file name sent : %d\n",sent_bytes);
}

void list_directory(int socket_id, struct sockaddr_in remote_addr)
{
	ssize_t fileSize,encodedFileSize,size_check=0;
	int bytes_received;
	/* A buffer to store the file content to be sent */           
	char *file_buffer;
	int file_size_bytes = recvfrom( socket_id, &fileSize, sizeof(fileSize), 0, NULL,NULL);
	printf("Number of bytes of file size received: %d\n",file_size_bytes);
	encodedFileSize = ntohl(fileSize);
	printf("File size received: %ld\n",encodedFileSize);
	int received_sequence_count = 0;
	/* Create new file where the data to be stored */
	char new_file[] = "list_file";
	FILE *fp;
	int i;
	fp = fopen(new_file,"w+");
	if(NULL == fp)
	{
		printf("Error opening the file\n");
		exit(0);
	}
	else
	{


		while(size_check < encodedFileSize)
		{
			struct Datagram *temp = malloc(sizeof(struct Datagram));
			if(temp != NULL)
			{
				bytes_received = recvfrom( socket_id, temp, sizeof(*temp), 0, NULL,NULL);
				if(received_sequence_count == temp->datagram_id)
				{
					
					fwrite(temp->datagram_message,1,temp->datagram_length,fp);
					printf("Received sequence id:%d\n",temp->datagram_id);
					printf("size_check:%ld,encodedFileSize:%ld,bytes_received:%d\n",size_check,encodedFileSize,bytes_received);
					printf("temp->datagram_length:%d",temp->datagram_length);
					int encoded_id = ntohl(temp->datagram_id);
					int sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
					printf("Encoded sequenced sent with number of bytes : %d\n",sent_bytes);
					free(temp);
					received_sequence_count++;
					size_check += sizeof(temp->datagram_message);
				}
				else
				{
					int encoded_id = ntohl(temp->datagram_id);
					int sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
					printf("Encoded sequenced sent with number of bytes : %d\n",sent_bytes);
				}
			}
		}

		printf("Done\n");
		fclose(fp);
	}
	
}

void exit_server(int socket_id, struct sockaddr_in remote_addr)
{
	char exit_confirmation_message[MAXBUFSIZE]="";
	bzero(exit_confirmation_message,sizeof(exit_confirmation_message));
	int exit_confirmation_received = recvfrom( socket_id, exit_confirmation_message, sizeof(exit_confirmation_message), 0, NULL,NULL);
	printf("Exit confirmation received with number of bytes : %d\n",exit_confirmation_received);
	printf("Exit confirmation received as:%s\n",exit_confirmation_message);

	if(strcmp(exit_confirmation_message,"Exit") == 0)
	{
		printf("Server exited successfully\n");
	}
	else
	{
		printf("Error in exiting the server\n");
	}
	exit(0);
	
}

/* Main FUnction definition */
int main (int argc, char * argv[])
{

	/* Number of bytes sent in the message */
	int bytes_sent;  
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
	char operation[MAXBUFSIZE];
	char temp_operation[MAXBUFSIZE];
	int count = 0,final_count =0;
	char *delimiter = " "; 
	char *words,*temp;
	char *final_words,*final_temp;	

	while(1)
	{
		tv_1.tv_sec = 0;
		tv_1.tv_usec = 0;
		if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO,&tv_1,sizeof(tv_1)) < 0) 
		{
		    perror("Error");
		}		
		
		/* Menu for the user */
		printf("/*****************************************************************/\n");
		printf("You can perform the following operations using this client:\n");
		printf("1) Get File from the server.\n");
		printf("2) Put File in the server.\n");
		printf("3) Delete File in the server.\n");
		printf("4) Get server contents by 'ls' command.\n");
		printf("5) Exit the server.\n");
		printf("/*****************************************************************/\n");
	
		bzero(operation,sizeof(operation));
		bzero(temp_operation,sizeof(temp_operation));
		count = 0;
		final_count =0;
		printf("\n Please enter the operation you want to perform:\n");

		scanf(" %[^\n]s",operation);
		printf("\n operation:%s\n",operation);
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
		printf("Number of words in the string: %d \n",count);
		if(count > 2)
		{
			printf("Error in the input string\n");
		}
		else
		{
			printf("In success case\n");
			if(count == 2)
			{
				printf("In count == 2 case\n");
				printf("%s\n",temp_operation);
				char first_word[MAXBUFSIZE]="",command[MAXBUFSIZE]="";
				final_temp = temp_operation;
				final_words = strtok(final_temp,delimiter);
				printf("final_temp:%s,final_words:%s\n",final_temp,final_words);
				while(final_words != NULL)
				{
					if(final_count == 0)
					{
						printf("words: %s\n",final_words);
						strcpy(first_word,final_words);
					}
					if(final_count == 1)
					{
						printf("words: %s\n",final_words);
						strcpy(command,final_words);
					}				
					if(strlen(final_words) > 0)
					{			
						final_count++;
					}		
					final_words = strtok(NULL,delimiter);
				}
				printf("Firstword:%s,Second_word:%s\n",first_word,command);
			
				printf("\nFirst sending the operation to the server:%s\n",first_word);
				bytes_sent = sendto( udp_socket, first_word, strlen(first_word), 0, (struct sockaddr*)&remote, sizeof(remote));
				printf("Number of bytes of the file name sent : %d\n",bytes_sent);
			
				if(strcmp(first_word,"get")==0)
				{
					get_file(udp_socket, command, remote);
				}
				else if(strcmp(first_word,"put")==0)
				{
					put_file(udp_socket, command, remote);
				}
				else if(strcmp(first_word,"delete")==0)
				{
					delete_file(udp_socket, command, remote);
				}
				else
				{
					printf("Error in the input message given by user\n");
				}
			}
			else if(count == 1)
			{
				printf("In count == 2 case\n");
				printf("operation: %s\n",temp_operation);
				printf("First sending the operation to the server:%s\n",temp_operation);
				int temp_bytes_sent = sendto( udp_socket, temp_operation, strlen(temp_operation), 0, (struct sockaddr*)&remote, sizeof(remote));
				printf("Number of bytes of the file name sent : %d\n",temp_bytes_sent);

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
		}
	usleep(1);
	}
	/* Closing the socket */
	close(udp_socket);

}


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

struct timeval tv,tv_1,tv_2,end;

struct Datagram 
{

	unsigned int datagram_id;
	char datagram_message[MAXBUFSIZE];
	unsigned int datagram_length;
};


void get_file(int socket_id, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	/* A buffer to store our command received */    
	char command_buffer[MAXBUFSIZE],size_buffer[MAXBUFSIZE],file_name_buffer[MAXBUFSIZE]; 
	/* A buffer to store the file content to be sent */           
	char *file_buffer;
	int temp_bytes;

	bzero(file_name_buffer,sizeof(file_name_buffer));
	temp_bytes = recvfrom( socket_id, file_name_buffer, MAXBUFSIZE, 0, (struct sockaddr*)&remote_addr, &remote_len);
	printf("The client requires the file %s\n", file_name_buffer);
	

	/* Creating a file pointer to the requested file from the client */
	FILE *fp;
	fp = fopen(file_name_buffer,"r");
	if(fp == NULL)
	{
		printf("Sending File does not exist to client\n");
		bzero(command_buffer,sizeof(command_buffer));
		strcat(command_buffer,"File does not exist");
		int file_exist_confirmation = sendto(socket_id, command_buffer, strlen(command_buffer), 0, (struct sockaddr*)&remote_addr, remote_len);
		printf("File exist confiramtion sent 'no' with number of bytes : %d\n",file_exist_confirmation);
	}
	else
	{
		ssize_t fileSize,encodedFileSize,size_check;
		int bytes_read,bytes_sent;
		printf("Sending File exist to client\n");
		bzero(command_buffer,sizeof(command_buffer));
		strcpy(command_buffer,"File exist");
		int file_exist_confirmation = sendto(socket_id, command_buffer, strlen(command_buffer), 0, (struct sockaddr*)&remote_addr, remote_len);
		printf("File exist confiramtion : %s\n",command_buffer);
		printf("File exist confiramtion sent 'yes' with number of bytes : %d\n",file_exist_confirmation);
	
		bzero(size_buffer,sizeof(size_buffer));
		recvfrom( socket_id, size_buffer, MAXBUFSIZE, 0, (struct sockaddr*)&remote_addr, &remote_len);
		printf("The client says %s\n", command_buffer);

		printf("Sending the size of the file\n");
		fseek(fp, 0, SEEK_END);
   		fileSize = ftell(fp);
  		encodedFileSize = htonl(fileSize);
		rewind(fp);
		int file_size = sendto(socket_id, &encodedFileSize, sizeof(encodedFileSize), 0, (struct sockaddr*)&remote_addr, remote_len);
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

				bytes_sent = sendto(socket_id, temp, (sizeof(*temp)), 0, (struct sockaddr*)&remote_addr, remote_len);
				printf("Number of bytes sent : %d\n",bytes_sent);
				free(temp);

				if(recvfrom( socket_id, &received_sequence_count, sizeof(received_sequence_count), 0, (struct sockaddr*)&remote_addr, &remote_len)>0)
				{	printf("The client received sequence %d\n", htonl(received_sequence_count));
					decoded_sequence_count = htonl(received_sequence_count);
					if(decoded_sequence_count == actual_sequence_count)
					{
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
				printf("Done\n");
			}
		}
		
		fclose(fp);	
	}

}

void put_file(int socket_id, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	printf("In put_file case, waiting for file size to be received from client\n");
	ssize_t fileSize=0,encodedFileSize=0,size_check=0;
	int file_size_bytes = recvfrom( socket_id, &fileSize, sizeof(fileSize), 0, (struct sockaddr*)&remote_addr, &remote_len);
	printf("Number of bytes of file size received: %d\n",file_size_bytes);
	encodedFileSize = ntohl(fileSize);
	printf("File size received: %ld\n",encodedFileSize);
	
	/* A buffer to store the file content received */                               
	int bytes_received;
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
				bytes_received = recvfrom( socket_id, temp, sizeof(*temp), 0, (struct sockaddr*)&remote_addr, &remote_len);
				if(received_sequence_count == temp->datagram_id)
				{
					fwrite(temp->datagram_message,1,temp->datagram_length,fp);
					printf("Received sequence id:%d",temp->datagram_id);
					printf("size_check:%ld,encodedFileSize:%ld,bytes_received:%d\n",size_check,encodedFileSize,bytes_received);
					int encoded_id = ntohl(temp->datagram_id);
					int sent_bytes = sendto( socket_id, &encoded_id, sizeof(encoded_id), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
					printf("Encoded sequenced sent with number of bytes%d, with data : %d\n",sent_bytes,encoded_id);
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

void delete_file(int socket_id, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	/* A buffer to store our command received */    
	char command_buffer[MAXBUFSIZE]; 
	int temp_bytes;

	bzero(command_buffer,sizeof(command_buffer));
	temp_bytes = recvfrom( socket_id, command_buffer, MAXBUFSIZE, 0, (struct sockaddr*)&remote_addr, &remote_len);
	printf("The client requires the file %s to be removed\n", command_buffer);
	
	/* Creating a file pointer to the requested file from the client */
	FILE *fp;
	fp = fopen(command_buffer,"r");
	if(fp == NULL)
	{
		printf("Requested file does not exist, so not deleting any file\n");
	}
	else
	{
		printf("Requested file exist, so deleting the file:%s\n",command_buffer);
		int ret = remove(command_buffer);
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

void list_directory(int socket_id, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	char new_file[] = "list_file";
	FILE *fp;
	int bytes_sent,bytes_read;
	fp = fopen(new_file,"w+");
	if(NULL == fp)
	{
		printf("Error opening the list file\n");
		exit(0);
	}
	else
	{
		DIR *d;
		
		ssize_t fileSize,encodedFileSize,size_check;
		struct dirent *dir;
		d = opendir(".");
		if(d)
		{
			while( (dir = readdir(d)) != NULL)
			{
				printf("%s\n",dir->d_name);
				fprintf(fp,"%s\n",dir->d_name);
			}
			closedir(d);
		}
		fileSize = ftell(fp);
		encodedFileSize = ntohl(fileSize);
		rewind(fp);
		int file_size = sendto(socket_id, &encodedFileSize, sizeof(encodedFileSize), 0, (struct sockaddr*)&remote_addr, remote_len);
		printf("File size sent with number of bytes : %d\n",file_size);
		printf("File size: %ld\n",fileSize);
		size_check = 0;
		int actual_sequence_count = 0;
		int received_sequence_count = 0;
		int decoded_sequence_count;

		tv_2.tv_sec = 0;
		tv_2.tv_usec = 100000;
		if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv_2,sizeof(tv_2)) < 0) {
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
				printf("temp->datagram_length: %d\n",temp->datagram_length);
				bytes_sent = sendto(socket_id,  temp, (sizeof(*temp)), 0, (struct sockaddr*)&remote_addr, remote_len);
				printf("Number of bytes sent : %d\n",bytes_sent);
				free(temp);

				if(recvfrom( socket_id, &received_sequence_count, sizeof(received_sequence_count), 0, (struct sockaddr*)&remote_addr, &remote_len)>0)
				{				
					printf("The client received sequence %d\n", htonl(received_sequence_count));
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

void exit_server(int socket_id, struct sockaddr_in remote_addr, unsigned int remote_len)
{
	char exit_buffer[MAXBUFSIZE];
	bzero(exit_buffer,sizeof(exit_buffer));
	strcat(exit_buffer,"Exit");
	printf("exit_buffer : %s\n",exit_buffer);
	int exit_confirmation = sendto(socket_id, exit_buffer, strlen(exit_buffer), 0, (struct sockaddr*)&remote_addr, remote_len);
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
	int nbytes;                    
	  
	char operation_buffer[MAXBUFSIZE];
	
	
	/* Check for input paramaters during execution */
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/*  Creating sockaddr_in struct with the information about our socket */
	/* zero the struct */
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

	while(1)
	{
		tv_1.tv_sec = 0;
		tv_1.tv_usec = 0;
		if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO,&tv_1,sizeof(tv_1)) < 0) 
		{
		    perror("Error");
		}

		/* Waits for an incoming message */
		/* Clearing the operation_buffer */
		bzero(operation_buffer,sizeof(operation_buffer));
		printf("First receiving the operation\n");
		nbytes = recvfrom( udp_socket, operation_buffer, MAXBUFSIZE, 0, (struct sockaddr*)&remote, &remote_length);
		printf("Operation performing %s\n", operation_buffer);

		

		if(strcmp(operation_buffer,"get") == 0)
		{		
			get_file(udp_socket,remote,remote_length);
		}
		else if(strcmp(operation_buffer,"put") == 0)
		{		
			put_file(udp_socket,remote,remote_length);
		}
		else if(strcmp(operation_buffer,"delete") == 0)
		{		
			delete_file(udp_socket,remote,remote_length);
		}
		else if(strcmp(operation_buffer,"ls") == 0)
		{		
			list_directory(udp_socket,remote,remote_length);
		}
		else if(strcmp(operation_buffer,"exit") == 0)
		{		
			exit_server(udp_socket,remote,remote_length);
		}
		else
		{
			printf("Error command sent from client\n");
		}
		usleep(1);
	}
	
	/* Closing the socket */
	//close(udp_socket);
}


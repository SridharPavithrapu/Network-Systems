/***************************************************************************************
* FILENAME: server.c                                                                   *
* OWNER: Sridhar Pavithrapu.				                     			       *
* FILE DESCRIPTION: This file consists the function's of dfc.c                  *
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
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


#define BUFFER	(1024)



typedef struct 
{
	char command[BUFFER];
	char user_name[BUFFER];
	char user_password[BUFFER];
	
} CommandStruct;

/* Structure for file parsing commands */
typedef struct 
{
	char server_name[BUFFER];
	int server_port; 
	
} ServerStruct;

/* Structure for file parsing commands */
typedef struct 
{
	char message[BUFFER];
	int message_length; 
	
} MessageStruct;

ServerStruct server[4];
char username[BUFFER];
char password[BUFFER];
int sockets[4];
struct sockaddr_in server_address;
int server_user_availability[4] = {0};
char receive_buffer[BUFFER];


/* Definition of 'parse_config' function */
int parse_config(char * client_config_file) 
{
	FILE *fp;
	char temp[BUFFER];
	int server_count = 0;
	char * line = NULL;
	char *addr;
	size_t len = 0;
	ssize_t read;
	if ((fp = fopen(client_config_file,"r")) == NULL)					
	{
		printf("Client configuration file not found.\n So, exiting\n");
		return 0;
	}
	else
	{
		/* Line-by-line read config file */
		while ((read = getline(&line, &len, fp)) != -1) 
		{
			
			if(strstr(line, "Server") != NULL)
			{
				memset(&server[server_count], 0 , sizeof(server[server_count]));
				char *string_token = strtok(line," ");
				int temp_count = 0;
				while(string_token != NULL)
				{
					if(temp_count == 0)
					{
						string_token = strtok(NULL," ");
						strncpy(server[server_count].server_name,string_token,strlen(string_token));
					}
					else if(temp_count == 1)
					{
						string_token = strtok(NULL,":");
					}
					else if(temp_count == 2)
					{
						string_token = strtok(NULL,"\n");
						server[server_count].server_port = atoi(string_token);
					}
					else
					{
						string_token = NULL;
					}
					temp_count++;
				}
				server_count++;
			}
			else if(strstr(line, "Username:") != NULL)
			{
				char *string_token = strtok(line," ");
				bzero(username,sizeof(username));
				
				string_token = strtok(NULL,"\n");
				strncpy(username,string_token,strlen(string_token));
				
			}
			else if(strstr(line, "Password:") != NULL)
			{
				char *string_token = strtok(line," ");
				bzero(password,sizeof(password));
					
				string_token = strtok(NULL,"\n");
				strncpy(password,string_token,strlen(string_token));
				
			}
			
		}
		fclose(fp);
		return 1;
	}
}

void connect_to_server(int server_number)
{
	printf("In connect_to_server function\n");
	
	if((sockets[server_number] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Error opening socket for server:%d",server_number);
	}

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server[server_number].server_port);
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(sockets[server_number],(struct sockaddr *) &server_address, sizeof(server_address)) < 0)
	{
		printf("Error connecting for server:%d",server_number);
	}
	
}

void send_command_DFS(CommandStruct *inf)
{
	printf("In send_command_DFS function\n");
	for(int index =0; index<4; index++)
	{
		connect_to_server(index);
		send(sockets[index], inf, sizeof(*inf),MSG_NOSIGNAL);
		//close(sockets[index]);
	}
}

char * recvFromDFS(int server_number)
{
	printf("In recvFromDFS function\n");
	bzero(receive_buffer,sizeof(receive_buffer));
	
	MessageStruct *temp = malloc(sizeof(MessageStruct));
	recv(sockets[server_number], temp, sizeof(*temp), 0);
	strncpy(receive_buffer,temp->message,temp->message_length);
	free(temp);
	return receive_buffer;
}

/* Definition of 'client_get_file' function */
void client_get_file(char *file_name)
{
	printf("In client_get_file function with file_name:%s\n",file_name);
	

}


/* Definition of 'client_put_file' function */
int client_put_file(char *file_name)
{
	printf("In client_put_file function with file_name:%s\n",file_name);
	int hash_packetcombo[4][4][2] = {
								{
									{3,0},
									{0,1},
									{1,2},
									{2,3}
								},
								{
									{0,1},
									{1,2},
									{2,3},
									{3,0}
								},
								{
									{1,2},
									{2,3},
									{3,0},
									{0,1}
								},
								{
									{2,3},
									{3,0},
									{0,1},
									{1,2}
								}
							};  

	char file_exist_buffer[BUFFER];
	FILE *putfile,*mdfile;	
	putfile = fopen(file_name,"r");
	if(putfile == NULL)
	{
		printf("Sending file doesn't exist information to server\n");
		
		bzero(file_exist_buffer,sizeof(file_exist_buffer));
		strcpy(file_exist_buffer,"File does not exist");
		printf("File exist confirmation message : %s\n",file_exist_buffer);
		for(int index =0; index<4; index++)
		{
			MessageStruct *temp = malloc(sizeof(MessageStruct));
			temp->message_length = strlen(file_exist_buffer);
			strncpy(temp->message,file_exist_buffer,strlen(file_exist_buffer));
			send(sockets[index], temp, (sizeof(*temp)),MSG_NOSIGNAL);
			free(temp);
		}
		return(1);
		
	}
	else
	{
		printf("Sending file exist's information to server\n");
		
		bzero(file_exist_buffer,sizeof(file_exist_buffer));
		strcpy(file_exist_buffer,"File exists");
		printf("File exist confirmation message : %s\n",file_exist_buffer);
		for(int index =0; index<4; index++)
		{
			MessageStruct *temp = malloc(sizeof(MessageStruct));
			temp->message_length = strlen(file_exist_buffer);
			strncpy(temp->message,file_exist_buffer,strlen(file_exist_buffer));
			send(sockets[index], temp, (sizeof(*temp)),MSG_NOSIGNAL);
			free(temp);
		}
		
		
		/* Now check the md5sum */
		char system_call_buffer[BUFFER];
		bzero(system_call_buffer,sizeof(system_call_buffer));	
		sprintf(system_call_buffer,"md5sum %s >md5file.txt",file_name);
		printf("\nsystem call buffer:%s\n",system_call_buffer);
		system(system_call_buffer);
		if((mdfile = fopen("md5file.txt", "r")) == NULL)
		{
			printf("md5sum file not created\n");
		}
		else
		{
			printf("md5sum file created\n");
			char *temp_buffer = NULL;
			char md5string[32], last_nibble,last_byte[2];
			int value;
			size_t len = 0;

			getline(&temp_buffer, &len, mdfile);
				
			//fgets(temp_buffer, BUFFER, mdfile);			
			strncpy(md5string, temp_buffer, 32);
			last_nibble = md5string[31];
			printf("last_nibble is %c\n", last_nibble);
			if(last_nibble == '0' || last_nibble == '1' || last_nibble == '2' || last_nibble == '3' || last_nibble == '4' || 
				last_nibble == '5' || last_nibble == '6' || last_nibble == '7' || last_nibble == '8' || last_nibble == '9')
			{
				value = atoi(&last_nibble);
			}
			else
			{
				sprintf(last_byte, "%x", last_nibble);
				value = atoi(last_byte) - 51;
			}
			printf("value is:%d\n",value);
			value = value%4;
			printf("hash value of the pair to be sent:%d\n",value);
		
			/* Finding the file size */
			int filesize,file_chunk;
			char packet_message[BUFFER];
			fseek(putfile, 0, SEEK_END);
			filesize = ftell(putfile);
			fseek(putfile, 0, SEEK_SET);	
			file_chunk = (filesize/4)+1;
			printf("Filesize:%d,file chunk size:%d\n",filesize,file_chunk);
				
			for(int packet_count=0;packet_count<4;packet_count++)
			{
				/* Sending packet 0 */	
				if(packet_count == 0)
				{
					printf("Sending packet 0\n");
					int size_check = 0;
					int bytes_read;
					char datagram_message[BUFFER];
					int chunk_sent = file_chunk;
					bzero(packet_message,sizeof(packet_message));
					sprintf(packet_message,"%d %d",packet_count,chunk_sent);
					for(int x=0;x<2;x++)
					{
						MessageStruct *temp = malloc(sizeof(MessageStruct));
						temp->message_length = strlen(packet_message);
						strncpy(temp->message,packet_message,strlen(packet_message));
						send(sockets[hash_packetcombo[value][packet_count][x]], temp, (sizeof(*temp)),MSG_NOSIGNAL);
						free(temp);
					}
					/* Sending content */
					while(size_check < chunk_sent)
					{
						bzero(datagram_message,sizeof(datagram_message));
						bytes_read = fread(datagram_message,sizeof(char),BUFFER,putfile);
						for(int x=0;x<2;x++)
						{
							MessageStruct *temp = malloc(sizeof(MessageStruct));
							temp->message_length = strlen(datagram_message);
							strncpy(temp->message,datagram_message,strlen(datagram_message));
							send(sockets[hash_packetcombo[value][packet_count][x]], temp, (sizeof(*temp)),MSG_NOSIGNAL);
							free(temp);
						}
						size_check = size_check + bytes_read;
					}				
				}
				/* Sending packet 1 */	
				else if(packet_count == 1)
				{
					printf("Sending packet 1\n");
					int size_check = 0;
					int bytes_read;
					char datagram_message[BUFFER];
					int chunk_sent = file_chunk;
					fseek(putfile, chunk_sent, SEEK_SET);
					bzero(packet_message,sizeof(packet_message));
					sprintf(packet_message,"%d %d",packet_count,chunk_sent);
					for(int x=0;x<2;x++)
					{
						MessageStruct *temp = malloc(sizeof(MessageStruct));
						temp->message_length = strlen(packet_message);
						strncpy(temp->message,packet_message,strlen(packet_message));
						send(sockets[hash_packetcombo[value][packet_count][x]], temp, (sizeof(*temp)),MSG_NOSIGNAL);
						free(temp);
					}
					/* Sending content */
					while(size_check < chunk_sent)
					{
						bzero(datagram_message,sizeof(datagram_message));
						bytes_read = fread(datagram_message,sizeof(char),BUFFER,putfile);
						for(int x=0;x<2;x++)
						{
							MessageStruct *temp = malloc(sizeof(MessageStruct));
							temp->message_length = strlen(datagram_message);
							strncpy(temp->message,datagram_message,strlen(datagram_message));
							send(sockets[hash_packetcombo[value][packet_count][x]],temp, (sizeof(*temp)),MSG_NOSIGNAL);
							free(temp);
						}
						size_check = size_check + bytes_read;
					}				
					
				}	
				/* Sending packet 2 */	
				else if(packet_count == 2)
				{
					printf("Sending packet 2\n");
					int size_check = 0;
					int bytes_read;
					char datagram_message[BUFFER];
					int chunk_sent = file_chunk;
					fseek(putfile, 2*chunk_sent, SEEK_SET);
					bzero(packet_message,sizeof(packet_message));
					sprintf(packet_message,"%d %d",packet_count,chunk_sent);
					for(int x=0;x<2;x++)
					{
						MessageStruct *temp = malloc(sizeof(MessageStruct));
						temp->message_length = strlen(packet_message);
						strncpy(temp->message,packet_message,strlen(packet_message));
						send(sockets[hash_packetcombo[value][packet_count][x]], temp, (sizeof(*temp)),MSG_NOSIGNAL);
						free(temp);
					}
					/* Sending content */
					while(size_check < chunk_sent)
					{
						bzero(datagram_message,sizeof(datagram_message));
						bytes_read = fread(datagram_message,sizeof(char),BUFFER,putfile);
						for(int x=0;x<2;x++)
						{
							MessageStruct *temp = malloc(sizeof(MessageStruct));
							temp->message_length = strlen(datagram_message);
							strncpy(temp->message,datagram_message,strlen(datagram_message));
							send(sockets[hash_packetcombo[value][packet_count][x]], temp, (sizeof(*temp)),MSG_NOSIGNAL);
							free(temp);
						}
						size_check = size_check + bytes_read;
					}				
					
				}
				/* Sending packet 3 */	
				else
				{
					printf("Sending packet 3\n");
					int size_check = 0;
					int bytes_read;
					char datagram_message[BUFFER];
					int chunk_sent = filesize - 3*file_chunk;
					bzero(packet_message,sizeof(packet_message));
					sprintf(packet_message,"%d %d",packet_count,chunk_sent);
					for(int x=0;x<2;x++)
					{
						MessageStruct *temp = malloc(sizeof(MessageStruct));
						temp->message_length = strlen(packet_message);
						strncpy(temp->message,packet_message,strlen(packet_message));
						send(sockets[hash_packetcombo[value][packet_count][x]], temp, (sizeof(*temp)),MSG_NOSIGNAL);
						free(temp);
					}
					/* Sending content */
					while(size_check < chunk_sent)
					{
						bzero(datagram_message,sizeof(datagram_message));
						bytes_read = fread(datagram_message,sizeof(char),BUFFER,putfile);
						for(int x=0;x<2;x++)
						{
							MessageStruct *temp = malloc(sizeof(MessageStruct));
							temp->message_length = strlen(datagram_message);
							strncpy(temp->message,datagram_message,strlen(datagram_message));
							send(sockets[hash_packetcombo[value][packet_count][x]], temp, (sizeof(*temp)),MSG_NOSIGNAL);
							free(temp);
						}
						size_check = size_check + bytes_read;
					}				
					
				}
			}
			printf("All packets sent\n");	
		}
				
	}
}


/* Definition of 'main' function */
int main(int argc, char * argv[]) 
{
	char *client_config_file;
	int status;
	char operation[BUFFER];
	char temp_operation[BUFFER];
	char *delimiter = " "; 
	char *words,*temp;
	char *final_words,*final_temp;
	

	if(argc != 2)
	{
		printf("Number of command line arguments are incorrect\n");
		exit(1);
	}

	client_config_file = argv[1];
	/* Parse config */
	status = parse_config(client_config_file);
	
	if (status != 1)
	{
		printf("Error while parsing file\n");
		exit(1);
	}	
	else
	{
		printf("File parsed succssesfuly\n\n");
	}
	
	printf("\nconf file information:\n");
	printf("**************************************\n");
	
	/* Print parsed values */
	for(int index =0;index<4;index++)
	{

		printf("server-%d information:\n",index);
		printf("\tserver-%d name:%s\n",index,server[index].server_name);
		printf("\tserver-%d port:%d\n",index,server[index].server_port);	
	}
	printf("\nUsername: %s\n", username);
	printf("Password: %s\n", password);
	printf("**************************************\n");

	/* Loop for getting input from the user for different operation */
	while(1)
	{		
		
		/* Menu for the user */
		printf("\n/********************************************************************/\n");
		printf("You can perform the following operations using this client:\n");
		printf("1) Get File from the server.               [Eg: GET 'file_name']\n");
		printf("2) Put File in the server.                 [Eg: PUT 'file_name']\n");
		printf("4) Get server contents by 'ls' command.    [Eg: LIST]\n");
		printf("6) Exit the server.                        [Eg: EXIT]\n");
		printf("/********************************************************************/\n");

		bzero(operation,sizeof(operation));
		int count = 0,final_count = 0;
	
		/* Getting input from the user */
		printf("\nPlease enter the operation you want to perform:\n");
		scanf(" %[^\n]s",operation);
		printf("\noperation:%s\n",operation);

		/* First sending the command to all the 4 servers */
		CommandStruct command_info;
		strncpy(command_info.command, operation, strlen(operation));
		strncpy(command_info.user_name, username, strlen(username));
		strncpy(command_info.user_password, password, strlen(password));
		send_command_DFS(&command_info);

		printf("Done sending the commands\n");
		

		/* Receive user avaialability from server */
		char *user_availability_buffer;
		for(int receive_count =0; receive_count <4; receive_count++)
		{
			user_availability_buffer = recvFromDFS(receive_count);
			if(strstr(user_availability_buffer,"User is available") != NULL)
			{
				server_user_availability[receive_count] = 1;
			}
			else
			{
				server_user_availability[receive_count] = 0;
			}
		}
		if(server_user_availability[0]==1 && server_user_availability[1]==1 && server_user_availability[2]==1 && server_user_availability[3]==1)
		{
		
			printf("User is available at all the servers\n");
			/* Parsing the received operation */
			strcpy(temp_operation,operation);
			temp = operation;
			words = strtok(temp,delimiter);
			while(words != NULL)
			{
				if(strlen(words) > 0)
				{			
					count++;
				}		
				words = strtok(NULL,delimiter);
			}
			/* Checking the number ofo words in the input string */
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
					char first_word[BUFFER]="",second_word[BUFFER]="";
					final_temp = temp_operation;
					final_words = strtok(final_temp,delimiter);
				
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
						final_words = strtok(NULL,delimiter);
					}
			
				
					/* Calling required functions depending on the command received */
					if(strcmp(first_word,"GET")==0)
					{
						client_get_file(second_word);
					}
					else if(strcmp(first_word,"PUT")==0)
					{
						client_put_file(second_word);
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
			}
		}
		else
		{
			printf("Invalid Username/Password. Please try again.\n");
		}
	}
		
	exit(0);
	
}

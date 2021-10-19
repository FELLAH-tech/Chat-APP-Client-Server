#include <arpa/inet.h>
#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include <assert.h>
#include "msg_struct.h"



int socket_and_connect(char *hostname, char *port) {
	int sock_fd = -1;
	// Création de la socket
	if (-1 == (sock_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}
	printf("Socket created (%d)\n", sock_fd);
	struct addrinfo hints, *res, *tmp;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	int error;
	error = getaddrinfo(hostname, port, &hints, &res);
	if (error) {
		errx(1, "%s", gai_strerror(error));
		exit(EXIT_FAILURE);
	}
	tmp = res;
	while (tmp != NULL) {
		if (tmp->ai_addr->sa_family == AF_INET) {
			struct sockaddr_in *sockin_ptr = (struct sockaddr_in *)tmp->ai_addr;
			u_short port_number = sockin_ptr->sin_port;
			char *ip_str = inet_ntoa(sockin_ptr->sin_addr);
			printf("Address is %s:%hu\n", ip_str, htons(port_number));
			printf("Connecting...");
			fflush(stdout);
			if (-1 == connect(sock_fd, tmp->ai_addr, tmp->ai_addrlen)) {
				perror("Connect");
				exit(EXIT_FAILURE);
			}
			printf("OK\n");
			freeaddrinfo(res);
			return sock_fd;
		}
		tmp = tmp->ai_next;
	}
	freeaddrinfo(res);
	return -1;
}

int Echo_pseudo(int sock_fd,char*buff,struct message msgstruct,struct message* msgstr, int n){


		if ( strcmp(msgstruct.nick_sender,"ok") == 0 ){ // if we receive on ok from server
			printf("[SERVER] : Welcome %s\n", buff);
			strncpy(msgstr->nick_sender, buff, strlen(buff));
			msgstruct.type = ECHO_SEND;

			return ECHO_SEND;
		}
		else { // we need to write again ! 
			printf("[SERVER] : %s\n", buff);
			char test2[MSG_LEN];
			memset(test2, 0, MSG_LEN);
			memset(buff, 0, MSG_LEN);
			while ((buff[n++] = getchar()) != '\n') {} 
			for (int j = 6;j < n;j++){ // to extract the pseudo and verify
						int k = j-6;
						test2[k] = buff[j];
					}
			
			strncpy(msgstruct.infos, test2, strlen(test2));
			strncpy(msgstruct.nick_sender, test2, strlen(test2));
			msgstruct.type = NICKNAME_NEW;
			
			// Sending structure (NICKNAME_NEW)
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return 0;
			}
			// Sending message (NICKNAME_NEW)
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return 0;
			}
		}
        return 0;

}

void extract_data(int from,int to,char* buff,char*test){ // to extract data from command 
	memset(test, 0, MSG_LEN);
	for (int k = from;k < to;k++){
		if (from == 0){
			test[k] = buff[k];
		}
		else{
			int j = k - from;
			test[j] = buff[k];
		}
	}

}

int Echo_message(int sock_fd,char*buff,struct message msgstruct,struct message* msgstr, int n){
    // Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
		// Filling structure
		msgstruct.pld_len = strlen(buff);
        strncpy(msgstruct.nick_sender, msgstr->nick_sender, strlen(msgstr->nick_sender));
        strncpy(msgstruct.infos, "\0", 1);
		char test[MSG_LEN]; // store command
		memset(test, 0, MSG_LEN);
		extract_data(0,6,buff,test);
		if (strcmp(test,"/nick ") == 0){
			msgstruct.type = NICKNAME_NEW;
			// send data to server

			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
			return NICKNAME_NEW;
		}
		if (strcmp(buff,"/quit\n") == 0) { 
			printf("Disconnected...\n");
			// send data to server
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
			free(msgstr);
			return 50;
		}
		if ( strcmp(buff,"/who\n") == 0 ){
			msgstruct.type = NICKNAME_LIST;
			// send data to server
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return NICKNAME_LIST;
		}
		if ( strcmp(test,"/whois") == 0 ){
			msgstruct.type = NICKNAME_INFOS;
			extract_data(7,msgstruct.pld_len,buff,test);
			strcpy(msgstruct.infos,test);
			// send data to server
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return NICKNAME_INFOS;
		}
        memset(test, 0, MSG_LEN);
		extract_data(0,7,buff,test);
		if ( strcmp(test,"/msgall") == 0 ){
			msgstruct.type = BROADCAST_SEND;
			// send data to server
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return BROADCAST_SEND;
		}
        memset(test, 0, MSG_LEN);
		extract_data(0,5,buff,test);
		if ( strcmp(test,"/msg ") == 0 ){
			memset(test, 0, MSG_LEN);
			int k = 5;
			while(buff[k] != ' '){ // extract receiver
					int j = k - 5;
					test[j] = buff[k];
					k++;
				}
			test[k+1] ='\n';
			strcpy(msgstruct.infos,test);
			memset(test, 0, MSG_LEN);
			int i = k+1; 
			while (i<n) // extract the message to send
			{
				int j = i - k-1;
				test[j] = buff[i];
				i++; 
			}
			memset(buff, 0, MSG_LEN);
			strcpy(buff,test);
			msgstruct.type = UNICAST_SEND;	
			// send data to server
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return UNICAST_SEND;
		}
        memset(test, 0, MSG_LEN);
		extract_data(0,8,buff,test);
		if ( strcmp(test,"/create ") == 0 ){
			msgstruct.type = MULTICAST_CREATE;

			memset(test,0,MSG_LEN); // extract channel name
			extract_data(8,msgstruct.pld_len,buff,test);
			strcpy(msgstruct.infos,test);
			// send data to server

			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return MULTICAST_CREATE;
		}
		memset(test, 0, MSG_LEN);
		extract_data(0,13,buff,test);
		if ( strcmp(test,"/channel_list") == 0 ){
			msgstruct.type = MULTICAST_LIST;
			// send data to server

			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return MULTICAST_LIST;
		}
		memset(test, 0, MSG_LEN);
		extract_data(0,6,buff,test);
		if ( strcmp(test,"/join ") == 0 ){
			msgstruct.type = MULTICAST_JOIN;
			memset(test, 0, MSG_LEN);
			extract_data(6,msgstruct.pld_len,buff,test);
			strcpy(msgstruct.infos,test);
			// send data to server
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return MULTICAST_JOIN;
		}  
		printf("Message sent!\n");
        return ECHO_SEND;
}

int send_2_salon(int sock_fd,char*buff,struct message msgstruct,struct message* msgstr,int n){
	// Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent

		strncpy(msgstruct.nick_sender, msgstr->nick_sender, strlen(msgstr->nick_sender));
        strcpy(msgstruct.infos, msgstr->nom_salon);
		msgstruct.type = MULTICAST_SEND;
		msgstruct.pld_len = strlen(buff);

		char test[MSG_LEN];
		memset(test, 0, MSG_LEN);
		extract_data(0,6,buff,test);
		if ( strcmp(test,"/quit ") == 0 ){
			msgstruct.type = MULTICAST_QUIT;

			memset(test, 0, MSG_LEN);
			extract_data(6,msgstruct.pld_len,buff,test);
			strcpy(msgstruct.infos,test); // channel name
			// send data to server

			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return MULTICAST_QUIT;
		}

		memset(test, 0, MSG_LEN);
		extract_data(0,6,buff,test);
		if ( strcmp(test,"/join ") == 0 ){
			msgstruct.type = MULTICAST_JOIN;
			memset(test, 0, MSG_LEN);
			extract_data(6,msgstruct.pld_len,buff,test);
			strcpy(msgstruct.infos,test);
			// send data to server
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return ECHO_SEND;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return ECHO_SEND;
			}
    		return MULTICAST_JOIN;
		} 

		// send data to server
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return MULTICAST_ECHO;
			}
			// Sending message 
			if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
				return 	MULTICAST_ECHO;
			}
		return MULTICAST_ECHO;
}

int main(int argc, char  *argv[]) {
	if (argc != 3) {
		printf("Usage: ./client hostname port_number\n");
		exit(EXIT_FAILURE);
	}
	char *hostname = argv[1];
	char *port = argv[2];

	int sock_fd = -1;
	if (-1 == (sock_fd = socket_and_connect(hostname, port))) {
		printf("Could not create socket and connect properly\n");
		return 1;
	}

	// Declare array of struct pollfd
	int nfds = 2;
	struct pollfd pollfds[nfds];

	// Init first slot with listening socket
	pollfds[0].fd = sock_fd;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;

	pollfds[1].fd = 0;
	pollfds[1].events = POLLIN;
	pollfds[1].revents = 0;


	// Client loop
	int n_active = 0;
    struct message msgstruct;
	char buff[MSG_LEN];
	char test[MSG_LEN];
	int n;
  
    struct message* msgstr = malloc(sizeof(*msgstr)); // pour stocker des infos globals et récupérable (pseduo,nom_salon)
	msgstr->type = NICKNAME_NEW; // initialize the type
	// recv data from server to know the message type
    while (1)
    {
		if ( msgstr->type == 50){ // to get out frome the while when sending /quit
			break;
		}
        
		if (msgstr->type == ECHO_SEND){ // to show that we are waiting for commands
				printf("Message au serveur : \n");
		}
		if (msgstr->type == MULTICAST_ECHO){ // to show that we are waiting for commands to the channel
				printf("Message au salon : \n");
		}
		// Block until new activity detected on existing socket
		if (-1 == (n_active = poll(pollfds, nfds, -1))) {
			perror("Poll");
		}
		
		if (pollfds[0].revents & POLLIN){
			
				// Cleaning memory
				memset(&msgstruct, 0, sizeof(struct message));
				memset(buff, 0, MSG_LEN);

				// recv data from server

				// Receiving structure
				if (recv(pollfds[0].fd, &msgstruct, sizeof(struct message), 0) <= 0) {
					return ECHO_SEND;
				}
				// Receiving message
				if (recv(pollfds[0].fd, buff, msgstruct.pld_len, 0) <= 0) {
					return ECHO_SEND;
				}
				msgstr->type = msgstruct.type;
				switch (msgstr->type)
				{
					case NICKNAME_NEW:
						n = 0;
						msgstr->type = Echo_pseudo(sock_fd,buff,msgstruct,msgstr,n);
						break;
					case NICKNAME_LIST:
						printf("[SERVER] : Users online are :\n %s\n", buff);
						msgstr->type = ECHO_SEND;
						break;
					case NICKNAME_INFOS:
						printf("[SERVER] : %s\n",buff);
						msgstr->type = ECHO_SEND;
						break;
					case BROADCAST_SEND:
						printf(" [Client]: pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
						printf("[%s]: \"%s\"\n",msgstruct.nick_sender, buff);
						msgstr->type = ECHO_SEND;
						break;
					case UNICAST_SEND:
						printf(" [Client]: pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
						printf("[%s]: \"%s\"\n",msgstruct.nick_sender, buff);
						msgstr->type = ECHO_SEND;
						break;
                    case MULTICAST_CREATE:
						printf("[SERVER] : %s\n",buff);
						memset(test,0,MSG_LEN);
						extract_data(38,msgstruct.pld_len-1,buff,test);
						strcpy(msgstr->nom_salon,test);
						msgstr->type = MULTICAST_ECHO; // to send message automatically to the salon created
						break;
					case MULTICAST_LIST:
						printf("[SERVER] : Salons existed are :\n %s\n", buff);
						msgstr->type = ECHO_SEND;
						break;
					case MULTICAST_JOIN:
						printf("[SERVER] : %s\n",buff);
						memset(test,0,MSG_LEN);
						extract_data(29,msgstruct.pld_len-1,buff,test);
						strcpy(msgstr->nom_salon,test);
						msgstr->type = MULTICAST_ECHO;
						break;	
					case MULTICAST_SEND:
						printf("%s[%s]>: %s\n",msgstruct.nick_sender,msgstruct.infos, buff);
						msgstr->type = MULTICAST_ECHO;
						break;	
					case MULTICAST_INFO:
						memset(test,0,MSG_LEN);
						extract_data(0,strlen(msgstruct.infos)-1,msgstruct.infos,test);
						printf("[%s] : %s\n",test,buff);
						msgstr->type = MULTICAST_ECHO;
						break;
					
					default :
						break;
				}
				// Set .revents of listening socket back to default
				pollfds[0].revents = 0;
			}
		else if (msgstr->type == ECHO_SEND || msgstr->type == MULTICAST_ECHO){
				switch (msgstr->type)
				{
					case MULTICAST_ECHO:
						msgstr->type = send_2_salon(sock_fd,buff,msgstruct,msgstr,n);
						break;
					default :
						msgstr->type = Echo_message(sock_fd,buff,msgstruct,msgstr,n);
						break;
				}
			
		}
		
	}
	return 0;
}
#include <arpa/inet.h>
#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common.h"
#include <assert.h>
#include "msg_struct.h"

void write_int_size(int fd, void *ptr) {
	int ret = 0, offset = 0;
	while (offset != sizeof(int)) {
		ret = write(fd, ptr + offset, sizeof(int) - offset);
		if (-1 == ret)
			perror("Writing size");
		offset += ret;
	}
}

int read_int_size(int fd) {
	int read_value = 0;
	int ret = 0, offset = 0;
	while (offset != sizeof(int)) {
		ret = read(fd, (void *)&read_value + offset, sizeof(int) - offset);
		if (-1 == ret)
			perror("Reading size");
		if (0 == ret) {
			printf("Should close connection, read 0 bytes\n");
			close(fd);
			return -1;
		}
		offset += ret;
	}
	return read_value;
}

int read_from_socket(int fd, void *buf, int size) {
	int ret = 0;
	int offset = 0;
	while (offset != size) {
		ret = read(fd, buf + offset, size - offset);
		if (-1 == ret) {
			perror("Reading from client socket");
			exit(EXIT_FAILURE);
		}
		if (0 == ret) {
			printf("Should close connection, read 0 bytes\n");
			close(fd);
			return -1;
			break;
		}
		offset += ret;
	}
	return offset;
}

int write_in_socket(int fd, void *buf, int size) {
	int ret = 0, offset = 0;
	while (offset != size) {
		if (-1 == (ret = write(fd, buf + offset, size - offset))) {
			perror("Writing from client socket");
			return -1;
		}
		offset += ret;
	}
	return offset;
}


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

		// Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		
        
		// Receiving structure
		
		if (recv(sock_fd, &msgstruct, sizeof(struct message), 0) <= 0)  {
			return 0;
		}
		// Receiving message
		if (recv(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
			return 0;
		}
		if ( strcmp(msgstruct.nick_sender,"ok") == 0 ){ // if we receive on ok from server
			printf("[SERVER] : Welcome %s\n", buff);
			strncpy(msgstr->nick_sender, buff, strlen(buff));
			msgstruct.type = ECHO_SEND;
			// Sending structure (NICKNAME_NEW)
			if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
				return 0;
			}

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

int Echo_message(int sock_fd,char*buff,struct message msgstruct,struct message* msgstr, int n){
   
    // Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		printf("Message au serveur: ");
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
		// Filling structure
		msgstruct.pld_len = strlen(buff);
        strncpy(msgstruct.nick_sender, msgstr->nick_sender, strlen(msgstr->nick_sender));
		free(msgstr);

		if ( strcmp(buff,"/who") == 0 ){

    		msgstruct.type = NICKNAME_LIST;
		}
        strncpy(msgstruct.infos, "\0", 1);
        // send data to server

        if (send(sock_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
            return ECHO_SEND;
        }
        // Sending message (ECHO)
        if (send(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
            return ECHO_SEND;
        }
        
		printf("Message sent!\n");
		if (strcmp(buff,"/quit\n") == 0) { 
			printf("Disconnected...\n");
			close(sock_fd);
			return ECHO_SEND;
		}
		// Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);

		// recv data from server

        // Receiving structure
		if (recv(sock_fd, &msgstruct, sizeof(struct message), 0) <= 0) {
			return ECHO_SEND;
		}
		// Receiving message
		if (recv(sock_fd, buff, msgstruct.pld_len, 0) <= 0) {
			return ECHO_SEND;
		}
        
        printf(" [Client]: pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		printf("[Client]: Server says \"%s\" (%d bytes)\n", buff, msgstruct.pld_len);
        return ECHO_SEND;
}

int Echo_users(char*buff,struct message* msgstr){
		printf("[SERVER] : Users online are :\n %s\n", buff);
		return ECHO_SEND;
    
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
    struct message msgstruct;
	char buff[MSG_LEN];
	int n;
  
    struct message* msgstr = malloc(sizeof(*msgstr)); // pour stocker le pseudo
	// recv data from server to know the message type
    while (1)
    {
        
    static enum msg_type test = NICKNAME_NEW;
	printf("test %d\n",test);
    switch (test)
	{
		case NICKNAME_NEW:
			n = 0;
			test = Echo_pseudo(sock_fd,buff,msgstruct,msgstr,n);
			break;
		
		case NICKNAME_LIST:
			test = Echo_users(buff,msgstr);
			break;
		default :
			Echo_message(sock_fd,buff,msgstruct,msgstr,n);
			break;
    }
    
	
    }
    
	return 0;
}
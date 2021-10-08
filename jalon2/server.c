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
#include "common.h"
#include <assert.h>
#include "msg_struct.h"


// inistialiser le pointeur de contrôl de la chaine
Client_ancien *initialisation()
{
    Client_ancien *Client_ancien = malloc(sizeof(*Client_ancien));
	Client_info *Client_init = malloc(sizeof(*Client_init));
    if (Client_ancien == NULL || Client_init == NULL)
    {
        exit(EXIT_FAILURE);
    }

	Client_init->client_id = 0;
    Client_init->client_fd = 0;
 	Client_init->local_address = ""; 
	Client_init->pseudo = ""; 
	Client_init->sin_port= 0;
	Client_init->suivant = NULL;
    Client_ancien->premier = Client_init;

    return Client_ancien;
}

void insertion(Client_ancien *Client_ancien,int client_id,int client_fd,in_port_t sin_port,char* local_address,char* pseudo) // une file de client
{
	// création du nouveau client
    Client_info *nouveau_client = malloc(sizeof(*nouveau_client));
    if (nouveau_client == NULL)
    {
        exit(EXIT_FAILURE);
    }

	nouveau_client->client_id = client_id;
    nouveau_client->client_fd = client_fd;
	nouveau_client->local_address = local_address; 
	nouveau_client->pseudo = pseudo; 
	nouveau_client->sin_port= sin_port;
    
    
	// insertion du client dans la chaine
		nouveau_client->suivant = Client_ancien->premier;
        Client_ancien->premier = nouveau_client;
		printf("Connection succeeded and client[%i] info stored !\n Client used address %s:%hu \n",client_id, local_address, sin_port);
		printf("client_fd = %d\n", client_fd);
    
}

void supprimer(Client_ancien *Client_ancien, int Client_fd )
{
    assert(Client_ancien);
	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		while (Client_actuel->suivant != NULL)
		{	
			static int c = 0;
			if (Client_actuel->client_fd == Client_fd && c == 0 && Client_actuel->suivant->suivant != NULL){ // si le 1er client de la liste a supprimé
				Client_ancien->premier = Client_actuel->suivant;
				free(Client_actuel);
				printf("Client on socket %d Deleted from storage\n",Client_fd);
				c++;
				break;
			}
			else if (Client_actuel->suivant->client_fd == Client_fd){
				Client_info *asupprimer = Client_actuel->suivant;
				Client_actuel->suivant = Client_actuel->suivant->suivant;
				free(asupprimer);
				printf("Client on socket %d Deleted from storage\n",Client_fd);
				break;
			}
			else if (Client_actuel->client_fd == Client_fd && Client_actuel->suivant->suivant == NULL){
				Client_info *asupprimer = Client_actuel;
				Client_info *asupprimer2 = Client_actuel->suivant;
				free(asupprimer);
				free(asupprimer2);
				free (Client_ancien);
				printf("Client on socket %d Deleted from storage\n",Client_fd);
				printf("Storage is empty ! \n");
				break;
			}
			Client_actuel->suivant = Client_actuel->suivant->suivant;
		}
		
	}
	
}

// check if there is a same pseudo before
int find_pseudo(Client_ancien *Client_ancien, char* pseudo )
{
    assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		while (Client_actuel != NULL)
		{	
			if ( strcmp(pseudo,Client_actuel->pseudo) == 0){
				return 1;
			}
			Client_actuel = Client_actuel->suivant;
		}
		
	}
	return 0;
	
}

// show users online
char* find_users(Client_ancien *Client_ancien,char*buff)
{
    assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		
		while (Client_actuel->suivant != NULL)
		{	
			buff = strcat(buff,"-");
			buff = strcat(buff,Client_actuel->pseudo);
			buff = strcat(buff,"\n");
			Client_actuel = Client_actuel->suivant->suivant;
		}
		return buff;
		
	}
	return buff;
	
}
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

int socket_listen_and_bind(char *port) {
	int listen_fd = -1;
	if (-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}
	printf("Listen socket descriptor %d\n", listen_fd);

	int yes = 1;
	if (-1 == setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	struct addrinfo indices;
	memset(&indices, 0, sizeof(struct addrinfo));
	indices.ai_family = AF_INET;
	indices.ai_socktype = SOCK_STREAM; //TCP
	indices.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	struct addrinfo *res, *tmp;

	int err = 0;
	if (0 != (err = getaddrinfo(NULL, port, &indices, &res))) {
		errx(1, "%s", gai_strerror(err));
	}

	tmp = res;
	while (tmp != NULL) {
		if (tmp->ai_family == AF_INET) {
			struct sockaddr_in *sockptr = (struct sockaddr_in *)(tmp->ai_addr);
			struct in_addr local_address = sockptr->sin_addr;
			printf("Binding to %s on port %hd\n",inet_ntoa(local_address),ntohs(sockptr->sin_port));
			

			if (-1 == bind(listen_fd, tmp->ai_addr, tmp->ai_addrlen)) {
				perror("Binding");
			}
			if (-1 == listen(listen_fd, MSG_LEN)) {
				perror("Listen");
			}
			freeaddrinfo(res);
			return listen_fd;
		}
		tmp = tmp->ai_next;
		
	}
	freeaddrinfo(res);
	return listen_fd;
}
char * Echo_chek_pseudo(int pollfds,char*buff,struct message msgstruct,Client_ancien* Client_ancien){

    static int c = 0;
	if ( c == 0 ){
		// Filling structure
		c = 1;
		return "please login with /nick <your pseudo>";
	}
	else {
		int block = 0; // to control the input of pseudo
		char test2[MSG_LEN]; // extract pseudo
		while ( block == 0){
			char test[MSG_LEN]; // extract command
			memset(test2, 0, MSG_LEN);
			memset(test, 0, MSG_LEN);
			// test if the command /nick was inputed right
			for (int k = 0;k < 6;k++){
				test[k] = buff[k];
			}
			
			if (strcmp(test,"/nick ") != 0){
				return "please login with /nick <your pseudo>";
			}
			// test the pseudo
				int block2 = 0;// to control if there is a space in the pseudo
				int block3 = 0;// to control if there is a special caracter in the pseudo
				int block4 = 0;// to control if there is something

				for (int j = 6;j < strlen(buff);j++){ // to extract the pseudo and verify
					int k = j-6;
					test2[k] = buff[j];
					if (test2[k] == ' '){
						block2 = 1; // to control if there is a space in the pseudo
					}
					else if ( ((int)test2[k] > 32 && (int)test2[k] < 48) || ((int)test2[k] > 57 && (int)test2[k] < 65) || ((int)test2[k] > 90 && (int)test2[k] < 97) || ((int)test2[k] > 122 && (int)test2[k] < 127)){
						block3 = 1; // for special caracter
					}
					else if (test2[0] == '\n'){
					block4 = 1; // to control if there is nothing
					}
				
				} 
				//display input errors
				if ( block2 == 1){
					return "[SERVER] : Please Repeat -> no space on your pseudo ^^\n";
					
				}
				else if ( strlen(test2) > 10){
					return "[SERVER] : Please Repeat -> pseudo length must not exceed 10 caracters :)\n";
				}
				else if (block3 == 1){
					return "[SERVER] : Please Repeat -> no special caracter on your pseudo ^^\n";
				
				}
				else if (block4 == 1){
					return "[SERVER] : Please Repeat -> no pseudo founded :/ \n";
			
				}
				else if (1 == (find_pseudo(Client_ancien,msgstruct.infos))) {
					return "[SERVER] : Please Repeat -> pseudo already used :/ \n";
				
				}
				else{
				// everything is okey
				block = 1;
				}
			} 
			
	}
	return "ok";		     
}
void set_msgtructure_info(struct message *msgstruct,char* str){
	strncpy(msgstruct->infos, str, strlen(str));
}

int Echo_for_pseudo(int pollfds,char*buff,struct message msgstruct,struct message* msgstr,Client_ancien* Client_ancien){
    // request for login
                //cleaning memory
                memset(&msgstruct, 0, sizeof(struct message));
				memset(buff, 0, MSG_LEN);
                
                strncpy(buff,Echo_chek_pseudo(pollfds,buff,msgstruct,Client_ancien),strlen(Echo_chek_pseudo(pollfds,buff,msgstruct,Client_ancien)));
				
				msgstruct.type = NICKNAME_NEW;
				strncpy(msgstruct.infos, "\0", 1);
				msgstruct.pld_len = strlen(buff);

                // send data to server
                //Sending structure
                if (send(pollfds, &msgstruct, sizeof(msgstruct), 0) <= 0) {
                    return 0;
                }
                // Sending message (ECHO)
                if (send(pollfds, buff, msgstruct.pld_len, 0) <= 0) {
                    return 0;
                }
                printf("%s\n",buff);
                printf("Waiting for login...\n");

				do 
					{
					//cleaning memory
					memset(&msgstruct, 0, sizeof(struct message));
					memset(buff, 0, MSG_LEN);

					// read data from socket
					// Receiving structure
					if (recv(pollfds, &msgstruct, sizeof(struct message), 0) <= 0) {
						return 0;
					}
					// Receiving message
					if (recv(pollfds, buff, msgstruct.pld_len, 0) <= 0) {
						return 0;
					}

					
					strncpy(buff,Echo_chek_pseudo(pollfds,buff,msgstruct,Client_ancien),strlen(Echo_chek_pseudo(pollfds,buff,msgstruct,Client_ancien)));
					if ( buff[0] == 'o' && buff[1] == 'k'){ // si buff = 'ok'
						char test3[MSG_LEN];
						memset(test3, 0, MSG_LEN);
						msgstruct.type = NICKNAME_NEW;
						int taille = strlen(buff);
						for (int i =6;i<taille;i++){
							int k = i-6;
							test3[k] = buff[i];
						}
						strncpy(msgstruct.nick_sender, "ok", 3);
						strncpy(msgstr->infos, test3, strlen(test3));
						printf("info = %s",msgstruct.infos);
						// Sending structure 
						if (send(pollfds, &msgstruct, sizeof(msgstruct), 0) <= 0) {
							return 0;
						}
						// Sending message 
						if (send(pollfds, test3, strlen(test3), 0) <= 0) {
							return 0;
						}
						return 0;
					}
					else {
						msgstruct.type = NICKNAME_NEW;
						strncpy(msgstruct.infos, "\0", 1);
						msgstruct.pld_len = strlen(buff);
						
						// Sending structure (ECHO)
						if (send(pollfds, &msgstruct, sizeof(msgstruct), 0) <= 0) {
							return 0;
						}
						// Sending message (ECHO)
						if (send(pollfds, buff, msgstruct.pld_len, 0) <= 0) {
							return 0;
						}
						
	
					}
				}while ( buff[0] != 'o' && buff[1] != 'k');
                return 0;
			}


int server(int listen_fd) {

	// Declare array of struct pollfd
	int nfds = 10;
	struct pollfd pollfds[nfds];

	// Init first slot with listening socket
	pollfds[0].fd = listen_fd;
	pollfds[0].events = POLLIN;
	pollfds[0].revents = 0;
	// Init remaining slot to default values
	for (int i = 1; i < nfds; i++) {
		pollfds[i].fd = -1;
		pollfds[i].events = 0;
		pollfds[i].revents = 0;
	}
	//Init le pointeur vers la chaine
	Client_ancien *Client_ancien = initialisation();
	// server loop
	int n_active = 0;
	struct message msgstruct;
	char buff[MSG_LEN];
	while (1) {

		// Block until new activity detected on existing socket
		if (-1 == (n_active = poll(pollfds, nfds, -1))) {
			perror("Poll");
		}
		printf("[SERVER] : %d active socket\n", n_active);
	
	
		// Iterate on the array of monitored struct pollfd
		for (int i = 0; i < nfds; i++) {

			// If listening socket is active => accept new incoming connection
			if (pollfds[i].fd == listen_fd && pollfds[i].revents & POLLIN) {
				// accept new connection and retrieve new socket file descriptor
				struct sockaddr client_addr;
				socklen_t size = sizeof(client_addr);
				int client_fd;
				if (-1 == (client_fd = accept(listen_fd, &client_addr, &size))) {
					perror("Accept");
				}

			// display client connection information
				struct sockaddr_in *sockptr = (struct sockaddr_in *)(&client_addr);
				struct in_addr client_address = sockptr->sin_addr;
				struct message* msgstr = malloc(sizeof(*msgstr)); // pour stocker le pseudo

                Echo_for_pseudo(client_fd,buff,msgstruct,msgstr,Client_ancien);
                int static c = 1;
				insertion(Client_ancien,c++,client_fd, ntohs(sockptr->sin_port),inet_ntoa(client_address),msgstr->infos);
				free(msgstr);

				
					
				// store new file descriptor in available slot in the array of struct pollfd set .events to POLLIN
				for (int j = 0; j < nfds; j++) {
					if (pollfds[j].fd == -1) {
						pollfds[j].fd = client_fd;
						pollfds[j].events = POLLIN;
						break;
					}
				}

				// Set .revents of listening socket back to default
				pollfds[i].revents = 0;

			} else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLHUP) { // If a socket previously created to communicate with a client detects a disconnection from the client side
				// display message on terminal
				printf("[SERVER] : Client[%i] on socket %d has disconnected from server\n", i,pollfds[i].fd);
				
				// free memory
				supprimer(Client_ancien,pollfds[i].fd);
				// Close socket and set struct pollfd back to default
				close(pollfds[i].fd);
				pollfds[i].events = 0;
				pollfds[i].revents = 0;
			} else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLIN) { // If a socket different from the listening socket is active
				
                //cleaning memory
        		memset(&msgstruct, 0, sizeof(struct message));
				memset(buff, 0, MSG_LEN);

			// read data from socket
				// Receiving structure
                if (recv(pollfds[i].fd , &msgstruct, sizeof(struct message), 0) <= 0) {
                    return 0;
                }
                // Receiving message
                if (recv(pollfds[i].fd , buff, msgstruct.pld_len, 0) <= 0) {
                    return 0;
                }

                printf("[SERVER]: pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
				if (strcmp(buff,"/quit\n") == 0) { // If a socket previously created to communicate with a client detects a disconnection from the client side
					// display message on terminal
					printf("[SERVER] : Client[%i] on socket %d has disconnected from server\n", i,pollfds[i].fd);
					
					// free memory
					supprimer(Client_ancien,pollfds[i].fd);
					// Close socket and set struct pollfd back to default
					close(pollfds[i].fd);
					pollfds[i].fd = -1;
					pollfds[i].events = 0;
					pollfds[i].revents = 0;
					continue;
				}

				else {
				printf("[SERVER]: Client[%i] on socket (%d) says \"%s\"\n", i,pollfds[i].fd, buff);
				
				if (strcmp(buff,"/who\n") == 0){// to show client online users
					memset(buff, 0, MSG_LEN);
					strncpy(buff,find_users(Client_ancien,buff),MSG_LEN);
					msgstruct.type = NICKNAME_LIST;
				}
			// send back data to client
                // Sending structure (ECHO)
                if (send(pollfds[i].fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
                    return 0;
                }
                // Sending message (ECHO)
                if (send(pollfds[i].fd, buff, msgstruct.pld_len, 0) <= 0) {
                    return 0;;
                }
				// Set activity detected back to default
				pollfds[i].revents = 0;
				}
			}
		}
	}
    return 0;

}

int main(int argc, char *argv[]) {

	// Test argc
	if (argc != 2) {
		printf("Usage: ./server port_number\n");
		exit(EXIT_FAILURE);
	}

	// Create listening socket
	char *port = argv[1];
	int listen_fd = -1;
	if (-1 == (listen_fd = socket_listen_and_bind(port))) {
		printf("Could not create, bind and listen properly\n");
		return 1;
	}
	// Handle new connections and existing ones
	server(listen_fd);

	return 0;
}
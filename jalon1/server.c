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



typedef struct Client_info Client_info;
struct Client_info
{
	int client_id;
	int client_fd; 
    in_port_t sin_port;
	char* local_address;
    Client_info *suivant;
	
};

typedef struct Client_ancien Client_ancien;
struct Client_ancien
{
    Client_info *premier;
};

// inistialiser le pointeur vers la chaine
Client_ancien *initialisation()
{
    Client_ancien *Client_ancien = malloc(sizeof(*Client_ancien));

    if (Client_ancien == NULL)
    {
        exit(EXIT_FAILURE);
    }

    Client_ancien->premier = NULL;

    return Client_ancien;
}

void insertion(Client_ancien *Client_ancien,int client_id,int client_fd,in_port_t sin_port,char* local_address) // une file de client
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
	nouveau_client->sin_port= sin_port;
    nouveau_client->suivant = NULL;
    
    
	// insertion du client dans la chaine
    if (Client_ancien->premier != NULL) /* La chaine contient au moins un élément */
    {
        /* On se positionne à la fin de la chaine*/
        Client_info *Actuel_client = Client_ancien->premier;
        while (Actuel_client->suivant != NULL)
        {
            Actuel_client = Actuel_client->suivant;
        }		
        Actuel_client->suivant = nouveau_client;
		printf("Connection succeeded and client[%i] info stored !\n Client used address %s:%hu \n",client_id, local_address, sin_port);
		printf("client_fd = %d\n", client_fd);
    }
    else /* la chaine est vide */
    {
        Client_ancien->premier = nouveau_client;
		printf("Connection succeeded and client[%i] info stored !\n Client used address %s:%hu \n",client_id, local_address, sin_port);
		printf("client_fd = %d\n", client_fd);
    }
	
}

void supprimer(Client_ancien *Client_ancien)
{
    assert(Client_ancien);

    /* On vérifie si la chaine n'est pas vide */
    if (Client_ancien->premier != NULL)
    {
        Client_info *Client_info = Client_ancien->premier;
        Client_ancien->premier = Client_info->suivant;
        free(Client_info);
    }

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
			return listen_fd;
		}
		tmp = tmp->ai_next;
		
	}
	freeaddrinfo(res);
	return listen_fd;
}

void server(int listen_fd) {

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
				int static c = 1;
				insertion(Client_ancien,c++,client_fd, ntohs(sockptr->sin_port),inet_ntoa(client_address));
				// store new file descriptor in available slot in the array of struct pollfd set .events to POLLIN
				for (int j = i; j < nfds; j++) {
					if (pollfds[j].fd == -1) {
						pollfds[j].fd = client_fd;
						pollfds[j].events = POLLIN;
						break;
					}
				}

				// Set .revents of listening socket back to default
				pollfds[i].revents = 0;
				// free the memory

			} else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLHUP) { // If a socket previously created to communicate with a client detects a disconnection from the client side
				// display message on terminal
				printf("[SERVER] : Client[%i] on socket %d has disconnected from server\n", i,pollfds[i].fd);
				// Close socket and set struct pollfd back to default
				close(pollfds[i].fd);
				pollfds[i].events = 0;
				pollfds[i].revents = 0;
			} else if (pollfds[i].fd != listen_fd && pollfds[i].revents & POLLIN) { // If a socket different from the listening socket is active
				// read data from socket
				int size = 0;
				size = read_int_size(pollfds[i].fd);
				char str[size];
				read_from_socket(pollfds[i].fd, str, size);

				if (strcmp(str,"/quit\n") == 0) { // If a socket previously created to communicate with a client detects a disconnection from the client side
					// display message on terminal
					printf("[SERVER] : Client[%i] on socket %d has disconnected from server\n", i,pollfds[i].fd);
					// Close socket and set struct pollfd back to default
					close(pollfds[i].fd);
					pollfds[i].fd = -1;
					pollfds[i].events = 0;
					pollfds[i].revents = 0;
					continue;
				}
				else {
				printf("[SERVER]: Client[%i] on socket (%d) says \"%s\"\n", i,pollfds[i].fd, str);

				// send back data to client
				write_int_size(pollfds[i].fd, (void *)&size);
				write_in_socket(pollfds[i].fd, str, size);
				
				// Set activity detected back to default
				pollfds[i].revents = 0;
				}
			}
		}
	}
	// free memory
	supprimer(Client_ancien);
	free(Client_ancien);
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
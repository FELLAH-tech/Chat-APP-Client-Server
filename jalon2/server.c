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

void insertion(Client_ancien *Client_ancien,int client_id,int client_fd,in_port_t sin_port,char* local_address,char* pseudo,struct tm instant) // une file de client
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
	nouveau_client->instant= instant;
    
    
	// insertion du client dans la chaine
		nouveau_client->suivant = Client_ancien->premier;
        Client_ancien->premier = nouveau_client;
		printf("Connection succeeded and client[%i] info stored !\n %s used address %s:%hu \n",client_id,Client_ancien->premier->pseudo, local_address, sin_port);
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
// check if there is a same pseudo before
int find_pseudo(Client_ancien *Client_ancien, char* pseudo )
{
    assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		while (Client_actuel->suivant != NULL)
		{	
			char test[MSG_LEN];
			memset(test,0,MSG_LEN);
			extract_data(0,strlen(pseudo)-1,pseudo,test); // delete the \n
			if ( strcmp(test,Client_actuel->pseudo) == 0){
				return 1;
			}
			Client_actuel = Client_actuel->suivant;
		}
		
	}
	return 0;
	
}

// show users online
char* echo_users(Client_ancien *Client_ancien,char*buff)
{
    assert(Client_ancien);
	

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		
		while (Client_actuel->suivant != NULL)
		{	
			printf("user : %s\n",Client_actuel->pseudo);
			buff = strcat(buff,"-");
			buff = strcat(buff,Client_actuel->pseudo);
			buff = strcat(buff,"\n");
			Client_actuel = Client_actuel->suivant;
		}
	}
	return buff;
}

// change pseudo
int change_pseudo(Client_ancien *Client_ancien,char*buff,int client_fd)
{
    assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		
		while (Client_actuel->suivant != NULL)
		{	
			if ( Client_actuel->client_fd == client_fd){
				printf("Pseudo changed ! from : %s to %s\n",Client_actuel->pseudo,buff);
				fflush(stdout);
				strncpy(Client_actuel->pseudo,buff,strlen(buff));
				return 0;
			}
			Client_actuel = Client_actuel->suivant;
		}
		return 0;
		
	}
	return 0;
	
}

// user info
char* user_info(Client_ancien *Client_ancien,char*buff)
{
    assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		
		while (Client_actuel->suivant != NULL)
		{	
			char test[MSG_LEN];
			memset(test,0,MSG_LEN);
			extract_data(0,strlen(buff)-1,buff,test); // delete the \n
			if ( strcmp(test,Client_actuel->pseudo) == 0){
				char text[20];
				memset(text,0,20);

				char test2[MSG_LEN];
				memset(test2,0,MSG_LEN);

				for (int j = 0;j < strlen(buff)-1;j++){ // to extract the pseudo and verify
					test2[j] = buff[j];
				}
				memset(buff,0,MSG_LEN);
				buff = strcat(test2," connected since : ");
				
				sprintf(text, "%d", Client_actuel->instant.tm_year+1900);
				buff = strcat(buff,text);
				buff = strcat(buff,"/");
				memset(text,0,20);
				sprintf(text, "%d", Client_actuel->instant.tm_mon+1);
				buff = strcat(buff,text);
				buff = strcat(buff,"/");
				memset(text,0,20);
				sprintf(text, "%d", Client_actuel->instant.tm_mday);
				buff = strcat(buff,text);
				buff = strcat(buff,"@");
				memset(text,0,20);
				sprintf(text, "%d", Client_actuel->instant.tm_hour);
				buff = strcat(buff,text);
				buff = strcat(buff,":");
				memset(text,0,20);
				sprintf(text, "%d", Client_actuel->instant.tm_min);
				buff = strcat(buff,text);
				buff = strcat(buff," with IP address : ");
				buff = strcat(buff,Client_actuel->local_address);
				buff = strcat(buff," and port number : ");
				memset(text,0,20);
				sprintf(text, "%hu", Client_actuel->sin_port);
				buff = strcat(buff,text);
				return buff;
			}
			Client_actuel = Client_actuel->suivant;
		}
		return "No User with this pseudo try again :)";
		
	}
	return "No User with this pseudo try again :)";
	
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
			extract_data(0,6,buff,test);
			
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
						extract_data(6,taille-1,buff,test3);
						strncpy(msgstruct.nick_sender, "ok", 3); // to be detected in client code
						memset(msgstr->infos, 0, 128);
						strncpy(msgstr->infos, test3, strlen(test3));// show the pseudo in server terminal
						printf("New pseudo : %s",msgstr->infos);
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
				}while ( buff[0] != 'o' && buff[1] != 'k'); // if we have the ok from the check function
                return 0;
			}


int brodcast_send(Client_ancien* Client_ancien,char *buff,struct message msgstruct){

	assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		
		while (Client_actuel->suivant != NULL)
		{	
			if ( (strcmp(msgstruct.nick_sender,Client_actuel->pseudo)) != 0){ // to avoid receiving the message sent
			// send back data to client
				// Sending structure (ECHO)
				char test2[MSG_LEN];
				memset(test2,0,MSG_LEN);
				msgstruct.pld_len = strlen(buff)-1;
				extract_data(0,msgstruct.pld_len,buff,test2);// delete the \n
				msgstruct.pld_len = strlen(buff)-1;
				if (send(Client_actuel->client_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
					return 0;
				}
				// Sending message (ECHO)
				if (send(Client_actuel->client_fd, test2, msgstruct.pld_len, 0) <= 0) {
					return 0;
				}
			}
			
			Client_actuel = Client_actuel->suivant;
		}
		
	}
	return 0;
}

char* unicast_send(Client_ancien* Client_ancien,char *buff,struct message msgstruct){

	assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		
		while (Client_actuel->suivant != NULL)
		{	
			if ( (strcmp(msgstruct.infos,Client_actuel->pseudo)) == 0){ 
			// send back data to client
				printf("pass la vers : %d\n",Client_actuel->client_fd);
				msgstruct.type = UNICAST_SEND;
				msgstruct.pld_len = strlen(buff)-1;
				// Sending structure (UNICAST)
				if (send(Client_actuel->client_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
					return "";
				}
				// Sending message (UNICAST)
				if (send(Client_actuel->client_fd, buff, msgstruct.pld_len, 0) <= 0) {
					return "";
				}
				return "Message Delivred !\n";
			}
			
			Client_actuel = Client_actuel->suivant;
		}
		
	}

	return "User not found :/\n";
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
			struct message* msgstr = malloc(sizeof(*msgstr)); // pour stocker le pseudo ( problème de variable local)
			memset(msgstr,0,sizeof(*msgstr));

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

                Echo_for_pseudo(client_fd,buff,msgstruct,msgstr,Client_ancien);
                
				// connection time
				time_t secondes;
				struct tm instant;

				time(&secondes);
				instant=*localtime(&secondes);

				
				insertion(Client_ancien,client_fd-3,client_fd, ntohs(sockptr->sin_port),inet_ntoa(client_address),msgstr->infos,instant);

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
				free(msgstr);
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
				printf("buffer : %s, taille : %d\n", buff,msgstruct.pld_len);
				if (strcmp(buff,"/quit\n") == 0) { // If a socket previously created to communicate with a client detects a disconnection from the client side
					// display message on terminal
					printf("[SERVER] : Client[%i] on socket %d has disconnected from server\n", i,pollfds[i].fd);
					
					// free memory
					supprimer(Client_ancien,pollfds[i].fd);
					free(msgstr);
					// Close socket and set struct pollfd back to default
					close(pollfds[i].fd);
					pollfds[i].fd = -1;
					pollfds[i].events = 0;
					pollfds[i].revents = 0;
					continue;
				}

				else {
					printf("[SERVER]: Client[%i] on socket (%d) says \"%s\"\n", i,pollfds[i].fd, buff);
					printf(" [SERVER]: pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
					char test2[MSG_LEN];// store changed pseudo
					memset(test2, 0, MSG_LEN);
					switch (msgstruct.type)
					{
					case NICKNAME_LIST :
						memset(buff, 0, MSG_LEN);
						strcpy(buff,echo_users(Client_ancien,buff));
						msgstruct.pld_len = strlen(buff);
						break;
					case NICKNAME_NEW :
						Echo_for_pseudo(pollfds[i].fd,buff,msgstruct,msgstr,Client_ancien);
						memset(test2,0,MSG_LEN);
						extract_data(6,msgstruct.pld_len,buff,test2);
						change_pseudo(Client_ancien,test2,pollfds[i].fd);
						break;	
					case NICKNAME_INFOS :
						memset(buff, 0, MSG_LEN);
						strcpy(buff,user_info(Client_ancien,msgstruct.infos));
						printf("buff apres:%s",buff);
						msgstruct.pld_len = strlen(buff);
						break;
					case BROADCAST_SEND :
						memset(test2,0,MSG_LEN);
						extract_data(7,msgstruct.pld_len,buff,test2);
						brodcast_send(Client_ancien,test2,msgstruct);
						break;
					case UNICAST_SEND :
						memset(test2,0,MSG_LEN); // exctract msg
						extract_data(0,strlen(buff),buff,test2);
						printf("test to : %s\n",test2);
						memset(buff, 0, MSG_LEN);
						strcpy(buff,unicast_send(Client_ancien,test2,msgstruct));
						msgstruct.pld_len = strlen(buff)-1;
						break;
					
					default:
						break;
					}
				
					if (msgstruct.type != NICKNAME_NEW){
					// send back data to client
						// Sending structure (ECHO)
						if (send(pollfds[i].fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
							return 0;
						}
						// Sending message (ECHO)
						if (send(pollfds[i].fd, buff, msgstruct.pld_len, 0) <= 0) {
							return 0;
						}
					}
				}
				// Set activity detected back to default
				pollfds[i].revents = 0;
			}
			free(msgstr);
		}
	}

	return 0;
}
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
	strcpy(nouveau_client->nom_salon,"");
    
    
	// insertion du client dans la chaine
		nouveau_client->suivant = Client_ancien->premier;
        Client_ancien->premier = nouveau_client;
		printf("Connection succeeded and client[%i] info stored !\n %s used address %s:%hu \n",client_id,Client_ancien->premier->pseudo, local_address, sin_port);
		printf("client_fd = %d\n", client_fd);
    
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

void supprimer(Client_ancien *Client_ancien,salon_recent* Salon_recent, int Client_fd )
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
			else if (Client_actuel->client_fd == Client_fd && Client_actuel->suivant->suivant == NULL){ // if the last client quit we need to free all mallocs
				Client_info *asupprimer = Client_actuel;
				Client_info *asupprimer2 = Client_actuel->suivant;
				salon *asupprimer3 = Salon_recent->premier;
				free(asupprimer);
				free(asupprimer2);
				free(asupprimer3);
				free(Client_ancien);
				free(Salon_recent);
				printf("Client on socket %d Deleted from storage\n",Client_fd);
				printf(" Client Storage is empty ! No Client in the server :/\n");
				break;
			}
			Client_actuel->suivant = Client_actuel->suivant->suivant;
		}
		
	}
	
}

// inistialiser le pointeur de contrôl de la chaine
salon_recent *salon_init()
{
    salon_recent *Salon_recent = malloc(sizeof(salon_recent *));
	salon *salon_init = malloc(sizeof(*salon_init));
    if (Salon_recent == NULL || salon_init == NULL)
    {
        exit(EXIT_FAILURE);
    }

	salon_init->Nutilisateurs = 0;
	salon_init->next = NULL;
	strcpy(salon_init->nom_salon,"There is no channel :/ Why don't you create one with /create <channel_name> ? :)\n"); // to control if there is no channel left
    Salon_recent->premier = salon_init;

    return Salon_recent;
}

char* add_salon(Client_ancien*Client_ancien, salon_recent *salon_recent,char* buff,char*pseudo) // une chaine de salon
{
	// création du nouveau salon
    salon *nouveau_salon = malloc(sizeof(*nouveau_salon));
    if (nouveau_salon == NULL)
    {
        exit(EXIT_FAILURE);
    }

	nouveau_salon->Nutilisateurs = 1;
	strcpy(nouveau_salon->nom_salon,buff); 

    
    
	// insertion du salon dans la chaine
		nouveau_salon->next = salon_recent->premier;
        salon_recent->premier = nouveau_salon;
        

		if ( Client_ancien->premier != NULL){
			Client_info* Client_actuel= Client_ancien->premier;
			while (Client_actuel->suivant != NULL) // find the pseudo to add nom_salon in struct
			{	

				if ( strcmp(pseudo,Client_actuel->pseudo) == 0){
					strcpy(Client_actuel->nom_salon,buff);
				}
				Client_actuel = Client_actuel->suivant;
			}
		}

		char test[MSG_LEN];
		memset(test,0,MSG_LEN);
		extract_data(0,strlen(buff),buff,test); // delete the \n
        memset(buff,0,MSG_LEN);
        strcpy(buff,"You have created and joined channel : ");
        buff = strcat(buff,test);
		return buff;
	
}

// delete a salon
void supprimer_salon(salon_recent *salon_recent, char *nom_salon)
{
    assert(salon_recent);
	if ( salon_recent->premier != NULL){
		salon* salon_actuel= salon_recent->premier;
		while (salon_actuel->next != NULL)
		{	
			static int c = 0;
			if ( (strcmp(salon_actuel->nom_salon,nom_salon)) == 0 && c == 0 && salon_actuel->next->next != NULL){ // si le 1er salon de la liste est a supprimé
				salon_recent->premier = salon_actuel->next;
				free(salon_actuel);
				printf("Channel %s deleted\n",nom_salon);
				c++;
				break;
			}
			else if ((strcmp(salon_actuel->next->nom_salon,nom_salon)) == 0){
				salon *asupprimer = salon_actuel->next;
				salon_actuel->next = salon_actuel->next->next;
				free(asupprimer);
				printf("Channel %s deleted\n",nom_salon);
				break;
			}
			else if ((strcmp(salon_actuel->nom_salon,nom_salon)) == 0 && salon_actuel->next->next == NULL){
				salon *asupprimer = salon_actuel;
				salon_recent->premier = salon_actuel->next;
				free(asupprimer);
				printf("Channel %s deleted\n",nom_salon);
				printf("Channel storage is empty ! no channel in the server \n");
				break;
			}
			salon_actuel->next = salon_actuel->next->next;
		}
		
	}
	
}
// check if there is a same channel name before
int find_salon(salon_recent *Salon_recent, char* nom_salon)
{
    assert(Salon_recent);

	if ( Salon_recent->premier != NULL){
		salon* Salon_actuel = Salon_recent->premier;
		while (Salon_actuel->next != NULL)
		{				
			if ( strcmp(nom_salon,Salon_actuel->nom_salon) == 0){
				return 1;
			}
			Salon_actuel = Salon_actuel->next;
		}
		
	}
	return 0;

}

// if the salon exists add number of members
int find_salon_and_add(salon_recent *Salon_recent, char* nom_salon) 
{
    assert(Salon_recent);

	if ( Salon_recent->premier != NULL){
		salon* Salon_actuel = Salon_recent->premier;
		while (Salon_actuel->next != NULL)
		{				
			if ( strcmp(nom_salon,Salon_actuel->nom_salon) == 0){
				Salon_actuel->Nutilisateurs++;
				return 1;
			}
			Salon_actuel = Salon_actuel->next;
		}
		
	}
	return 0;
	
}

// show existing salons
char* echo_salons(salon_recent *Salon_recent, char* buff)
{
    assert(Salon_recent);

	if (Salon_recent->premier->next == NULL){ // if there is no channel
		memset(buff,0,MSG_LEN);
		strcpy(buff,"There is no channel :/ Why don't you create one with /create <channel_name> ? :)\n");
		return buff;
	}
	else{
	salon* Salon_actuel = Salon_recent->premier;
		while (Salon_actuel->next != NULL)
			{				
				buff = strcat(buff,"-");
				buff = strcat(buff,Salon_actuel->nom_salon);
				buff = strcat(buff,"\n");
				Salon_actuel = Salon_actuel->next;
			}
		return buff;
	}
}

//send message to a salon

int send_to_salon(Client_ancien*Client_ancien,struct message msgstruct,char* nom_salon, char* pseudo,char*buff){
	
	assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		while (Client_actuel->suivant != NULL)
		{	
			char test[MSG_LEN];
			memset(test,0,MSG_LEN);
			extract_data(0,strlen(Client_actuel->nom_salon)-1,Client_actuel->nom_salon,test); // delete the \n
			char test2[MSG_LEN];
			memset(test2,0,MSG_LEN);
			extract_data(0,strlen(pseudo),pseudo,test2); // delete the \n
			if ( (strcmp(test,nom_salon)) == 0 && (strcmp(test2,Client_actuel->pseudo)) != 0){ // if the user is in the salon and diffrent from the sender
				// send data to user
						// Sending structure 
						msgstruct.type = MULTICAST_SEND;
						msgstruct.pld_len = strlen(buff);

						if (send(Client_actuel->client_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
							return 0;
						}
						// Sending message 
						if (send(Client_actuel->client_fd, buff, msgstruct.pld_len, 0) <= 0) {
							return 0;
						}
			}
			Client_actuel = Client_actuel->suivant;
		}
		
	}
	return 0;	
}
//send message to a salon

int info_to_salon(Client_ancien*Client_ancien,struct message msgstruct,char* nom_salon, char* pseudo,char*buff){
	
	assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		while (Client_actuel->suivant != NULL)
		{	
			char test[MSG_LEN];
			memset(test,0,MSG_LEN);
			extract_data(0,strlen(Client_actuel->nom_salon)-1,Client_actuel->nom_salon,test); // delete the \n
			char test2[MSG_LEN];
			memset(test2,0,MSG_LEN);
			extract_data(0,strlen(pseudo),pseudo,test2); // delete the \n
			if ( ((strcmp(test,nom_salon)) == 0 && (strcmp(test2,Client_actuel->pseudo)) != 0) || (strcmp(Client_actuel->nom_salon,"-")) == 0){ // if the user is in the salon and diffrent from the sender or if he quit and he is the last one
				// send data to user
						// Sending structure 
						msgstruct.type = MULTICAST_INFO;
						msgstruct.pld_len = strlen(buff);
						
						if (send(Client_actuel->client_fd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
							return 0;
						}
						// Sending message 
						if (send(Client_actuel->client_fd, buff, msgstruct.pld_len, 0) <= 0) {
							return 0;
						}
			}
			Client_actuel = Client_actuel->suivant;
		}
		
	}
	return 0;	
}

char* exit_salon(Client_ancien*Client_ancien,salon_recent*salon_recent,char* nom_salon, char* pseudo,char*buff){
	assert(Client_ancien);
	assert(salon_recent);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		while (Client_actuel->suivant != NULL)
		{	
			char test2[MSG_LEN];
			memset(test2,0,MSG_LEN);
			extract_data(0,strlen(pseudo),pseudo,test2); // delete the \n
			if ((strcmp(test2,Client_actuel->pseudo)) == 0){
				if ( salon_recent->premier != NULL){
					salon* salon_actuel = salon_recent->premier;
					while (salon_actuel->next != NULL)
					{	
						if ( strcmp(nom_salon,salon_actuel->nom_salon) == 0){
							if ( salon_actuel->Nutilisateurs > 1){
								salon_actuel->Nutilisateurs--;
								strcpy(Client_actuel->nom_salon,"");
								memset(buff,0,MSG_LEN);
								strcpy(buff,"INFO > ");
								strcpy(buff,strcat(buff,pseudo));
								strcpy(buff,strcat(buff," has left "));
								strcpy(buff,strcat(buff,nom_salon));
								return buff;
							}
							else{
								salon_actuel->Nutilisateurs--;
								strcpy(Client_actuel->nom_salon,"-"); // le "-" comme indicateur qu'il est le dernier utilisé dans la fct info_to_salon
								memset(buff,0,MSG_LEN);
								strcpy(buff,"INFO > ");
								strcpy(buff,strcat(buff," You were the last user in this channel, "));
								char test[MSG_LEN];
								memset(test,0,MSG_LEN);
								extract_data(0,strlen(nom_salon)-1,nom_salon,test); // delete the \n
								strcpy(buff,strcat(buff,test));
								supprimer_salon(salon_recent,nom_salon); // delete channel
								strcpy(buff,strcat(buff," has been destroyed\n"));
								return buff;

							}
							
						}
						salon_actuel = salon_actuel->next;
					}
					return "The channel name is not correct";	
				}
				
			}
			Client_actuel = Client_actuel->suivant;
		}
		
	}
	return "";
}


//join salon
char* join_salon(Client_ancien*Client_ancien,salon_recent*Salon_recent, char* pseudo,char* nom_salon,char*buff){
	assert(Client_ancien);

	if (find_salon_and_add(Salon_recent,nom_salon) == 1){ // if the salon exists add number
		if ( Client_ancien->premier != NULL){
			Client_info* Client_actuel= Client_ancien->premier;
			while (Client_actuel->suivant != NULL) // find the pseudo to add nom_salon in struct
			{	
				char test[MSG_LEN];
				memset(test,0,MSG_LEN);
				extract_data(0,strlen(pseudo),pseudo,test); // delete the \n
				if ( strcmp(test,Client_actuel->pseudo) == 0){
					memset(Client_actuel->nom_salon,0,128);
					strcpy(Client_actuel->nom_salon,nom_salon);
					
					memset(buff,0,MSG_LEN);
					strcpy(buff,"You have joined the channel :");
					buff = strcat(buff,nom_salon);
					return buff;
				}
				Client_actuel = Client_actuel->suivant;
			}
		}	
	}
	else{
		memset(buff,0,MSG_LEN);
		strcpy(buff,"Channel not found :/ try again...\n");
		return buff;
	}
	return "no";
}

//check if a client in a salon
char* has_a_salon(Client_ancien* Client_ancien,char* pseudo){
	assert(Client_ancien);

	if ( Client_ancien->premier != NULL){
		Client_info* Client_actuel= Client_ancien->premier;
		while (Client_actuel->suivant != NULL)
		{	
			char test[MSG_LEN];
			memset(test,0,MSG_LEN);
			extract_data(0,strlen(pseudo),pseudo,test); // delete the \n

			if ( strcmp(test,Client_actuel->pseudo) == 0){ // find client 
				if ( (strcmp(Client_actuel->nom_salon,"") != 0) && (strcmp(Client_actuel->nom_salon,"-") != 0) ){ // if client has a channel
					return Client_actuel->nom_salon;
				}	
			}
			Client_actuel = Client_actuel->suivant;			
		}	
	}
	return "no";
}
//check salon name
char * check_salon_name(char*test2,salon_recent* salon_recent){

		int block = 0; // to verify the right input of the name
		while ( block == 0){
						
			// test the name
				int block2 = 0;// to control if there is a space in the pseudo
				int block3 = 0;// to control if there is a special caracter in the pseudo
				int block4 = 0;// to control if there is something after /create

				for (int k = 0;k < strlen(test2);k++){ // to extract the pseudo and verify
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
					return "[SERVER] : Please Repeat -> a space on your channel name ^^\n";
					
				}
				else if ( strlen(test2) > 10){
					return "[SERVER] : Please Repeat -> channel name length must not exceed 10 caracters :)\n";
				}
				else if (block3 == 1){
					return "[SERVER] : Please Repeat -> a special caracter on your channel name^^\n";
				
				}
				else if (block4 == 1){
					return "[SERVER] : Please Repeat -> no channel name founded after command :/ \n";
			
				}
				else if (1 == (find_salon(salon_recent,test2))) {
					return "[SERVER] : Please Repeat -> channel name already used :/ \n";
				
				}
				else{
				// everything is okey
				block = 1;
				}
			} 
			
	return "ok";		     
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

// check if the input of pseudo was right
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
						printf("New pseudo : %s\n",msgstr->infos);
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
	//Init le pointeur vers la chaine client
	Client_ancien *Client_ancien = initialisation();

    //Init le pointeur vers la chaine salon
	salon_recent * salon_recent = salon_init();

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
				supprimer(Client_ancien,salon_recent,pollfds[i].fd);
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
					supprimer(Client_ancien,salon_recent, pollfds[i].fd);
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
						memset(buff, 0, MSG_LEN);
						strcpy(buff,unicast_send(Client_ancien,test2,msgstruct));
						msgstruct.pld_len = strlen(buff)-1;
						break;
                    case MULTICAST_CREATE :
						memset(buff, 0, MSG_LEN);
						strcpy(buff,check_salon_name(msgstruct.infos,salon_recent));
						if ( (strcmp(buff,"ok") == 0)){
							memset(buff, 0, MSG_LEN);
							strcpy(buff,add_salon(Client_ancien, salon_recent,msgstruct.infos,msgstruct.nick_sender));
							msgstruct.type = MULTICAST_CREATE;
						}
						else {
							msgstruct.type = MULTICAST_INFO; // choisi juste pour afficher msg d'érreur
						}
						msgstruct.pld_len = strlen(buff);
						break;
					case MULTICAST_LIST :
						memset(buff, 0, MSG_LEN);
						strcpy(buff,echo_salons(salon_recent,buff));
						msgstruct.pld_len = strlen(buff);
						break;
					case MULTICAST_JOIN :
						memset(buff, 0, MSG_LEN);
						char test4[MSG_LEN];
						memset(test4,0,MSG_LEN);
						strcpy(test4,has_a_salon(Client_ancien,msgstruct.nick_sender));
						strcpy(buff,join_salon(Client_ancien,salon_recent,msgstruct.nick_sender,msgstruct.infos,buff));
						if (buff[0]=='Y') // if the client joined successfully the channel and had not been in channel 
						{
							memset(test2,0,MSG_LEN);
							strcpy(test2,"INFO > ");
							strcpy(test2,strcat(test2,msgstruct.nick_sender));
							strcpy(test2,strcat(test2," has joined "));
							strcpy(test2,strcat(test2,msgstruct.infos));
							char test[MSG_LEN];
							memset(test,0,MSG_LEN);
							extract_data(0,strlen(msgstruct.infos)-1,msgstruct.infos,test); // delete the \n
							info_to_salon(Client_ancien,msgstruct,test,msgstruct.nick_sender,test2);// send INFO to others in channel
							
							if ( (strcmp(test4,"no")) != 0){// if the client joined successfully the channel and been in one

								char test[MSG_LEN];
								memset(test,0,MSG_LEN);
								extract_data(0,strlen(test4)-1,test4,test); // delete the \n and extract channel name
						
								memset(test2,0,MSG_LEN);
								strcpy(test2,"INFO > ");
								strcpy(test2,strcat(test2,msgstruct.nick_sender));
								strcpy(test2,strcat(test2," has left "));
								strcpy(test2,strcat(test2,test4));
								info_to_salon(Client_ancien,msgstruct,test,msgstruct.nick_sender,test2);// send INFO about leaving channel to other members
							}
							
							msgstruct.type = MULTICAST_JOIN;
						}
						else
						{
							msgstruct.type = NICKNAME_INFOS; // if there channel not found
						}
						msgstruct.pld_len = strlen(buff);
						break;
					case MULTICAST_SEND :
						
						send_to_salon(Client_ancien,msgstruct,msgstruct.infos,msgstruct.nick_sender,buff);// send msg to others in channel
						msgstruct.type = MULTICAST_SEND;
						msgstruct.pld_len = strlen(buff);
						break;
					case MULTICAST_QUIT :
						memset(buff, 0, MSG_LEN);
						strcpy(buff,exit_salon(Client_ancien,salon_recent,msgstruct.infos,msgstruct.nick_sender,buff));// send msg to others in channel
						char test[MSG_LEN];
						memset(test,0,MSG_LEN);
						extract_data(0,strlen(msgstruct.infos)-1,msgstruct.infos,test);// delete the \n
						info_to_salon(Client_ancien,msgstruct,test,msgstruct.nick_sender,buff);
						memset(buff, 0, MSG_LEN);
						strcpy(buff,"You have left the channel\n");
						msgstruct.type = NICKNAME_INFOS;
						msgstruct.pld_len = strlen(buff);
						break;
					
					default:
						break;
					}
				
					if (msgstruct.type != NICKNAME_NEW && msgstruct.type != MULTICAST_SEND){ // le cas de NICKNAME_NEW est traité dans Echo_for_pseudo
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
#define MSG_LEN 1024
#define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1"

typedef struct Client_info Client_info;
struct Client_info
{
	int client_id;
	int client_fd; 
    in_port_t sin_port;
	char* local_address;
    char* pseudo;
    Client_info *suivant;
	
};

typedef struct Client_ancien Client_ancien;
struct Client_ancien
{
    Client_info *premier;
};

int src_port;
char src_ip[20];

/** Funciones **/
void error(char* msg);
int start_values();
int create_socket();
void wait_connections();

/** Variables **/
char config[30];
char file_config[100];
char file_path[100];

int udpSocket;
socklen_t addr_size;
struct sockaddr_in serverAddr;


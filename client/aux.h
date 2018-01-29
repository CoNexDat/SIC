/** Global variables **/

// Variables //
int count_w;

int CLIENT_PORT;
int SERVER_PORT;
char CLIENT_IP[20];
char SERVER_IP[20];
int MEDIAN_MAX_SIZE;
int P = 0;
double alpha;
double err_RTT;
int TIMEOUT=0;
double actual_m;
double actual_c;

bool synck=false;
bool to=false;
bool debug=false;
int pre_sync;
int epoch_sync;
int err_sync;

int clientSocket, flag_parameters=0;
socklen_t addr_size;

struct sockaddr_in serverAddr;
struct sockaddr_in local_addr; 

char file_path[100];
char sync_values_txt[30];
char sync_values_path[100];
char config[30];
char file_config[100];

double t1_p=0, t2_p=0, t3_p=0, t4_p=0;

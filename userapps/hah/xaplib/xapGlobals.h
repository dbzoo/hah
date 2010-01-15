
struct tg_xap_msg g_xap_msg[XAP_MAX_MSG_ELEMENTS];

int g_debuglevel=0;
int g_xap_index = 0;

// xAP global socket structures
int g_xap_sender_sockfd = 0;
int g_xap_receiver_sockfd = 0;

// The network interface, port that xAP is to use.
char g_interfacename[20] = {'\0'};
int g_interfaceport = 0;
char g_ip[16] = {'\0'};

char g_instance[20] = {'\0'};
char g_serialport[20] = {'\0'};
char g_uid[9] = {'\0'};

struct sockaddr_in g_xap_receiver_address;
struct sockaddr_in g_xap_mybroadcast_address;
struct sockaddr_in g_xap_sender_address;


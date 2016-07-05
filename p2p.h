#include<stdio.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<time.h>
#include<math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<stdint.h>
#define MAXPENDING 15 /* Maximum outstanding connection requests */
#include<pthread.h>
#include<netdb.h>
#define ERR_NO_NUM -1
#define ERR_NO_MEM -2





//Structure that carries all node information
struct Node_Info
{
	int node_no;	
	int packet_delay;
	int packet_drop_probability;
	char file_name_initial[100];
	char file_name_requested[100];
	int Start_time;
	int share;
	int req_timeout;
};

struct Group_Show_interest
{
	uint16_t msgtype;
	uint16_t client_node_id;
	uint16_t number_of_files;
	char filename[32];
	uint16_t type;
};

typedef struct
{
	uint16_t msgtype;
	uint16_t number_of_files;
	char filename[32];
	uint16_t number_of_neighbours;
	uint16_t neighbour_id[25];
	uint32_t neighbour_ip[32];
	uint16_t neighbour_port[25];
}Group_Assign;

struct File_maintenance
{
	char file_name[32];
	int number_of_neighbours;
	int neighbour_node_number[25];
	char neighbour_ip[25][32];
	int neighbour_port[25];	
	int type[25];
};

struct File_Exchange
{
	char filename[32];
	int segments[650];	
};

struct File_Neighbour_info
{
	char filename[32];
	int segments[25][650];
};
clock_t before;
int number_nodes;
int flag=1;
struct File_Neighbour_info track[25];
struct Node_Info client_details;
char file_name[100];
int assign_flag;
int file_tot_client=0;
int file_download_complete=0;
int max_file_download=0;

static FILE* in_fp;
static FILE* track_fp;

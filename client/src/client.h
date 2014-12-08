#include <pthread.h>

#define USER_NAME_LEN 16
#define CMD_LEN 16
#define BUFF_LEN 1024
#define LINEBUFF 2048

#define SERVERIP "127.0.0.1"
#define SERVERPORT 7000

struct packet_t
{
    char option[CMD_LEN];
    char username[USER_NAME_LEN];
    char buff[BUFF_LEN];
};

struct user_t
{
    int sockfd;
    char username[USER_NAME_LEN];
};

struct connection_t
{
    pthread_t thread_ID;
    int sockfd;
};
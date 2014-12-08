#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define USER_NAME_LEN 16
#define CMD_LEN 16
#define BUFF_LEN 1024

#define LISTEN_PORT 7000
#define LISTEN_IP "0.0.0.0"

/* connected client info */

typedef struct {
    pthread_t thread_id;
    int client_sockfd;
    char username[USER_NAME_LEN];
} user_info_t;

typedef struct node_t node_t;

/* connected client node */

typedef struct node_t {
    user_info_t user_info;
    node_t *next;
} node_t;

/* linked list for the connected clients */

typedef struct {
    node_t *head, *tail;
    int size;
} linked_list_t;

/* Packet data */

typedef struct {
    char option[CMD_LEN];
    char username[USER_NAME_LEN];
    char buff[BUFF_LEN];
} packet_t;

int compare_user(user_info_t *a, user_info_t *b);
void list_init(linked_list_t *list);
int list_insert(linked_list_t *list, user_info_t *user_tail);
void list_print(linked_list_t *list);
int list_delete(linked_list_t *list, user_info_t *user_info);

void *io_handler(void *param);
void *client_handler(void *fd);

#endif
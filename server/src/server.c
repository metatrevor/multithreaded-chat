#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "server.h"

int server_sockfd, client_sockfd;
int addr_size;
pthread_mutex_t client_list_mutex;
linked_list_t client_list;
struct sockaddr_in server_addr, client_addr;
pthread_t interrupt;
linked_list_t client_list;

int compare_user(user_info_t *a, user_info_t *b)
{
    return a->client_sockfd - b->client_sockfd;
}

void list_init(linked_list_t *list)
{
    list->head = list->tail = NULL;
    list->size = 0;
}

int list_insert(linked_list_t *list, user_info_t *user_tail)
{
    if (list->size == MAX_CLIENTS)
        return -1;
    if (list->head == NULL) {
        list->head = (node_t *) malloc(sizeof(node_t));
        list->head->user_info = *user_tail;
        list->head->next = NULL;
        list->tail = list->head;
    } else {
        list->tail->next = (node_t *) malloc(sizeof(node_t));
        list->tail->next->user_info = *user_tail;
        list->tail->next->next = NULL;
        list->tail = list->tail->next;
    }
    list->size++;
    return 0;
}

int list_delete(linked_list_t *list, user_info_t *user_info)
{
    node_t *curr, *temp;
    if (list->head == NULL)
        return -1;
    if (compare_user(user_info, &list->head->user_info) == 0) {
        temp = list->head;
        list->head = list->head->next;
        if (list->head == NULL)
            list->tail = list->head;
        free(temp);
        list->size--;
        return 0;
    }
    for (curr = list->head; curr->next != NULL; curr = curr->next) {
        if (compare_user(user_info, &curr->next->user_info) == 0) {
            temp = curr->next;
            if (temp == list->tail)
                list->tail = curr;
            curr->next = curr->next->next;
            free(temp);
            list->size--;
            return 0;
        }
    }
    return -1;
}

void list_print(linked_list_t *list)
{
    node_t *curr;
    user_info_t *thr_info;
    printf("Connection count: %d\n", list->size);
    for (curr = list->head; curr != NULL; curr = curr->next) {
        thr_info = &curr->user_info;
        printf("[%d] %s\n", thr_info->client_sockfd, thr_info->username);
    }
}

void *console_handler(void *param)
{
    char option[16];
    while (scanf("%s", option) == 1) {
        if (!strcmp(option, "exit")) {
            printf("Terminating server...\n");
            pthread_mutex_destroy(&client_list_mutex);
            close(client_sockfd);
            exit(0);
        }
        else if (!strcmp(option, "list")) {
            pthread_mutex_lock(&client_list_mutex);
            list_print(&client_list);
            pthread_mutex_unlock(&client_list_mutex);
        }
        else {
            fprintf(stderr, "Unknown command: %s...\n", option);
        }
    }
    return NULL;
}

void *client_handler(void *fd)
{
    user_info_t user_info = *(user_info_t *) fd;
    packet_t packet;
    node_t *curr;
    int bytes, sent;
    while (1) {
        bytes = recv(user_info.client_sockfd, (void *) &packet, sizeof(packet_t), 0);
        if (!bytes) {
            fprintf(stderr, "Connection lost from [%d] %s...\n", user_info.client_sockfd, user_info.username);
            pthread_mutex_lock(&client_list_mutex);
            list_delete(&client_list, &user_info);
            pthread_mutex_unlock(&client_list_mutex);
            break;
        }
        printf("[%d] %s : %s %s\n", user_info.client_sockfd, packet.username, packet.option, packet.buff);
        if (!strcmp(packet.option, "username")) {
            printf("Set username to %s\n", packet.username);
            pthread_mutex_lock(&client_list_mutex);
            for (curr = client_list.head; curr != NULL; curr = curr->next) {
                if (compare_user(&curr->user_info, &user_info) == 0) {
                    strcpy(curr->user_info.username, packet.username);
                    strcpy(user_info.username, packet.username);
                    break;
                }
            }
            pthread_mutex_unlock(&client_list_mutex);
        }
        else if (!strcmp(packet.option, "whisp")) {
            int i;
            char target[USER_NAME_LEN];
            for (i = 0; packet.buff[i] != ' '; i++);
            packet.buff[i++] = 0;
            strcpy(target, packet.buff);
            pthread_mutex_lock(&client_list_mutex);
            for (curr = client_list.head; curr != NULL; curr = curr->next) {
                if (strcmp(target, curr->user_info.username) == 0) {
                    packet_t spacket;
                    memset(&spacket, 0, sizeof(packet_t));
                    if (!compare_user(&curr->user_info, &user_info)) continue;
                    strcpy(spacket.option, "msg");
                    strcpy(spacket.username, packet.username);
                    strcpy(spacket.buff, &packet.buff[i]);
                    sent = send(curr->user_info.client_sockfd, (void *) &spacket, sizeof(packet_t), 0);
                }
            }
            pthread_mutex_unlock(&client_list_mutex);
        }
        else if (!strcmp(packet.option, "send")) {
            pthread_mutex_lock(&client_list_mutex);
            for (curr = client_list.head; curr != NULL; curr = curr->next) {
                packet_t spacket;
                memset(&spacket, 0, sizeof(packet_t));
                if (!compare_user(&curr->user_info, &user_info)) continue;
                strcpy(spacket.option, "msg");
                strcpy(spacket.username, packet.username);
                strcpy(spacket.buff, packet.buff);
                sent = send(curr->user_info.client_sockfd, (void *) &spacket, sizeof(packet_t), 0);
            }
            pthread_mutex_unlock(&client_list_mutex);
        }
        else if (!strcmp(packet.option, "exit")) {
            printf("[%d] %s has disconnected...\n", user_info.client_sockfd, user_info.username);
            pthread_mutex_lock(&client_list_mutex);
            list_delete(&client_list, &user_info);
            pthread_mutex_unlock(&client_list_mutex);
            break;
        }
        else {
            fprintf(stderr, "Garbage data from [%d] %s...\n", user_info.client_sockfd, user_info.username);
        }
    }

    close(user_info.client_sockfd);

    return NULL;
}

int main(int argc, char *argv[])
{
    /* Initialize client list*/
    list_init(&client_list);

    /*Initiate the client list mutex*/
    pthread_mutex_init(&client_list_mutex, NULL);

    /* create a socket */
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "socket() failed...\n");
        return -1;
    }

    /* initialize address struct */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LISTEN_PORT);
    server_addr.sin_addr.s_addr = inet_addr(LISTEN_IP);
    memset(&(server_addr.sin_zero), 0 , 8);

    /* bind address with socket */
    if (bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "bind() failed ...\n %s", strerror(errno));
        exit(EXIT_FAILURE);
    };

    /* initiate interrupt handler for IO controlling */
    printf("Starting admin interface...\n");
    if (pthread_create(&interrupt, NULL, console_handler, NULL) != 0) {
        fprintf(stderr, "pthread_create() failed...\n");
        exit(EXIT_FAILURE);
    }

    /* start listening for connections */
    if (listen(server_sockfd, 10) == -1) {
        fprintf(stderr, "listen() failed...\n %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Listening for client connections...\n");

    while (1) {
        addr_size = sizeof(struct sockaddr_in);
        if ((client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &addr_size)) == -1) {
            fprintf(stderr, "accept() failed...\n %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else {

            if(client_list.size == MAX_CLIENTS) {
                fprintf(stderr, "Max no of clients have already connected");
                continue;
            }

            /* add connected client to connected list*/
            printf("New connection request received...\n");
            user_info_t userinfo;
            userinfo.client_sockfd = client_sockfd;
            strcpy(userinfo.username, "Anonymous");
            pthread_mutex_lock(&client_list_mutex);
            list_insert(&client_list, &userinfo);
            pthread_mutex_unlock(&client_list_mutex);
            pthread_create(&userinfo.thread_id, NULL, client_handler, (void *) &userinfo);
        }

    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "client.h"

int isconnected, sockfd;
char option[LINEBUFF];
struct user_t me;

int connect_with_server();
void setusername(struct user_t *me);
void logout(struct user_t *me);
void login(struct user_t *me);
void *receiver(void *param);
void sendtoall(struct user_t *me, char *msg);
void sendtousername(struct user_t *me, char * target, char *msg);


void login(struct user_t *me)
{
    int recvd;
    if(isconnected) {
        fprintf(stderr, "You are already connected to server at %s:%d\n", SERVERIP, SERVERPORT);
        return;
    }
    sockfd = connect_with_server();
    if(sockfd >= 0) {
        isconnected = 1;
        me->sockfd = sockfd;
        if(strcmp(me->username, "Anonymous")) setusername(me);
        printf("Logged in as %s\n", me->username);
        printf("Receiver started [%d]...\n", sockfd);
        struct connection_t threadinfo;
        pthread_create(&threadinfo.thread_ID, NULL, receiver, (void *)&threadinfo);

    }
    else {
        fprintf(stderr, "Connection rejected...\n");
    }
}

int connect_with_server()
{
    int newfd;
    struct sockaddr_in serv_addr;
    struct hostent *to;

    /* resolve server ip */
    if((to = gethostbyname(SERVERIP))==NULL) {
        fprintf(stderr, "gethostbyname() error...\n");
        return -1;
    }

    if((newfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "socket() error...\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVERPORT);
    serv_addr.sin_addr = *((struct in_addr *)to->h_addr);
    memset(&(serv_addr.sin_zero), 0, 8);

    if(connect(newfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "connect() error...\n");
        return -1;
    }
    else {
        printf("Connected to server at %s:%d\n", SERVERIP, SERVERPORT);
        return newfd;
    }
}

void logout(struct user_t *me)
{
    int sent;
    struct packet_t packet;

    if(!isconnected) {
        fprintf(stderr, "You are not connected...\n");
        return;
    }

    memset(&packet, 0, sizeof(struct packet_t));
    strcpy(packet.option, "exit");
    strcpy(packet.username, me->username);

    /* send  close connetion request */
    sent = send(sockfd, (void *)&packet, sizeof(struct packet_t), 0);
    isconnected = 0;
}

void setusername(struct user_t *me)
{
    int sent;
    struct packet_t packet;

    if(!isconnected) {
        fprintf(stderr, "You are not connected...\n");
        return;
    }

    memset(&packet, 0, sizeof(struct packet_t));
    strcpy(packet.option, "username");
    strcpy(packet.username, me->username);

    /* send  close connetion request */
    sent = send(sockfd, (void *)&packet, sizeof(struct packet_t), 0);
}

void *receiver(void *param)
{
    int recvd;
    struct packet_t packet;

    printf("Waiting here [%d]...\n", sockfd);
    while(isconnected) {

        recvd = recv(sockfd, (void *)&packet, sizeof(struct packet_t), 0);
        if(!recvd) {
            fprintf(stderr, "Connection lost from server...\n");
            isconnected = 0;
            close(sockfd);
            break;
        }
        if(recvd > 0) {
            printf("[%s]: %s\n", packet.username, packet.buff);
        }
        memset(&packet, 0, sizeof(struct packet_t));
    }
    return NULL;
}

void sendtoall(struct user_t *me, char *msg)
{
    int sent;
    struct packet_t packet;

    if(!isconnected) {
        fprintf(stderr, "You are not connected...\n");
        return;
    }

    msg[BUFF_LEN] = 0;

    memset(&packet, 0, sizeof(struct packet_t));
    strcpy(packet.option, "send");
    strcpy(packet.username, me->username);
    strcpy(packet.buff, msg);

    /* send request to close this connetion */
    sent = send(sockfd, (void *)&packet, sizeof(struct packet_t), 0);
}

void sendtousername(struct user_t *me, char *target, char *msg)
{
    int sent, targetlen;
    struct packet_t packet;

    if(target == NULL) {
        return;
    }

    if(msg == NULL) {
        return;
    }

    if(!isconnected) {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    msg[BUFF_LEN] = 0;
    targetlen = strlen(target);

    memset(&packet, 0, sizeof(struct packet_t));
    strcpy(packet.option, "whisp");
    strcpy(packet.username, me->username);
    strcpy(packet.buff, target);
    strcpy(&packet.buff[targetlen], " ");
    strcpy(&packet.buff[targetlen+1], msg);

    /* send request to close this connetion */
    sent = send(sockfd, (void *)&packet, sizeof(struct packet_t), 0);
}

int main(int argc, char **argv)
{
    int sockfd;
    size_t usernamelen;

    memset(&me, 0, sizeof(struct user_t));

    while(fgets(option, sizeof(option), stdin)) {
        if(!strncmp(option, "exit", 4)) {
            logout(&me);
            break;
        }
        if(!strncmp(option, "help", 4)) {
            FILE *fin = fopen("help.txt", "r");
            if(fin != NULL) {
                while(fgets(option, LINEBUFF-1, fin)) puts(option);
                fclose(fin);
            }
            else {
                fprintf(stderr, "Help file not found...\n");
            }
        }
        else if(!strncmp(option, "login", 5)) {
            char *ptr = strtok(option, " ");
            ptr = strtok(0, " ");
            memset(me.username, 0, sizeof(char) * USER_NAME_LEN);
            if(ptr != NULL) {
                usernamelen =  strlen(ptr);
                if(usernamelen > USER_NAME_LEN) ptr[USER_NAME_LEN] = 0;
                strcpy(me.username, ptr);
            }
            else {
                strcpy(me.username, "Anonymous");
            }
            login(&me);
        }
        else if(!strncmp(option, "username", 5)) {
            char *ptr = strtok(option, " ");
            ptr = strtok(0, " ");
            memset(me.username, 0, sizeof(char) * USER_NAME_LEN);
            if(ptr != NULL) {
                usernamelen =  strlen(ptr);
                if(usernamelen > USER_NAME_LEN) ptr[USER_NAME_LEN] = 0;
                strcpy(me.username, ptr);
                setusername(&me);
            }
        }
        else if(!strncmp(option, "whisp", 5)) {
            char *ptr = strtok(option, " ");
            char temp[USER_NAME_LEN];
            ptr = strtok(0, " ");
            memset(temp, 0, sizeof(char) * USER_NAME_LEN);
            if(ptr != NULL) {
                usernamelen =  strlen(ptr);
                if(usernamelen > USER_NAME_LEN) ptr[USER_NAME_LEN] = 0;
                strcpy(temp, ptr);
                while(*ptr) ptr++; ptr++;
                while(*ptr <= ' ') ptr++;
                sendtousername(&me, temp, ptr);
            }
        }
        else if(!strncmp(option, "send", 4)) {
            sendtoall(&me, &option[5]);
        }
        else if(!strncmp(option, "logout", 6)) {
            logout(&me);
        }
        else fprintf(stderr, "Unknown option...\n");
    }
    return 0;
}
// server.c
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "game.h"

int setup_server_socket(int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) return -1;
    if (listen(sockfd, 10) < 0) return -1;

    return sockfd;
}

int accept_new_connection(int listener) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int newfd = accept(listener, (struct sockaddr *)&client_addr, &addrlen);
    if (newfd != -1) {
        printf("New connection from %s on socket %d\n",
               inet_ntoa(client_addr.sin_addr), newfd);
    }
    return newfd;
}

//precondition: fd is ready for reading
//precondition: message_buff has no new"\n" left unprocessed
//disconnect if more than 100\n, return -1
int handle_client_message(int client_fd, PlayerList *players,int epfd) {
    char new_message_buf[BUFF_SIZE]; //dont need \0 ending, parse by \n
    Player *player=findPlayerByFd(players,client_fd);
    if (!player)
    {
        epoll_ctl(epfd,EPOLL_CTL_DEL,client_fd,NULL);
        close(client_fd);
        return -1;
    }
    
    int nbytes = recv(client_fd, new_message_buf, BUFF_SIZE - 1, 0);
    if (nbytes <= 0) {   //disconnected
        handle_disconnect(players,client_fd,epfd);
        return -1;
    }
    if (player->message_len+nbytes>BUFF_SIZE)
    {
        nbytes=BUFF_SIZE- player->message_len;
    }
    
    memcpy(player->message_buf+player->message_len, new_message_buf,nbytes);
    player->message_len+=nbytes;//this is at most BUFF_SIZE
    char *c;
    while ((c = memchr(player->message_buf,'\n',player->message_len))!=NULL)
    {
        int cmd_len=c - player->message_buf+1;//length of the first "cmd\n"
        char cmd_buf[102];
        if (cmd_len>101)
        {
            handle_disconnect(players,client_fd,epfd);
            return -1;
        }
        
        strncpy(cmd_buf,player->message_buf,cmd_len);
        cmd_buf[cmd_len]='\0';
        process_command(cmd_buf, client_fd, players,epfd); //may modify playerlist causing Player *player invalid
        player = findPlayerByFd(players,client_fd);
        if (!player)
        {
            return 0;
        }
        
        if (player->message_len-cmd_len>0)
        {
            memmove(player->message_buf,c+1,player->message_len-cmd_len);
        }
        player->message_len-=cmd_len;
        memset(player->message_buf+player->message_len,0,BUFF_SIZE-player->message_len);

    }
    if (player->message_len>100)
    {
        handle_disconnect(players, client_fd,epfd);
        return -1;
    }
    
    
    return 0;
}


void broadcast_message(PlayerList *players, const char *msg) {
    for (int i = 0; i < players->count; i++) {
        send_message(players->list[i].fd, msg);
    }
}

void send_message(int client_fd, const char *msg) {
    send(client_fd, msg, strlen(msg), 0);
}

void handle_disconnect(PlayerList *playerList, int fd, int epfd){
    Player *player=findPlayerByFd(playerList,fd);
    if (!player)
    {
        fprintf(stderr, "tried to disconnect non-exist fd %d\n", fd);
        return;
    }
    
    if (isRegistered(playerList,fd))
    {
        fprintf(stderr, "Disconnecting registered %s fd %d\n", player->name,fd);
        handle_gg(fd,playerList, epfd);
    }else{
        fprintf(stderr, "Disconnecting pending %s fd %d\n", player->name,fd);
        removePlayer(playerList, fd);
        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
        close(fd);
     }
}

// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include "server.h"
#include <signal.h>
#include "player.h"
#include "game.h"

#define MAX_CLIENTS 500

int pipefd[2];
void myhandler(int sig){
    write(pipefd[1], &sig,sizeof(int));
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    signal(SIGPIPE,SIG_IGN);
    if (pipe(pipefd)==-1){
        perror("pipe failed\n");
        exit(1);
    }
    
    int port = atoi(argv[1]);
    struct sigaction myaction;
    myaction.sa_handler=myhandler;
    myaction.sa_flags=SA_RESTART;
    sigfillset(&myaction.sa_mask);
    sigaction(SIGINT, &myaction,NULL);
    sigaction(SIGTERM, &myaction, NULL);
    
    int listener = setup_server_socket(port);
    if (listener < 0) {
        perror("setup_server_socket");
        return 1;
    }
    int epfd=epoll_create1(0);
    struct epoll_event event={.events=EPOLLIN,.data.fd=pipefd[0]};
    epoll_ctl(epfd, EPOLL_CTL_ADD, pipefd[0],&event);
    event=(struct epoll_event){.events=EPOLLIN,.data.fd=listener};
    epoll_ctl(epfd,EPOLL_CTL_ADD, listener,&event);

    


    PlayerList *players = createPlayerList();

    struct epoll_event events[MAX_CLIENTS];
    while (1) {

        int ready_fd_count=epoll_wait(epfd,events,MAX_CLIENTS,-1);

        for (int i = 0; i < ready_fd_count; i++) {
            fprintf(stderr, "Handling fd %d, events = %x\n", events[i].data.fd, events[i].events);

            if (events[i].data.fd==listener)
            {
                int client_fd=accept_new_connection(listener);
                fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);
                event=(struct epoll_event){.events=EPOLLIN|EPOLLERR|EPOLLHUP,.data.fd=client_fd};
                epoll_ctl(epfd,EPOLL_CTL_ADD, client_fd,&event);
                addPlayer(players,client_fd);
            }else if (events[i].data.fd==pipefd[0])
            {
                close(listener);
                freePlayerList(players);
                close(pipefd[0]);
                close(pipefd[1]);
                return 0;
            }else if (events[i].events & (EPOLLHUP|EPOLLERR))
            {
                handle_disconnect(players,events[i].data.fd,epfd);
            }else{
                handle_client_message(events[i].data.fd,players,epfd);
            }
            
            
            
        }
    }
    return 1;
    
}

// server.h
#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <sys/select.h>
#include "player.h"

int setup_server_socket(int port);
int accept_new_connection(int listener);
int handle_client_message(int client_fd, PlayerList *players,int epfd);
void broadcast_message(PlayerList *players, const char *msg);
void send_message(int client_fd, const char *msg);
void handle_disconnect(PlayerList *playerList, int fd,int epfd);

#endif

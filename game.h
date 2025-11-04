// game.h
#ifndef GAME_H
#define GAME_H

#include "player.h"

void process_command(const char *msg, int client_fd, PlayerList *players, int epfd);
void handle_bomb(int x, int y, int attacker_fd, PlayerList *players, int epfd);
void handle_gg(int client_fd, PlayerList *players,int epfd);
int isValidName(const char *name);
int isValidShip(const int x,const int y, const char d);

#endif

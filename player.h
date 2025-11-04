#ifndef PLAYER_H
#define PLAYER_H
#include "point.h"

#define NAME_LEN 21
#define BUFF_SIZE 1024

typedef enum{
    PENDING,
    REGISTAERED
}RegisterStatus;

typedef struct player{
    RegisterStatus registerStatus;
    //only a letter, a digit, or- 
    char name[NAME_LEN];  //at most 20 bytes
//occurs from left to right or bottom to top
    Point ship[5]; // (-1,-1) means damaged,0-9 valid
    int fd;
    char message_buf[1024];
    int message_len; //stored message length
}Player;

typedef struct {
    Player *list;
    int count;
    int capacity;
}PlayerList;

PlayerList *createPlayerList();

void freePlayerList(PlayerList *playerList);

int addPlayer(PlayerList *playerList, int fd);
int registerPlayer(PlayerList *playerList, const char *name, Point p, char orientation, int fd);

int isRegistered(PlayerList *playerList, int fd);

int copyPlayer(PlayerList *destination, const Player player_data);

void removePlayer(PlayerList *playerList, int fd);

Player *findPlayerByFd(PlayerList *playerList, int fd);

PlayerList *findPlayerAtPoint(PlayerList *playerList, Point p);

int nameExist(const PlayerList *playerList, const char *name);
int isAllDamaged(const Player *player);

#endif
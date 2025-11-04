#include <unistd.h>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "point.h"
#include "player.h"
#define INITIAL_CAPACITY 8

PlayerList *createPlayerList(){
    PlayerList *playerList=malloc(sizeof(PlayerList));
    if (!playerList) return NULL;
    playerList->count=0;
    playerList->capacity=INITIAL_CAPACITY;
    playerList->list=calloc(INITIAL_CAPACITY,sizeof(Player));
    if (!playerList->list)
    {
        free(playerList);
        return NULL;
    }
    
    return playerList;
}

void freePlayerList(PlayerList *playerList){
    for (int i = 0; i < playerList->count; i++)
    {
        close(playerList->list[i].fd);
    }
    
    free(playerList->list);
    free(playerList);
}


//return 0 if realloc fail, 1 if succeed
int addPlayer(PlayerList *playerList, int fd){
    if (playerList->count>=playerList->capacity)
    {
        playerList->list=realloc(playerList->list,sizeof(Player)*playerList->capacity*2);
        if (!playerList->list) return 0;
        memset(playerList->list+playerList->capacity, 0, sizeof(Player) * playerList->capacity);
        playerList->capacity*=2;
    }
    Player *player=&playerList->list[playerList->count++];
    memset(player,0,sizeof(Player));
    player->registerStatus=PENDING;
    player->fd=fd;
    player->message_len=0;
    return 1;
}

//precondition: name is not duplicate, ship is not out-of-bounds
int registerPlayer(PlayerList *playerList, const char *name, Point p, char orientation, int fd){
    Player *player;
    if ((player=findPlayerByFd(playerList,fd))==NULL)
    {
        return 0;
    }
    
    player->registerStatus=REGISTAERED;
    strncpy(player->name, name, NAME_LEN-1);
    
    if (orientation=='-')
    {
        for (int i = 0; i < 5; i++)
        {
            Point ship_point={p.x-2+i,p.y};
            player->ship[i]=ship_point;
        }
        
    }
    else if (orientation=='|')
    {
      for (int i = 0; i < 5; i++)
        {
            Point ship_point={p.x,p.y-2+i};
            player->ship[i]=ship_point;
        }
    }
    return 1;
}

//return 0 if fail
int copyPlayer(PlayerList *destination, const Player player_data){
    if (destination->count>=destination->capacity)
    {
        destination->capacity*=2;
        destination->list=realloc(destination->list,sizeof(Player)*destination->capacity);
        if (!destination->list) return 0;
    }
    Player *player=&destination->list[destination->count++];
    strncpy(player->name, player_data.name, NAME_LEN-1);
    player->fd=player_data.fd;
    for (int i = 0; i < 5; i++)
    {
        player->ship[i]=player_data.ship[i];
    }
    return 1;
}

void removePlayer(PlayerList *playerList, int fd){
    if (!findPlayerByFd(playerList,fd))
    {
        fprintf(stderr,"tried to remove %d,dne in list\n",fd);
        return;
    }
    Player *player=findPlayerByFd(playerList,fd);
    fprintf(stderr,"remove %s \n",player->name);
    if (playerList->list[playerList->count-1].fd==fd)
    {
        playerList->count--;
        return;
    }
    
    for (int i = 0; i < playerList->count-1; i++)
    {
        if (playerList->list[i].fd==fd)
        {
            
            playerList->list[i]=playerList->list[playerList->count-1];

            playerList->count--;
            return;
        }
        
    }
    return;
    
}

int isRegistered(PlayerList *playerList, int fd){
    Player *player=findPlayerByFd(playerList,fd);
    if (!player)
    {
        return -1;
    }
    
    return player->registerStatus==REGISTAERED;
}

Player *findPlayerByFd(PlayerList *playerList, int fd){
    for (int i = 0; i < playerList->count; i++)
    {
        if (playerList->list[i].fd==fd)
        {
            return &playerList->list[i];
        }
        
    }
    return NULL;
}

//return a list of players at location p
PlayerList *findPlayerAtPoint(PlayerList *playerList, Point p){
    PlayerList *damageList=createPlayerList();

    for (int i = 0; i < playerList->count; i++)
    {
        Player player_data=playerList->list[i];
        for (int j = 0; j < 5; j++)
        {
            
            if (point_equal(p,playerList->list[i].ship[j]))
            {
                copyPlayer(damageList, player_data);
                break;
            }
        }
        
    }
    return damageList;
    
}

int nameExist(const PlayerList *playerList, const char *name){
    for (int i = 0; i < playerList->count; i++)
    {
        if (strcmp(playerList->list[i].name,name)==0)
        {
            return 1;
        }
        
    }
    return 0;
}

int isAllDamaged(const Player *player){
    Point dead={-1,-1};
    for (int i = 0; i < 5; i++)
    {
        if (!point_equal(player->ship[i],dead))
        {
            fprintf(stderr,"%s is still alive", player->name);
            return 0;
        }
        
    }
    fprintf(stderr, "%s is alldamaged", player->name);
    return 1;
}

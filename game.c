// game.c
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <ctype.h>
#include "point.h"
#include "server.h"
#include "player.h"
//precondition: msg ends with \n and length<=101
void process_command(const char *msg, int client_fd, PlayerList *players, int epfd) {
    Player *player=findPlayerByFd(players,client_fd);
    if (!player)
    {
        return;
    }
    
    if (player->registerStatus==PENDING)
    {
        char name[NAME_LEN+1];
        int x, y;
        char d,newline;
        if(sscanf(msg, "REG %20s %d %d %c%c", name, &x, &y, &d,&newline)<5){
            send_message(client_fd,"INVALID\n");
            return;
        }
        if (newline!='\n'||!isValidName(name)||(d!='-'&&d!='|'))
        {
            send_message(client_fd,"INVALID\n");
            return;
        }
        if (!isValidShip(x,y,d))
        {
            send_message(client_fd,"INVALID\n");
            return;
        }
        if (nameExist(players,name))
        {
            send_message(client_fd,"TAKEN\n");
            return;
        }
        Point ship_center={x,y};
        if (registerPlayer(players, name, ship_center,d ,client_fd)) {
            send_message(client_fd, "WELCOME\n");
            char join_message[100];
            snprintf(join_message,sizeof(join_message),"JOIN %s\n",name);
            broadcast_message(players, join_message);
            return;
        }
    }else{
        int x, y;
        char newline;
        if(sscanf(msg, "BOMB %d %d%c", &x, &y,&newline)<3){
            send_message(client_fd,"INVALID\n");
            return;
        }
        if (newline!='\n')
        {
            send_message(client_fd,"INVALID\n");
            return;
        }
        handle_bomb(x, y, client_fd, players,epfd);
    }
    
}

void handle_bomb(int x, int y, int attacker_fd, PlayerList *players, int epfd) {
    Point p={x,y};
    Player *attacker=findPlayerByFd(players,attacker_fd);
    if (!attacker)
    {
        return;
    }
    
    if (x<0||x>9||y<0||y>9)  //out of bounds
    {
        char miss_broadcast[100];
        snprintf(miss_broadcast,sizeof(miss_broadcast),"MISS %s %d %d\n",attacker->name,x,y);
        broadcast_message(players,miss_broadcast);
        return;
    }
    
    PlayerList *damagedList=findPlayerAtPoint(players,p);
    if (damagedList->count==0)  //hit nothing
    {
        char miss_broadcast[100];
        snprintf(miss_broadcast,sizeof(miss_broadcast),"MISS %s %d %d\n",attacker->name,x,y);
        broadcast_message(players,miss_broadcast);
        freePlayerList(damagedList);
        return;
    }
    for (int i = 0; i < damagedList->count; i++)  //traverse all hits
    {
        Player *victim=&damagedList->list[i];
        Player *victim_in_list=findPlayerByFd(players,victim->fd);
        if (!victim_in_list)
        {
            continue;
        }
        
        for (int j = 0; j < 5; j++)
        {
            Point v_point=victim_in_list->ship[j];
            if (point_equal(v_point,p))
            {
                victim_in_list->ship[j].x=-1;
                victim_in_list->ship[j].y=-1;
                break;
            }
            
        }

        char damage_report[100];
        snprintf(damage_report,sizeof(damage_report),"HIT %s %d %d %s\n",attacker->name,x,y,victim->name);
        broadcast_message(players,damage_report);
        fprintf(stderr,"%s ship is now %d,%dOO%d,%dOO%d,%dOO%d,%dOO%d,%d",victim_in_list->name,victim_in_list->ship[0].x,victim_in_list->ship[0].y,victim_in_list->ship[1].x,victim_in_list->ship[1].y,victim_in_list->ship[2].x,victim_in_list->ship[2].y,victim_in_list->ship[3].x,victim_in_list->ship[3].y,victim_in_list->ship[4].x,victim_in_list->ship[4].y,victim_in_list->ship[5].x,victim_in_list->ship[5].y);
        if (isAllDamaged(victim_in_list))
        {
            handle_gg(victim_in_list->fd,players,epfd);
        }
        
    }
    freePlayerList(damagedList);
}

//disconnect registered players
void handle_gg(int client_fd, PlayerList *players,int epfd) {
    Player *p = findPlayerByFd(players, client_fd);
    
    if (p) {
        printf("handle_gg: Player %s (fd=%d) is GG, disconnecting\n", p->name, client_fd);
        char msg[100];
        snprintf(msg, sizeof(msg), "GG %s\n", p->name);
        broadcast_message(players, msg);
        removePlayer(players, client_fd);
        epoll_ctl(epfd,EPOLL_CTL_DEL,client_fd,NULL);
        close(client_fd);
    }else {
        printf("handle_gg: Player with fd=%d not found!\n", client_fd);
    }
}

int isValidName(const char *name){
    size_t len=strlen(name);
    if (len==0||len>20)
    {
        return 0;
    }
    for (size_t i = 0; i < len; i++)
    {
        char c=name[i];
        if (!isalnum(c)&&c!='-')
        {
            return 0;
        }
        
    }
    return 1;
    
}

int isValidShip(const int x,const int y, const char d){
    if (x<0||x>9||y<0||y>9)
    {
        return 0;
    }
    if (d=='-'&&(x-2<0||x+2>9))
    {
        return 0;
    }else if (d=='|'&&(y-2<0||y+2>9))
    {
        return 0;
    }
    return 1;
    
}

#ifndef POINT_H
#define POINT_H
typedef struct point{
    int x;
    int y;
}Point;

//return 1 if equal
int point_equal(Point a, Point b);

#endif
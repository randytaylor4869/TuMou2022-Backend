#ifdef __local_test__
#include "lib_bot/Bot.h"
#else
#include <bot.h>
#endif

#include "TuMou2022/Game.h"
#include <stdio.h>
#include <stdlib.h>
#include <stack>



/*以下部分无需修改*/
Point myData[2 * MAP_SIZE - 1][2 * MAP_SIZE - 1][2 * MAP_SIZE - 1]; //可以在此存储角色探索到的地图信息
std::vector<Point> pointsInView; //角色当前视野范围内的点
std::vector<Point> pointsInAttack; //角色当前攻击范围内的点
std::vector<Point> pointsInMove; //角色当前移动范围内的点
void getInfo(const Player& player, const Map& map)
{
    pointsInView.clear();
    pointsInAttack.clear();
    pointsInMove.clear();
    for (int i = 0; i < 2 * MAP_SIZE - 1; i++)
    {
        for (int j = 0; j < 2 * MAP_SIZE - 1; j++)
        {
            for (int k = 0; k < 2 * MAP_SIZE - 1; k++)
            {
                Coordinate tmp = Coordinate(i, j, k);
                if (map.isValid(tmp))
                {
                    if (map.getDistance(tmp, player.pos) <= player.sight_range - 1)
                    {
                        pointsInView.push_back(map.data[i][j][k]);
                        myData[i][j][k] = map.data[i][j][k];
                    }
                    if (map.getDistance(tmp, player.pos) <= player.attack_range)
                    {
                        pointsInAttack.push_back(map.data[i][j][k]);
                    }
                    if (map.getDistance(tmp, player.pos) <= player.move_range)
                    {
                        pointsInMove.push_back(map.data[i][j][k]);
                    }
                }
            }
        }
    }
}
int vis[2 * MAP_SIZE - 1][2 * MAP_SIZE - 1][2 * MAP_SIZE - 1];

/*请修改本函数*/
Operation get_operation(const Player& player, const Map& map)
{
    getInfo(player, map);
    Operation op;
    vis[player.pos.x][player.pos.y][player.pos.z] = 1;
    srand(time(NULL));

    //升级部分
    int uptype = 1;
    if (player.hp <= 70)uptype = 3;
    else if (player.mine_speed <= 20)uptype = 2;
    else if (player.move_range <= 2)uptype = 0;
    else if (player.mine_speed <= 25)uptype = 2;
    else if (player.move_range <= 5)uptype = 0;
    else if (player.at <= 20)uptype = 5;
    else if (player.attack_range <= 2)uptype = 1;
    else if (player.at <= 45)uptype = 5;
    else if (player.attack_range < player.sight_range)uptype = 1;
    else uptype = 4;


    if (player.mines >= UPGRADE_COST[uptype])
    {
        op.upgrade_type = uptype;
        op.upgrade = 1;
    }


    //攻击部分
    for (int i = 0; i < pointsInAttack.size(); i++) //遇到敌方优先攻击.
    {
        if (pointsInAttack[i].PlayerIdx != player.id && pointsInAttack[i].PlayerIdx != -1)
        {
            op.type = 1;
            op.target.x = pointsInAttack[i].x;
            op.target.y = pointsInAttack[i].y;
            op.target.z = pointsInAttack[i].z;
            return op;
        }
    }


    //移动部分



    //如果移动范围内有没采完的矿，就往那走
    for (int i = 0; i < map.mine.size(); i++)
    {
        for (int j = 0;j < pointsInMove.size(); j++)
        {
            Coordinate now;
            now.x = pointsInMove[j].x;
            now.y = pointsInMove[j].y;
            now.z = pointsInMove[j].z;
            if (map.mine[i].pos.x == pointsInMove[j].x && map.mine[i].pos.y == pointsInMove[j].y && map.mine[i].pos.z == pointsInMove[j].z && map.mine[i].num > 10)
            {
                op.type = 0;
                op.target.x = now.x;
                op.target.y = now.y;
                op.target.z = now.z;
                return op;
            }
        }
    }


    //否则往地图中间走，这部分代码作用为找到第一个缩短到中心的距离的位置
    Coordinate mid;
    mid.x = 25;
    mid.y = 25;
    mid.z = 25;
    int dis = map.getDistance(mid, player.pos);
    for (int i = 0; i < pointsInMove.size(); i++)
    {
        Coordinate now;
        now.x = pointsInMove[i].x;
        now.y = pointsInMove[i].y;
        now.z = pointsInMove[i].z;
        if (pointsInMove[i].BarrierIdx == -1 && map.getDistance(mid, now) < dis)
        {
            op.type = 0;
            op.target.x = pointsInMove[i].x;
            op.target.y = pointsInMove[i].y;
            op.target.z = pointsInMove[i].z;
            return op;
        }
    }
    //这部分代码作用为找到第一个保持到中心的距离的位置
    for (int i = 0; i < pointsInMove.size(); i++)
    {
        Coordinate now;
        now.x = pointsInMove[i].x;
        now.y = pointsInMove[i].y;
        now.z = pointsInMove[i].z;
        if (pointsInMove[i].BarrierIdx == -1 && map.getDistance(mid, now) == dis)
        {
            op.type = 0;
            op.target.x = pointsInMove[i].x;
            op.target.y = pointsInMove[i].y;
            op.target.z = pointsInMove[i].z;
            return op;
        }
    }


    return op;
}

/*以下部分无需修改*/
Game* game;

int main()
{
    char* s = bot_recv();
    int side;
    int seed;
    sscanf(s, "%d%d", &side, &seed);
    free(s);
    game = new Game(side, seed);

    game->proc();

    return 0;
}
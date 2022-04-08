#include "lib_bot/Bot.h"

#include "Game.h"
#include <stdio.h>
#include <stdlib.h>


Operation get_operation_red(const Player& player, const Map& map)   // todo : SDK，玩家编写
{
    Operation op;
    int x = player.pos.x;
    int y = player.pos.y;
    int z = player.pos.z;
    int attack_range = player.attack_range;    //以下变量为Player类中的public成员，玩家可以直接从Player中获取相关
    int sight_range = player.sight_range;
    int move_range = player.move_range;
    int mine_speed = player.mine_speed;
    int at = player.at;
    int hp = player.hp;
    int mines = player.mines;

    std::vector<Mine> mine = map.mine; //以下变量为map类中的public成员，玩家可以直接从map中获取相关信息
    std::vector<Coordinate> barrier;
    int nowSize = map.nowSize;
    int viewSize = map.viewSize;
    int barrier_num = map.barrier_num;
    int enemy_num = map.enemy_num;

    //玩家可以根据以上获取的信息来进行决策
    srand(time(0));
    op.type = 0;
    if(mines >= 100) op.upgrade = 1;
    op.type = rand() % 6;
    for(int i = 0; i < 6; i++)
    {
        if(map.data[x + dx[i]][y + dy[i]][z + dz[i]].MineIdx != -1)
        {
            op.target.x = x + dx[i];
            op.target.y = y + dy[i];
            op.target.z = z + dz[i];
            return op;
        } 
    }
    for(int i = 0; i < 6; i++)
    {
        if(map.data[x + dx[i]][y + dy[i]][z + dz[i]].PlayerIdx != player.id)
        {
            for(int j = 0 ; j < map.enemy_unit.size(); j++)
            {
                if(map.enemy_unit[j].hp < player.hp && map.enemy_unit[j].at < player.at)
                {
                    op.type = 1;
                    op.target.x = map.enemy_unit[j].pos.x;
                    op.target.y = map.enemy_unit[j].pos.y;
                    op.target.z = map.enemy_unit[j].pos.z;
                    return op;
                }
                else if(map.enemy_unit[j].hp > player.hp && map.enemy_unit[j].at > player.at)
                {
                    op.type = 0;
                    op.target.x = x - dx[i];
                    op.target.y = y - dy[i];
                    op.target.z = z - dz[i];
                    return op;
                }
            }
        }
    }
    int move = rand() % 6;
    op.type = 0;
    op.target.x = x + dx[move];
    op.target.y = y + dy[move];
    op.target.z = z + dz[move];
    return op;
    
}

Operation get_operation_blue(const Player& player, const Map& map)   // todo : SDK，玩家编写
{
    Operation op;
    int x = player.pos.x;
    int y = player.pos.y;
    int z = player.pos.z;
    int attack_range = player.attack_range;    //以下变量为Player类中的public成员，玩家可以直接从Player中获取相关
    int sight_range = player.sight_range;
    int move_range = player.move_range;
    int mine_speed = player.mine_speed;
    int at = player.at;
    int hp = player.hp;
    int mines = player.mines;

    std::vector<Mine> mine = map.mine; //以下变量为map类中的public成员，玩家可以直接从map中获取相关信息
    std::vector<Coordinate> barrier;
    int nowSize = map.nowSize;
    int viewSize = map.viewSize;
    int barrier_num = map.barrier_num;
    int enemy_num = map.enemy_num;

    //玩家可以根据以上获取的信息来进行决策
    srand(time(0));
    op.type = 0;
    if(mines >= 100) op.upgrade = 1;
    op.type = rand() % 6;
    for(int i = 0; i < 6; i++)
    {
        if(map.data[x + dx[i]][x + dy[i]][x + dz[i]].MineIdx != -1)
        {
            op.target.x = x + dx[i];
            op.target.y = y + dy[i];
            op.target.z = z + dz[i];
            return op;
        } 
    }
    for(int i = 0; i < 6; i++)
    {
        if(map.data[x + dx[i]][y + dy[i]][z + dz[i]].PlayerIdx != player.id)
        {
            for(int j = 0 ; j < map.enemy_unit.size(); j++)
            {
                if(map.enemy_unit[j].hp < player.hp && map.enemy_unit[j].at < player.at)
                {
                    op.type = 1;
                    op.target.x = map.enemy_unit[j].pos.x;
                    op.target.y = map.enemy_unit[j].pos.y;
                    op.target.z = map.enemy_unit[j].pos.z;
                    return op;
                }
                else if(map.enemy_unit[j].hp > player.hp && map.enemy_unit[j].at > player.at)
                {
                    op.type = 0;
                    op.target.x = x - dx[i];
                    op.target.y = y - dy[i];
                    op.target.z = z - dz[i];
                    return op;
                }
            }
        }
    }
    int move = rand() % 6;
    op.type = 0;
    op.target.x = x + dx[move];
    op.target.y = y + dy[move];
    op.target.z = z + dz[move];
    return op;
    
}

Operation get_operation(const Player& player, const Map& map)
{
    return get_operation_red(player, map);
}

Game *game;

int main()
{
    char *s = bot_recv();
    int side;
    int seed;
    sscanf(s, "%d%d", &side, &seed);
    free(s);
    game = new Game(side, seed);

    game->proc();

    return 0;
}
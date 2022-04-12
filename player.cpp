#ifdef __local_test__
#include "lib_bot/Bot.h"
#else
#include "bot.h"
#endif

#include "Game.h"
#include <stdio.h>
#include <stdlib.h>


Operation get_operation_red(const Player& player, const Map& map)   // todo : SDK，玩家编写
{
    Operation op;
    //以下变量为Player类中的public成员，玩家可以直接从Player中获取相关
    int x = player.pos.x;
    int y = player.pos.y;
    int z = player.pos.z;    //x, y, z为角色的位置信息
    int attack_range = player.attack_range;    //角色的攻击范围
    int move_range = player.move_range;        //角色的移动范围
    int mine_speed = player.mine_speed;        //角色的采矿速度
    int at = player.at;                        //角色的攻击力
    int hp = player.hp;                        //角色的生命值
    int mines = player.mines;                  //角色的采矿数量

    //以下变量为map类中的public成员，玩家可以直接从map中获取相关信息
    std::vector<Mine> mine = map.mine;         //矿物信息
    std::vector<Coordinate> barrier;           //障碍物信息
    int nowSize = map.nowSize;                 //地图大小
    int viewSize = map.viewSize;               //视野范围
    int barrier_num = map.barrier_num;         //障碍物数量
    int enemy_num = map.enemy_num;             //敌方数量

    //玩家可以根据以上获取的信息来进行决策
    srand(time(0));
    op.type = 0;
    if(mines >= 100) op.upgrade = 1;
    op.upgrade_type = rand() % 6;
    for(int i = 0; i < 6; i++) //若遇到矿藏点，则本回合选手优先选择采矿
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
                if(map.enemy_unit[j].hp < player.hp && map.enemy_unit[j].at < player.at) //若选手遇到敌方，且选手的生命值和攻击力均大于敌方，则选择进攻
                {
                    op.type = 1;
                    op.target.x = map.enemy_unit[j].pos.x;
                    op.target.y = map.enemy_unit[j].pos.y;
                    op.target.z = map.enemy_unit[j].pos.z;
                    return op;
                }
                else if(map.enemy_unit[j].hp > player.hp && map.enemy_unit[j].at > player.at) //若选手遇到敌方，且选手的生命值和攻击力均小于敌方，则选择往反方向逃跑
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
    op.type = 0; //角色默认向远离毒圈的方向移动
    Coordinate tmp;
    Coordinate center(MAP_SIZE - 1, MAP_SIZE - 1, MAP_SIZE - 1);
    for(int i = 0; i < 6; i++)
    {
        tmp.x = x + dx[i];
        tmp.y = y + dy[i];
        tmp.z = z + dz[i];
        if(map.isValid(tmp) && map.getDistance(tmp, center) < map.getDistance(player.pos, center))
        {
            int flag = 1;
            for(int j = 0; j < map.barrier.size(); j++)
            {
                if(map.barrier[j].x == tmp.x && map.barrier[j].y == tmp.y && map.barrier[j].z == tmp.z)
                {
                    flag = 0;
                    break;
                }
            }
            if(flag == 0) continue;
            op.target = tmp;
            return op;
        }
    }
    for(int i = 0; i < 6; i++)
    {
        tmp.x = x + dx[i];
        tmp.y = y + dy[i];
        tmp.z = z + dz[i];
        if(map.isValid(tmp)) op.target = tmp;
    }
    op.target = tmp;
    return op;
}

Operation get_operation_blue(const Player& player, const Map& map)   // todo : SDK，玩家编写
{
    Operation op;
    //以下变量为Player类中的public成员，玩家可以直接从Player中获取相关
    int x = player.pos.x;
    int y = player.pos.y;
    int z = player.pos.z;    //x, y, z为角色的位置信息
    int attack_range = player.attack_range;    //角色的攻击范围
    int move_range = player.move_range;        //角色的移动范围
    int mine_speed = player.mine_speed;        //角色的采矿速度
    int at = player.at;                        //角色的攻击力
    int hp = player.hp;                        //角色的生命值
    int mines = player.mines;                  //角色的采矿数量

    //以下变量为map类中的public成员，玩家可以直接从map中获取相关信息
    std::vector<Mine> mine = map.mine;         //矿物信息
    std::vector<Coordinate> barrier;           //障碍物信息
    int nowSize = map.nowSize;                 //地图大小
    int viewSize = map.viewSize;               //视野范围
    int barrier_num = map.barrier_num;         //障碍物数量
    int enemy_num = map.enemy_num;             //敌方数量

    //玩家可以根据以上获取的信息来进行决策
    srand(time(0));
    op.type = 0;
    if(mines >= 100) op.upgrade = 1;
    op.upgrade_type = rand() % 6;
    for(int i = 0; i < 6; i++) //若遇到矿藏点，则本回合选手优先选择采矿
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
                if(map.enemy_unit[j].hp < player.hp && map.enemy_unit[j].at < player.at) //若选手遇到敌方，且选手的生命值和攻击力均大于敌方，则选择进攻
                {
                    op.type = 1;
                    op.target.x = map.enemy_unit[j].pos.x;
                    op.target.y = map.enemy_unit[j].pos.y;
                    op.target.z = map.enemy_unit[j].pos.z;
                    return op;
                }
                else if(map.enemy_unit[j].hp > player.hp && map.enemy_unit[j].at > player.at) //若选手遇到敌方，且选手的生命值和攻击力均小于敌方，则选择往反方向逃跑
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
    op.type = 0; //角色默认向远离毒圈的方向移动
    Coordinate tmp;
    Coordinate center(MAP_SIZE - 1, MAP_SIZE - 1, MAP_SIZE - 1);
    for(int i = 0; i < 6; i++)
    {
        tmp.x = x + dx[i];
        tmp.y = y + dy[i];
        tmp.z = z + dz[i];
        if(map.isValid(tmp) && map.getDistance(tmp, center) < map.getDistance(player.pos, center))
        {
            int flag = 1;
            for(int j = 0; j < map.barrier.size(); j++)
            {
                if(map.barrier[j].x == tmp.x && map.barrier[j].y == tmp.y && map.barrier[j].z == tmp.z)
                {
                    flag = 0;
                    break;
                }
            }
            if(flag == 0) continue;
            op.target = tmp;
            return op;
        }
    }
    srand(time(NULL));
    int randomMove = rand() % 6;
    for(int i = 0; i < 6; i++)
    {
        tmp.x = x + dx[i];
        tmp.y = y + dy[i];
        tmp.z = z + dz[i];
        if(map.isValid(tmp)) op.target = tmp;
    }
    op.target = tmp;
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
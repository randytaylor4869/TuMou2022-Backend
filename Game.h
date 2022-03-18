#pragma once
#define _CRT_SECURE_NO_WARNINGS
#ifndef GAME_H
#define GAME_H

#include "Map.h"
#include "Player.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

const int UPGRADE_COST[6] = { 100, 100, 100, 100, 100, 100}; // todo：确定升级类型所需要的mine数

class Operation
{
public:
    int type; // 移动(0) 或 攻击(1) 或 none(-1)
    Coordinate target;  // 移动/攻击目标

    bool upgrade; //是否升级
    int upgrade_type; //升级类型：0-移动速度， 1-攻击力， 2-采集速度， 3-回血，4-视野，5-生命值

    Operation() : type(-1) {};
};



Operation get_operation_red(const Player& player, const Map& map);   // todo : SDK，玩家编写
Operation get_operation_blue(const Player& player, const Map& map);   // todo : SDK，玩家编写
//这样写其实就不用写通信了


class Game
{
    Map map;
	Player player_red=Player(0,MAP_SIZE-1,2*MAP_SIZE-2,0);    
	Player player_blue=Player(1,MAP_SIZE,0,2*MAP_SIZE-2);
	//to do: 更改player初始位置
    // Player ai[2];
    int turn;
    
    void init_map()			//已在Map的构造函数中实现
    {
        // Barrier
        // Mine
        // 玩家位置！
    }

    //Map limited_map(const Player& p)
    //{
    //    //考虑玩家p的视野
    //    return map;
    //}

	Map limited_map = Map(map,player_red);		//返回玩家p的视野地图

    Operation regulate(Operation op, Player p)
    {
        Operation ret;
        //若不合法，一律返回什么都不做none(-1)

        // 检查type在范围内
        if(op.type < -1 || op.type > 1)
            return ret;
        
        // 检查target在地图范围内、在攻击/移动距离内
        if(op.type == 0)
        {
            if(!map.isValid(op.target))
                return ret;
            if(map.getDistance(op.target, p.pos) > p.move_range)
                return ret;
        }
        else if(op.type == 1)
        {
            if(!map.isValid(op.target))
                return ret;
            if(map.getDistance(op.target, p.pos) > p.attack_range)
                return ret;
        }

        //前10回合攻击保护
        if(op.type == 1)
        {
            if(turn <= 10) // todo : 10?
                return ret;
        }

        //检查移动的终点是否合法
        /*
        if(op.type == 0)
        {
            if(map[op.target].hasBarrier)
                return ret;
            //TODO:检查是否有敌方玩家
        }
        */

        // 检查升级时机（回合数）是否合法
        // TBD
        
        return op;
    }

    void upgrade(int type, Player& p)
    {
        switch(type)
        {
            case 0:
                p.move_range++;
                break;
            case 1:
                p.attack_range++;
                break;
            case 2:
                p.mine_speed++;
                break;
            case 3:
                p.hp = (p.hp+50 > 100 ? 100 : p.hp+50); //血量上限100
                // TODO: += f(turn) ?
                break;
            case 4:
                p.sight_range++;
                break;
            default:
                break;
        }
    }
public:
    Game(unsigned int seed = 0)
    {
        if(seed == 0)
            seed = srand(time(0));
        
        turn = 0;
        init_map();

        // TODO : replay output
    }
    bool Update() //进行一个回合：若有一方死亡，游戏结束，返回true，否则返回false
    {
        Operation op;
        int mine_get;
        // update采矿收入
        if(map[player_red.pos].MineIdx != -1)
        {
            if(map.mine[map[player_red.pos].MineIdx].available && map.mine[map[player_red.pos].MineIdx].belong != -1)
            {
                mine_get = map.mine[map[player_red.pos].MineIdx].num;
                if(mine_get > player_red.mine_speed)
                    mine_get = player_red.mine_speed;
                player_red.mines += mine_get;
                map.mine[map[player_red.pos].MineIdx].num -= mine_get;
            }
        }
        if(map[player_blue.pos].MineIdx != -1)
        {
            if(map.mine[map[player_blue.pos].MineIdx].available && map.mine[map[player_blue.pos].MineIdx].belong != 1)
            {
                mine_get = map.mine[map[player_blue.pos].MineIdx].num;
                if(mine_get > player_blue.mine_speed)
                    mine_get = player_blue.mine_speed;
                player_blue.mines += mine_get;
                map.mine[map[player_blue.pos].MineIdx].num -= mine_get;
            }
        }

        // get operation from player red(0)
        //op = get_operation_red(player_red, limited_map(player_red));
		op = get_operation_red(player_red, Map(map,player_red));
        op = regulate(op);
        // todo: REPLAY 输出op red
        //更新移动相关（位置）
        if(op.type == 0)
        {
            player_red.pos = op.target;
        }
        //更新攻击相关（血量）
        else if(op.type == 1)
        {
            if(op.target == player_blue.pos)
            {
                player_blue.hp -= player_red.at;
            }
        }
        //更新升级，注意这里升级是在行动之后（可以改顺序)，应当声明
        if(op.upgrade)
        {
            if(op.upgrade_type >= 0 && op.upgrade_type <= 5) // 检查升级类型是否合法
                if(player_red.mines >= UPGRADE_COST[op.upgrade_type]) // 检查资源是否足够
                {
                    upgrade(op.upgrade_type, player_red);
                }
            // todo : 调节升级资源数 
        }

        //判断存活状态
        if(player_blue.hp <= 0)
            return true;


        // get operation from player blue(1)
        op = get_operation_blue(player_red, limited_map(player_blue));
        op = regulate(op);
        // todo: REPLAY 输出op blue
        //更新移动相关（位置）
        if(op.type == 0)
        {
            player_blue.pos = op.target;
        }
        //更新攻击相关（血量）
        else if(op.type == 1)
        {
            if(op.target == player_red.pos)
            {
                player_red.hp -= player_blue.at;
            }
        }
        //更新升级，注意这里升级是在行动之后（可以改顺序)，应当声明
        if(op.upgrade)
        {
            if(op.upgrade_type >= 0 && op.upgrade_type <= 5) // 检查升级类型是否合法
                if(player_blue.mines >= UPGRADE_COST[op.upgrade_type]) // 检查资源是否足够
                {
                    upgrade(op.upgrade_type, player_blue);
                }
            // todo : 调节升级资源数 
        }

        //判断存活状态
        if(player_red.hp <= 0)
            return true;

        //缩圈
        if(turn >= 0.75 * TURN_COUNT && turn % 5 == 0)
        {
            if(map.nowSize)
                map.nowSize--;
        }
        //缩圈伤害
        if(map.getDistance(player_red.pos, Coordinate(0,0,0)) > map.nowSize)
        {
            player_red.hp -= 10; // TODO: 调整这个值，随时间变化？
        }
        if(map.getDistance(player_blue.pos, Coordinate(0,0,0)) > map.nowSize)
        {
            player_blue.hp -= 10; // TODO: 调整这个值，随时间变化？
        }

        //判断存活状态
        if(player_blue.hp <= 0 || player_red.hp <= 0)
            return true;
        
        return false;
    }

    int proc() // 对局入口，返回胜者编号red(0), blue(1)
    {
        for(turn = 1; turn <= TURN_COUNT; turn++)
            if(Update())
                break;
        //终局结算
        //（todo：伤害结算顺序？如果同时死亡，如何计算？）
        if(player_red.hp <= 0)
        {
            // cout << "Game ends in turn " << turn << " : player blue wins!" << endl;
            return 1;
        }
        if(player_blue.hp <= 0)
        {
            // cout << "Game ends in turn " << turn << " : player red wins!" << endl;
            return 0;
        }
        return -1; // 出错，没有死亡
         // cout << "Game ends in turn " << turn << " : Error, no one die but stop." << endl;
        //输出到REPLAY
        
    }

};

#endif
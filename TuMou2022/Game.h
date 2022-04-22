#pragma once
#define _CRT_SECURE_NO_WARNINGS
#ifndef GAME_H
#define GAME_H

#include "Map.h"
#include "Player.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include "json.hpp"
//#include <bot.h>
using json = nlohmann::json;

const int UPGRADE_COST[6] = { 30, 30, 5, 40, 30, 20 }; // todo：确定升级类型所需要的mine数

class Operation
{
public:
	int type; // 移动(0) 或 攻击(1) 或 none(-1)
	Coordinate target;  // 移动/攻击目标

	int upgrade; //是否升级
	int upgrade_type; //升级类型：0-移动速度， 1-攻击范围， 2-采集速度， 3-回血，4-视野范围，5-攻击力
	Operation() : type(-1), upgrade(0) {};
};



//Operation get_operation_red(const Player& player, const Map& map);   // todo : SDK，玩家编写
//Operation get_operation_blue(const Player& player, const Map& map);   // todo : SDK，玩家编写
//这样写其实就不用写通信了

Operation get_operation(const Player& player, const Map& map);   // todo : SDK，玩家编写


class Game
{
	const int player_id;

	Player player_red = Player(0, MAP_SIZE - 1, 2 * MAP_SIZE - 2, 0);   
	Player player_blue = Player(1, MAP_SIZE - 1, 0, 2 * MAP_SIZE - 2);

	Map map;
    Map mymap; // 用于player_map()函数中返回值，向选手传参

	/*暂时去掉
	map.enemy_unit.push_back(&player_red);
	map.enemy_unit.push_back(&player_blue);
	map.enemy_num+=2;
	*/
	int turn;
public:
	json m_root;//用于写json向前端汇报的根节点
	json m_init;
	//to do: 更改player初始位置
	// Player ai[2];

	Map buildMap(unsigned seed = 0)		//初始化地图
	{
		Map mymap = Map();
		mymap.mine_num = 0;
		mymap.barrier_num = 0;
		mymap.enemy_num = 0;
		srand(seed);
		for (int i = 0; i < 2 * MAP_SIZE - 1; i++) {
			for (int j = 0; j < 2 * MAP_SIZE - 1; j++) {
				for (int k = 0; k < 2 * MAP_SIZE - 1; k++) {
					if (mymap.isValid(i, j, k))
					{
						//随机判断是否有资源点、障碍物
						int mineidx = -1;
						int barrieridx = -1;
						if (rand() % 10 == 1) { mineidx = mymap.mine_num; }				//十分之一概率， TO DO：修改数值
						else if (rand() % 10 == 2) { barrieridx = mymap.barrier_num; }	//十分之一概率， TO DO：修改数值

						if (mymap.isValid(i, j, k))
						{
							mymap.data[i][j][k] = Point(i, j, k, mineidx, barrieridx, 1);
							mymap.data[i][j][k].isvalid = 1;
							if (mineidx >= 0)
							{
								mymap.mine.push_back(Mine(MINE_NUM, Coordinate(i, j, k)));
								mymap.data[i][j][k].MineIdx = mymap.mine_num;
								mymap.mine_num++;
							}
							if (barrieridx >= 0)
							{
								mymap.barrier.push_back(Coordinate(i, j, k));
								mymap.data[i][j][k].BarrierIdx = mymap.barrier_num;
								mymap.barrier_num++;
							}
						}


					}
				}
			}
		}
		mymap.enemy_unit.push_back(player_red);
		mymap.enemy_unit.push_back(player_blue);
		mymap.data[player_red.pos.x][player_red.pos.y][player_red.pos.z].PlayerIdx = player_red.id;
		mymap.data[player_blue.pos.x][player_blue.pos.y][player_blue.pos.z].PlayerIdx = player_blue.id;
		return mymap;
	}

	const Map& player_map(const Player& player)
	{
		//todo:add
		// map.ememy_unit.clear()
		// if(dist ...) map.enemy_unit.push_back(&player_red);
		// mine ...
		mymap = map;
		mymap.enemy_unit.clear();
		mymap.mine.clear();
		mymap.barrier.clear();
		mymap.enemy_num = 0;
		mymap.mine_num = 0;
		mymap.barrier_num = 0;

		mymap.viewSize = player.sight_range;

		for (int i = 0; i < 2 * MAP_SIZE - 1; i++) {
			for (int j = 0; j < 2 * MAP_SIZE - 1; j++) {
				for (int k = 0; k < 2 * MAP_SIZE - 1; k++) {
					if (mymap.data[i][j][k].isvalid == 1)
					{
						mymap.data[i][j][k].MineIdx = -1;
						mymap.data[i][j][k].BarrierIdx = -1;
						mymap.data[i][j][k].PlayerIdx = -1;
					}
				}
			}
		}

		for (auto i : map.mine)
		{

			if (mymap.getDistance(player.pos, i.pos) <= mymap.viewSize - 1)
			{
				mymap.mine.push_back(i);
				mymap.data[i.pos.x][i.pos.y][i.pos.z].MineIdx = mymap.mine_num++;
			}
		}
		for (auto i : map.barrier)
		{
			if (mymap.getDistance(player.pos, i) <= mymap.viewSize - 1)
			{
				mymap.barrier.push_back(i);
				mymap.data[i.x][i.y][i.z].BarrierIdx = mymap.barrier_num++;
			}
		}
		for (auto i : map.enemy_unit)
		{
			if (mymap.getDistance(player.pos, i.pos) <= mymap.viewSize - 1)
			{
				mymap.enemy_unit.push_back(i);
				mymap.data[i.pos.x][i.pos.y][i.pos.z].PlayerIdx = i.id;
				mymap.enemy_num++;
			}
		}


		return mymap;
	}

	void init_json()			//已在Map的构造函数中实现
	{
		// Barrier
		// Mine
		// 玩家位置！
		//输出json文件
		json root;
		json maps;
		for (int i = 0; i < 3; i++)
		{
			maps["size"].push_back(map.nowSize);
		}
		for (int i = 0; i < map.barrier_num; i++)
		{
			json tmp_array;
			tmp_array.push_back(map.barrier[i].x);
			tmp_array.push_back(map.barrier[i].y);
			tmp_array.push_back(map.barrier[i].z);
			maps["barriers"].push_back(tmp_array);
		}
		for (int i = 0; i < map.mine_num; i++)
		{
			json tmp_array;
			tmp_array.push_back(map.mine[i].pos.x);
			tmp_array.push_back(map.mine[i].pos.y);
			tmp_array.push_back(map.mine[i].pos.z);
			tmp_array.push_back(map.mine[i].num);
			maps["resources"].push_back(tmp_array);
		}
		root["map"] = maps;
		json players;
		json item;
		item["id"] = 0;
		item["name"] = "Player1";
		item["attack_range"] = player_red.attack_range;
		item["sight_range"] = player_red.sight_range;
		item["move_range"] = player_red.move_range;
		item["mine_speed"] = player_red.mine_speed;
		item["atk"] = player_red.at;
		item["hp"] = player_red.hp;
		item["Position"].push_back(player_red.pos.x);
		item["Position"].push_back(player_red.pos.y);
		item["Position"].push_back(player_red.pos.z);
		players.push_back(item);
		item["id"] = 1;
		item["name"] = "Player2";
		item["attack_range"] = player_blue.attack_range;
		item["sight_range"] = player_blue.sight_range;
		item["move_range"] = player_blue.move_range;
		item["mine_speed"] = player_blue.mine_speed;
		item["atk"] = player_blue.at;
		item["hp"] = player_blue.hp;
		item["Position"].clear();
		item["Position"].push_back(player_blue.pos.x);
		item["Position"].push_back(player_blue.pos.y);
		item["Position"].push_back(player_blue.pos.z);
		players.push_back(item);
		root["players"] = players;
		m_init = root;
	}

	json reportEvent(int id, Coordinate pos)
	{
		json current;
		current["Round"] = turn;
		current["CurrentEvent"];
		current["ActivePlayerId"] = id;
		current["VictimId"] = json::array();
		current["ActivePos"].push_back(pos.x);
		current["ActivePos"].push_back(pos.y);
		current["ActivePos"].push_back(pos.z);
		current["Exp"] = -1;
		for (int i = 0; i < 3; i++)
			current["MapSize"].push_back(map.nowSize);
		current["MinesLeft"] = -1;
		current["UpgradeType"] = -1;
		current["BoundaryHurt"] = -1;
		current["WinnerId"] = -1;
		return current;
	}
	//Map limited_map(const Player& p)
	//{
	//    //考虑玩家p的视野
	//    return map;
	//}

	//Map limited_map = Map(map,player_red);		//返回玩家p的视野地图

	Operation regulate(Operation op, const Player& p)
	{
		Operation ret;
		ret.upgrade = op.upgrade;
		ret.upgrade_type = op.upgrade_type;
		//若不合法，一律返回什么都不做none(-1)

		// 检查type在范围内
		if (op.type < -1 || op.type > 1)
			return ret;

		// 检查target在地图范围内、在攻击/移动距离内
		if (op.type == 0)
		{
			if (!map.isValid(op.target))
            {
                return ret;
            }
			if (map.getDistance(op.target, p.pos) > p.move_range)
				return ret;
			if (map.data[op.target.x][op.target.y][op.target.z].BarrierIdx != -1 || map.data[op.target.x][op.target.y][op.target.z].PlayerIdx != -1)
				return ret;
		}
		else if (op.type == 1)
		{
			if (!map.isValid(op.target))
				return ret;
			if (map.getDistance(op.target, p.pos) > p.attack_range)
				return ret;
		}
		//前10回合攻击保护
		if (op.type == 1)
		{
			if (turn <= 10) // todo : 10?
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
        //std::cerr << "valid move " << op.type << ' ' << map.getDistance(p.pos,op.target) << "\n";
		return op;
	}

	void upgrade(int type, Player& p)
	{
		switch (type)
		{
		case 0:
			p.move_range++;
			p.mines -= UPGRADE_COST[type];
			break;
		case 1:
			p.attack_range++;
			p.mines -= UPGRADE_COST[type];
			break;
		case 2:
			p.mine_speed += 5;
			p.mines -= UPGRADE_COST[type];
			break;
		case 3:
			p.hp = (p.hp + 30 > 100 ? 100 : p.hp + 30); //血量上限100
			p.mines -= UPGRADE_COST[type];
			// TODO: += f(turn) ?
			break;
		case 4:
			p.sight_range++;
			p.mines -= UPGRADE_COST[type];
			break;
		case 5:
			p.at += 5;
			p.mines -= UPGRADE_COST[type];
			break;
		default:
			break;
		}
	}
	Operation get_operation_opponent()
	{
		Operation op;
		char *s = bot_recv();
		sscanf(s, "%d %d %d %d %d %d", &op.type, &op.target.x, &op.target.y, &op.target.z, &op.upgrade, &op.upgrade_type);
		free(s);
		return op;
	}
	void send_operation(Operation op)
	{
		char s[100];
		sprintf(s, "%d %d %d %d %d %d", op.type, op.target.x, op.target.y, op.target.z, op.upgrade, op.upgrade_type);
		bot_send(s);
		return;
	}
	Operation judge_proc(int current_player, int* err)
	{
		int o_len;
		char* s = bot_judge_recv(current_player, &o_len, 2000);
		*err = 0;
		if(s == 0)
		{
			*err = o_len;
			return Operation();
		}
		bot_judge_send(current_player^1, s);
		Operation op;
		sscanf(s, "%d %d %d %d %d %d", &op.type, &op.target.x, &op.target.y, &op.target.z, &op.upgrade, &op.upgrade_type);
		free(s);
		return op;
		// TODO: 根据err判断选手程序是否正常返回操作
	}

	bool Update(int* err) //进行一个回合：若有一方死亡，游戏结束，返回true，否则返回false
	{
		Operation op;
		int mine_get;

		// update采矿收入
		if (map[player_red.pos].MineIdx != -1)
		{
			if (map.mine[map[player_red.pos].MineIdx].available && map.mine[map[player_red.pos].MineIdx].belong != -1)
			{
				mine_get = map.mine[map[player_red.pos].MineIdx].num;
				if (mine_get > player_red.mine_speed)
					mine_get = player_red.mine_speed;
				player_red.mines += mine_get;
				map.mine[map[player_red.pos].MineIdx].num -= mine_get;
				if (map.mine[map[player_red.pos].MineIdx].num == 0)
				{
					map.mine[map[player_red.pos].MineIdx].available = false;
					map.mine[map[player_red.pos].MineIdx].belong = -1;
				}

				json event = reportEvent(0, player_red.pos);
				event["CurrentEvent"] = "GATHER";
				event["Exp"] = mine_get;
				event["MinesLeft"] = map.mine[map[player_red.pos].MineIdx].num;
				m_root.push_back(event);
			}
		}
		if (map[player_blue.pos].MineIdx != -1)
		{
			if (map.mine[map[player_blue.pos].MineIdx].available && map.mine[map[player_blue.pos].MineIdx].belong != 1)
			{
				mine_get = map.mine[map[player_blue.pos].MineIdx].num;
				if (mine_get > player_blue.mine_speed)
					mine_get = player_blue.mine_speed;
				player_blue.mines += mine_get;
				map.mine[map[player_blue.pos].MineIdx].num -= mine_get;
				if (map.mine[map[player_blue.pos].MineIdx].num == 0)
				{
					map.mine[map[player_blue.pos].MineIdx].available = false;
					map.mine[map[player_blue.pos].MineIdx].belong = -1;
				}
				json event = reportEvent(1, player_blue.pos);
				event["CurrentEvent"] = "GATHER";
				event["Exp"] = mine_get;
				event["MinesLeft"] = map.mine[map[player_blue.pos].MineIdx].num;
				m_root.push_back(event);
			}
		}

		// get operation from player red(0)
		//op = get_operation_red(player_red, limited_map(player_red));
		// real: 
		//Map(map,player_red);

		//op = get_operation_red(player_red, tmp);
		if(player_id == 0)
		{
			op = get_operation(player_red, player_map(player_red)); // 暂时！
			send_operation(op);
		}
		else if(player_id == 1)
			op = get_operation_opponent();
		else // judge -1
		{
			op = judge_proc(0, err);
			if(*err)
				return true;
			// todo : process err
		}

		op = regulate(op, player_red);

		if (op.type == -1)
		{
			json event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "ERROR";
			m_root.push_back(event);
		}
		// todo: REPLAY 输出op red



		//更新移动相关（位置）
		else if (op.type == 0)
		{
			map.data[player_red.pos.x][player_red.pos.y][player_red.pos.z].PlayerIdx = -1;
			player_red.pos = op.target;
			map.data[player_red.pos.x][player_red.pos.y][player_red.pos.z].PlayerIdx = 0;
			json event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "MOVE";
			m_root.push_back(event);
			for (auto i : map.enemy_unit)
			{
				if (i.id != player_red.id)
				{
					continue;
				}
				else
				{
					i.pos = player_red.pos;
					break;
				}
			}
		}

		//更新攻击相关（血量）
		else if (op.type == 1)
		{
			if (op.target == player_blue.pos)
			{
				player_blue.hp -= player_red.at;
				json event = reportEvent(0, player_red.pos);
				event["CurrentEvent"] = "ATTACK";
				event["VictimId"].push_back(player_blue.id);// todo: 由于目前没实现中立单位，故只添加了blue的id
				m_root.push_back(event);
			}
		}
		//更新升级，注意这里升级是在行动之后（可以改顺序)，应当声明
		if (op.upgrade)
		{
			if (op.upgrade_type >= 0 && op.upgrade_type <= 5) // 检查升级类型是否合法
				if (player_red.mines >= UPGRADE_COST[op.upgrade_type]) // 检查资源是否足够
				{
					if(!(op.upgrade_type == 2 && player_red.mine_speed >= 30) &&
					   !(op.upgrade_type == 5 && player_red.at >= 50) )
					{
						upgrade(op.upgrade_type, player_red);
						json event = reportEvent(0, player_red.pos);
						event["CurrentEvent"] = "UPGRADE";
						switch (op.upgrade_type)
						{
						case 0:
							event["UpgradeType"] = "move_range";
							break;
						case 1:
							event["UpgradeType"] = "attack_range";
							break;
						case 2:
							event["UpgradeType"] = "mine_speed";
							break;
						case 3:
							event["UpgradeType"] = "hp";
							break;
						case 4:
							event["UpgradeType"] = "sight_range";
							break;
						case 5:
							event["UpgradeType"] = "atk";
							break;
						default:
							break;
						}
						m_root.push_back(event);
					}
				}
			// todo : 调节升级资源数 
		}

		//判断存活状态
		if (player_blue.hp <= 0)
			return true;


		// get operation from player blue(1)
		//real: op = get_operation_blue(player_red, Map(map,player_blue));
		
		if(player_id == 1)
		{
			op = get_operation(player_blue, player_map(player_blue));
			send_operation(op);
		}
		else if(player_id == 0)
			op = get_operation_opponent();
		else // judge -1
		{
			op = judge_proc(1, err);
			if(*err)
			{
				*err *= -1;
				return true;
			}
		}
		
		op = regulate(op, player_blue);
		if (op.type == -1)
		{
			json event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "ERROR";
			m_root.push_back(event);
		}
		// todo: REPLAY 输出op blue
		//更新移动相关（位置）
		if (op.type == 0)
		{
			map.data[player_blue.pos.x][player_blue.pos.y][player_blue.pos.z].PlayerIdx = -1;
			player_blue.pos = op.target;
			map.data[player_blue.pos.x][player_blue.pos.y][player_blue.pos.z].PlayerIdx = 1;
			json event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "MOVE";
			m_root.push_back(event);
			for (auto i : map.enemy_unit)
			{
				if (i.id != player_blue.id)
				{
					continue;
				}
				else
				{
					i.pos = player_blue.pos;
					break;
				}
			}
		}
		//更新攻击相关（血量）
		else if (op.type == 1)
		{
			if (op.target == player_red.pos)
			{
				player_red.hp -= player_blue.at;
				json event = reportEvent(1, player_blue.pos);
				event["CurrentEvent"] = "ATTACK";
				event["VictimId"].push_back(player_red.id);// todo: 由于目前没实现中立单位，故只添加了red的id
				m_root.push_back(event);
			}
		}
		//更新升级，注意这里升级是在行动之后（可以改顺序)，应当声明
		if (op.upgrade)
		{
			if (op.upgrade_type >= 0 && op.upgrade_type <= 5) // 检查升级类型是否合法
				if (player_blue.mines >= UPGRADE_COST[op.upgrade_type]) // 检查资源是否足够
				{
					upgrade(op.upgrade_type, player_blue);
					json event = reportEvent(1, player_blue.pos);
					event["CurrentEvent"] = "UPGRADE";
					switch (op.upgrade_type)
					{
					case 0:
						event["UpgradeType"] = "move_range";
						break;
					case 1:
						event["UpgradeType"] = "attack_range";
						break;
					case 2:
						event["UpgradeType"] = "mine_speed";
						break;
					case 3:
						event["UpgradeType"] = "hp";
						break;
					case 4:
						event["UpgradeType"] = "sight_range";
						break;
					case 5:
						event["UpgradeType"] = "atk";
						break;
					default:
						break;
					}
					m_root.push_back(event);
				}
			// todo : 调节升级资源数 
		}

		//判断存活状态
		if (player_red.hp <= 0)
			return true;

		//缩圈
		if (turn >= 0.75 * TURN_COUNT )
		{
			if (map.nowSize)
				map.nowSize--;
		}
		//缩圈伤害
		if (map.getDistance(player_red.pos, Coordinate(MAP_SIZE-1, MAP_SIZE-1, MAP_SIZE-1)) > map.nowSize)
		{
			int hurt = 10;// TODO: 调整这个值，随时间变化？
			player_red.hp -= hurt; 
			json event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "BOUNDARYHURT";
			event["BoundaryHurt"] = hurt;
			m_root.push_back(event);
			
		}
		if (map.getDistance(player_blue.pos, Coordinate(MAP_SIZE-1, MAP_SIZE-1, MAP_SIZE-1)) > map.nowSize)
		{
			int hurt = 10;// TODO: 调整这个值，随时间变化？
			player_blue.hp -= hurt; 
			json event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "BOUNDARYHURT";
			event["BoundaryHurt"] = hurt;
			m_root.push_back(event);
		}

		//判断存活状态
		if (player_blue.hp <= 0 || player_red.hp <= 0)
			return true;

		return false;

	}
public:
	Game() : player_id (-2) // -2 stands for uninitialized
	{
		turn = 0; 
		init_json();
		// TODO : replay output
	}
	Game(int x, int y) : player_id(x), map(buildMap(y)){
		turn = 0;
		init_json();
	}

	int proc() // 对局入口，返回胜者编号red(0), blue(1)
	{
		init_json();
		int err;
		for (turn = 1; turn <= TURN_COUNT; turn++)
		{
			//std::cerr << "turn" << turn << std::endl;
            //std::cerr << player_red.hp << ' '<< player_blue.hp << std::endl;
            //std::cerr << map.getDistance(player_red.pos, Coordinate(MAP_SIZE-1, MAP_SIZE-1, MAP_SIZE-1)) << ' ' << map.getDistance(player_blue.pos, Coordinate(MAP_SIZE-1, MAP_SIZE-1, MAP_SIZE-1)) << ' ' << map.nowSize << std::endl;
			
			if (Update(&err))
				break;
		}
		if(err > 0) // 红方出错结束
		{
			std::cerr << "Player red error. Code : " << err << std::endl;
			//todo: json output
			json event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "TIMEOUT";
			event["WinnerId"] = player_blue.id;
			m_root.push_back(event);
			return 1;
		}
		else if (err < 0) // 蓝方出错结束
		{
			std::cerr << "Player blue error. Code : " << err << std::endl;
			//todo: json output
			json event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "TIMEOUT";
			event["WinnerId"] = player_red.id;
			m_root.push_back(event);
			return 0;
		}
		//终局结算
		else if (player_red.hp <= 0)
		{
			json event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "DIED";
			event["WinnerId"] = player_blue.id;
			m_root.push_back(event);
			std::cerr << "Game ends in turn " << turn << " : player blue wins!" << std::endl;
			return 1;
		}
		else if (player_blue.hp <= 0)
		{
			json event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "DIED";
			event["WinnerId"] = player_red.id;
			m_root.push_back(event);
			std::cerr << "Game ends in turn " << turn << " : player red wins!" << std::endl;
			return 0;
		}
		else if (player_red.hp < player_blue.hp)
		{
			json event = reportEvent(1, player_blue.pos);
			event["CurrentEvent"] = "ENDGAME";
			event["WinnerId"] = player_blue.id;
			m_root.push_back(event);
			std::cerr << "Game ends in turn " << turn << " : player blue wins!" << std::endl;
			return 1;
		}
		else if (player_red.hp > player_blue.hp)
		{
			json event = reportEvent(0, player_red.pos);
			event["CurrentEvent"] = "ENDGAME";
			event["WinnerId"] = player_red.id;
			m_root.push_back(event);
			std::cerr << "Game ends in turn " << turn << " : player red wins!" << std::endl;
			return 0;
		}
		else
		{
			json event = reportEvent(2, player_red.pos);//no winner!!
			event["CurrentEvent"] = "ENDGAME";
			event["WinnerId"] = 2;
			m_root.push_back(event);
			std::cerr << "Game ends in turn " << turn << " : draw!" << std::endl;
			return 2;
		}
		return -1;
	}

};

#endif
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#ifndef PLAYER_H
#define PLAYER_H

#include"Coordinate.h"

class Player
{
public:
	int id;

	Coordinate pos;

	int attack_range;   //攻击范围
	int sight_range; //视野范围
	int move_range; //移动范围
	int mine_speed; //采集速度

	int at; //攻击力
	int hp; //血量

	int mines;  // 采集到的资源

	Player(int _id) : id(_id), attack_range(1), sight_range(6), move_range(1), mine_speed(10),
		at(10), hp(100),mines(0) {};  //pos at (0,0,0)

	Player(int _id, int x, int y, int z) : id(_id), attack_range(1), sight_range(6), move_range(1), mine_speed(10),
		at(10), hp(100), mines(0), pos(Coordinate(x,y,z)) {};  //pos at (0,0,0)
	Player(int _id, Coordinate c) : id(_id), attack_range(1), sight_range(6), move_range(1), mine_speed(10),
		at(10), hp(100), mines(0), pos(c) {};  //pos at (0,0,0)

	// todo : 调节初始化数据

};


#endif
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#ifndef MAP_H
#define MAP_H

#include "Player.h"
#include<string.h>
#include<vector>
#include <cstdlib>
#include <cmath>

const int TURN_COUNT = 36 * 4;//回合数
const int MAP_SIZE = 50;//地图的边长
const int MINE_NUM = 100;

const int oo = 2e9;

enum Sign { RED, BLUE, TIE };//玩家势力，红/蓝
enum direction { N, NE, SE, S, SW, NW };//方向:

const int dx[6] = { 1, 1, 0, -1, -1, 0 }, dy[6] = { 0, -1, -1, 0, 1, 1 }, dz[6] = { -1, 0, 1, 1, 0, -1 };

class Point
{//坐标点类
public:
	int x;
	int y;
	int z;
	int MineIdx = -1;	//资源点的序号，-1代表没有资源点
	int BarrierIdx = 0;	//障碍物的序号，-1代表没有障碍物
	int PlayerIdx = -1;	   //玩家的编号，-1代表没有玩家， 0红方，1蓝方
	bool isvalid = 0;				//是否在地图内

	Point(const int _x = -1000, const int _y = -1000, const int _z = -1000, const int _mineindex = -1, const int _barrieridx = -1, const bool isvalid = 0) {
		x = _x;
		y = _y;
		z = _z;
		MineIdx = _mineindex;
		BarrierIdx = _barrieridx;
	}
};


class Mine {	//资源点
public:
	//int weight, available, belong, initheight;
	Coordinate pos;
	int num, belong = 0;   //belong=0为中立,-1蓝方，1红方
	bool available = 1;
	Mine(const int n, Coordinate x) { num = n; pos = x; }
};



struct Coordinate
{
	int x, y, z;
	Coordinate() { x = 0, y = 0, z = 0; }
	Coordinate(int a, int b, int c) { x = a, y = b, z = c; }

	bool operator==(Coordinate t)
	{
		if (this->x == t.x && this->y == t.y && this->z == t.z)
		{
			return true;
		}
		return false;
	}
};


class Map {
public:
	Point data[2 * MAP_SIZE - 1][2 * MAP_SIZE - 1][2 * MAP_SIZE - 1];		//地图信息，data[i][j][k]表示地图一个位置的信息

	std::vector <Mine> mine;
	std::vector<Coordinate> barrier;
	std::vector<Player> enemy_unit;
	int nowSize = MAP_SIZE;			//当前毒圈的边长
	int viewSize = MAP_SIZE;
	int mine_num = 0;
	int barrier_num = 0;
	int enemy_num = 0;


	Map()		//初始化地图
	{
		mine_num = 0;
		barrier_num = 0;
		enemy_num = 0;
		unsigned seed;
		srand(seed);
		for (int i = 0; i < 2 * MAP_SIZE - 1; i++)
			for (int j = 0; j < 2 * MAP_SIZE - 1; j++)
				for (int k = 0; k < 2 * MAP_SIZE - 1; k++) {
					if (isValid(i, j, k))
					{
						//随机判断是否有资源点、障碍物
						int mineidx = -1;
						int barrieridx = -1;
						if (rand() % 10 == 1) { mineidx = mine_num; }				//十分之一概率， TO DO：修改数值
						else if (rand() % 10 == 2) { barrieridx = barrier_num; }	//十分之一概率， TO DO：修改数值

						if (isValid(i, j, k))
						{
							data[i][j][k] = Point(i, j, k, mineidx, barrieridx, 1);
							if (mineidx >= 0)
							{
								mine.push_back(Mine(MINE_NUM, Coordinate(i, j, k)));
								mine_num++;
							}
							if (barrieridx >= 0)
							{
								barrier.push_back(Coordinate(i, j, k));
								barrier_num++;
							}
						}


					}
				}
	}


	Map(Map totalmap, const Player& p)			//返回玩家p的视野地图,传递给选手		
	{
		*this = totalmap;
		viewSize = p.sight_range;
		for (auto i : totalmap.mine)
		{

			if (getDistance(p.pos, i.pos) <= viewSize - 1)
			{
				mine.push_back(i);
				data[i.pos.x][i.pos.y][i.pos.z].MineIdx = mine_num++;
			}
		}
		for (auto i : totalmap.barrier)
		{
			if (getDistance(p.pos, i) <= viewSize - 1)
			{
				barrier.push_back(i);
				data[i.x][i.y][i.z].BarrierIdx = barrier_num++;
			}
		}
		for (auto i : totalmap.enemy_unit)
		{
			if (getDistance(p.pos, i.pos) <= viewSize - 1)
			{
				enemy_unit.push_back(i);
				data[i.pos.x][i.pos.y][i.pos.z].PlayerIdx = i.id;
				enemy_num++;
			}
		}
	}


	bool isValid(int x, int y, int z)		//判断某个坐标是否在地图内 
	{
		if (x >= 0 && x <= 2 * MAP_SIZE - 1 && y >= 0 && y <= 2 * MAP_SIZE - 1 && z >= 0 && z <= 2 * MAP_SIZE - 1)
		{
			if ((x + y + z) == (3 * MAP_SIZE - 3))
				return true;
			else
				return false;
		}
		else
			return false;
	}

	Point operator [] (const Coordinate& c) { return data[c.x][c.y][c.z]; }

	bool isValid(Coordinate c)				//判断某个坐标是否在地图内  
	{
		return isValid(c.x, c.y, c.z);
	}



	int getDistance(Coordinate a, Coordinate b)		//计算两个坐标之间的距离 
	{
		return (abs(a.x - b.x), abs(a.y - b.y), abs(a.z - b.z)) / 2;
	}

};

#endif
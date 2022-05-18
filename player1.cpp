#ifdef __local_test__
#include "lib_bot/Bot.h"
#else
#include <bot.h>
#endif

#include "TuMou2022/Game.h"
#include <stdio.h>
#include <stdlib.h>
#include <stack>
#include <functional>
#include <vector>
#include <map>
#include <queue>

/*****************
class Coordinate{
public:
	int x, y, z;
	Coordinate(){
		
	}
	Coordinate(int,int,int){
	}
		Coordinate& operator= (const Coordinate& x){
			return *this;
	}
};

class Point{
public:
int x, y, z;// ¸Ãµã¶ÔÓ¦µÄ×ø±ê
int MineIdx;// ×ÊÔ´µãÐòºÅ.-1´ú±í¸ÃµãÃ»ÓÐ×ÊÔ´µã
int BarrierIdx;// ÕÏ°­ÎïÐòºÅ.-1´ú±íÃ»ÓÐÕÏ°­Îï
int PlayerIdx;// Íæ¼Ò±àºÅ.-1´ú±íÃ»ÓÐÍæ¼Ò£¬0Îªºì·½£¬1ÎªÀ¶·½
};

class Player{
public:
Coordinate pos;// ½ÇÉ«Î»ÖÃ
int id;// //½ÇÉ«µÄid.0Îªºì·½,1ÎªÀ¶·½.
int attack_range;// ¹¥»÷·¶Î§
int sight_range;// ÊÓÒ°·¶Î§
int move_range;// ÒÆ¶¯·¶Î§
int mine_speed;// ²É¼¯ÄÜÁ¦
int at;// ¹¥»÷Á¦
int hp;// ÉúÃüÖµ
int mines;// µ±Ç°ÓµÓÐµÄmineÊýÁ¿
};


class Mine{
public:
	Coordinate pos;// ×ÊÔ´µãÎ»ÖÃ
int num;// ¸Ãµã×ÊÔ´ÊýÁ¿
int belong;// ×ÊÔ´µãËùÓÐÕß.-1ÎªÀ¶·½,1Îªºì·½
bool available;// ÊÇ·ñ¿É²É¼¯
	
};


class Map{
public:
	Point data[50][50][50];// ÈýÎ¬Êý×é,´æ´¢Ñ¡ÊÖÊÓÒ°·¶Î§ÄÚµØÍ¼ÐÅÏ¢.data[i][j][k]±íÊ¾×ø±ê(i, j, k)¶ÔÓ¦µÄµã.
std::vector<Mine> mine;// ½ÇÉ«ÊÓÒ°·¶Î§ÄÚµÄmine
std::vector<Coordinate> barrier;// ½ÇÉ«ÊÓÒ°·¶Î§ÄÚµÄbarrier
std::vector<Player> enemy_unit;// ½ÇÉ«ÊÓÒ°·¶Î§ÄÚµÄµÐ·½µ¥Î»
int nowSize;// µØÍ¼µÄµ±Ç°´óÐ¡ µØÍ¼³õÊ¼´óÐ¡Îª24*24*24.´ÓµÚ75»ØºÏ¿ªÊ¼ËõÈ¦£¬Ã¿»ØºÏ¼õÐ¡Ò»¸öµ¥Î»
int mine_num;// ½ÇÉ«ÊÓÒ°·¶Î§ÄÚµÄmineÊýÁ¿
int barrier_num;// ½ÇÉ«ÊÓÒ°·¶Î§ÄÚµÄbarrierÊýÁ¿
int enemy_num;// ½ÇÉ«ÊÓÒ°·¶Î§ÄÚµÄµÐ·½µ¥Î»ÊýÁ¿
bool isValid(int x, int y, int z)const{return 1;}// ÅÐ¶ÏÄ³µãÊÇ·ñÔÚµØÍ¼ÄÚ
bool isValid(Coordinate c) const{return 1;}//ÅÐ¶ÏÄ³µãÊÇ·ñÔÚµØÍ¼ÄÚ
int getDistance(Coordinate a, Coordinate b)const{return 1;
}// ¼ÆËãa, bÁ½µãÖ®¼äµÄ¾àÀë
	
};

#define MAP_SIZE 24


class Operation{
public:
	int type;// ½ÇÉ«µÄ²Ù×÷ÀàÐÍ.-1´ú±í²»½øÐÐ²Ù×÷,0´ú±íÒÆ¶¯£¬1´ú±í¹¥»÷.
Coordinate target ;//ÒÆ¶¯»ò¹¥»÷Ä¿±ê
int upgrade ;//ÊÇ·ñÉý¼¶.0´ú±í²»Éý¼¶,1´ú±íÉý¼¶.
int upgrade_type ;//Éý¼¶µÄÀàÐÍ.0´ú±íÒÆ¶¯ËÙ¶È£¬1´ú±í¹¥»÷·¶Î§£¬2´ú±í²É¼¯ËÙ¶È£¬3´ú±í»ØÑª£¬4´ú±íÊÓÒ°·¶Î§£¬5´ú±í¹¥»÷Á¦.

	
};
    char *bot_recv() {
	return NULL;
	}
Coordinate tmp;
struct Game{
    Game(int side,int seed){
	}
    void proc() const{
	}

	
};


*************/

/*ÒÔÏÂ²¿·ÖÎÞÐèÐÞ¸Ä*/
Point myData[2 * MAP_SIZE - 1][2 * MAP_SIZE - 1][2 * MAP_SIZE - 1]; //¿ÉÒÔÔÚ´Ë´æ´¢½ÇÉ«Ì½Ë÷µ½µÄµØÍ¼ÐÅÏ¢
std::vector<Point> pointsInView; //½ÇÉ«µ±Ç°ÊÓÒ°·¶Î§ÄÚµÄµã
std::vector<Point> pointsInAttack; //½ÇÉ«µ±Ç°¹¥»÷·¶Î§ÄÚµÄµã
std::vector<Point> pointsInMove; //½ÇÉ«µ±Ç°ÒÆ¶¯·¶Î§ÄÚµÄµã
void getInfo(const Player& player, const Map& map)
{
    pointsInView.clear();
    pointsInAttack.clear();
    pointsInMove.clear();
    for(int i = 0; i < 2 * MAP_SIZE - 1; i++)
    {
        for(int j = 0; j < 2 * MAP_SIZE - 1; j++)
        {
            for(int k = 0; k < 2 * MAP_SIZE - 1; k++)
            {
                Coordinate tmp = Coordinate(i, j, k);
                if(map.isValid(tmp))
                {
                    if(map.getDistance(tmp, player.pos) <= player.sight_range - 1)
                    {
                        pointsInView.push_back(map.data[i][j][k]);
                        myData[i][j][k] = map.data[i][j][k];
                    }
                    if(map.getDistance(tmp, player.pos) <= player.attack_range)
                    {
                        pointsInAttack.push_back(map.data[i][j][k]);
                    }
                    if(map.getDistance(tmp, player.pos) <= player.move_range)
                    {
                        pointsInMove.push_back(map.data[i][j][k]);
                    }
                }
            }
        }
    }
}

int vis[2 * MAP_SIZE - 1][2* MAP_SIZE - 1][2 * MAP_SIZE - 1];

/*ÇëÐÞ¸Ä±¾º¯Êý*/

using namespace std;

#define abs(x) (((x)>0)?(x):-(x))
#define max(x,y) ((x)>(y)?(x):(y))

class PLA{
public:
	Coordinate pos; //½Ç É« Î» ÖÃ 5
	int attack_range; //¹¥ »÷ ·¶ Î§ 7 
	int sight_range; //ÊÓ Ò° ·¶ Î§ 8 
	int move_range; //ÒÆ ¶¯ ·¶ Î§ 9 
	int at; //¹¥ »÷ Á¦
	int hp; //Ñª Á¿
	int mines; //µ± Ç° Óµ ÓÐ µÄ ×Ê Ô´
	PLA(){
		
	}
	PLA& operator=(const Player& p){
		pos=p.pos;
		attack_range=p.attack_range;
		sight_range=p.sight_range;
		move_range=p.move_range;
		at=p.at;
		mines=p.mines;
		hp=p.hp;
		return *this;
	}
};

struct ST{
	using CO = Coordinate;
	
	
	Coordinate C(int x,int y,int z){
		Coordinate a;
		a.x=x,a.y=y,a.z=z;
		return a;
	}
	
	//¾²Ì¬±äÁ¿ 
	PLA self;
	PLA enemy;
	
	short myMap[49][49]{};
	bool VIS[49][49]{};
	double midu[49][49]{};
	short mines[1000];
	Coordinate minesPos[1000];
	int ttM;
	
	int st;
	int subst;
	Coordinate target;
	int nowSize;
	int SPU;
	
	
	//³õÊ¼»¯»·¾³ 
	ST(){
		st=0;
		subst=0;
		ttM=0;
	}
	int DIS(const CO& a, const CO& b){
		int x=abs(a.x-b.x);
		int y=abs(a.y-b.y);
		int z=abs(a.z-b.z);
		x=max(x,y);
		return (max(x,z));
	}
	
	bool inGAME(const CO& v){
		return (v.x>=0&&v.x<=48
		&&v.y>=0&&v.y<=48
		&&v.z>=0&&v.z<=48);
	}
	
	void foreach(Coordinate c, int len, std::function<void(const Coordinate&)> fun){
		int x0=c.x,y0=c.y,i,j;
		for (i=-len;i<=0;i++) 
			for (j=-len-i;j<=len;j++) {
				auto v = C(x0+i,y0+j,-i-j-x0-y0 + 72);
				if (inGAME(v)) fun(v);
			}
		for (i=1;i<=len;i++) 
			for (j=-len;j<=len-i;j++)  {
				auto v = C(x0+i,y0+j,-i-j-x0-y0 + 72);
				if (inGAME(v)) fun(v);
			}
			
		/*	i=-1 j=0 1
		*	  0    -1 0 1
		*	  1    -1 0
		*/
	}
	
	bool S0(const CO& x){
		return DIS(x, enemy.pos) > enemy.attack_range;
	}
	bool S1(const CO& x){
		return DIS(x, Coordinate(24,24,24)) <= nowSize + 1;
	}
	bool S2(const CO& x){
		return DIS(x, enemy.pos) > enemy.sight_range;
	}
	
	struct BFDStruct{
		Coordinate cr;
		int len;
		BFDStruct(){}
		BFDStruct(Coordinate CR, int Len):cr(CR),len(Len){}
	};
	
	int* getDisToMines(const Coordinate& X) {
		bool vis[52][52]{};
		int sz=ttM;
		int* dis = new int[sz];
		queue<BFDStruct> q;
		
		for (int i=0;i<sz;i++) dis[i] = 0xFFFFFF;
		
		q.push(BFDStruct(X,0));
		vis[X.x ][X.y ] = 1;
		
		if (myMap[X.x ][X.y ]>0) dis[myMap[X.x ][X.y ] - 5] = 0, sz--;
		
				
		while (sz && !q.empty()){
			auto M = q.front();
			q.pop();
			
			foreach(M.cr, self.move_range, [&](Coordinate c){
				if (!vis[c.x ][c.y ] && myMap[c.x ][c.y ]>=0){
					vis[c.x ][c.y ] = 1;
					q.push(BFDStruct(c, M.len+1));
					if (myMap[c.x ][c.y ]>0) dis[myMap[c.x ][c.y ] - 5] = M.len+1,sz--;
				}
			});
		}
		
		return dis;
	}

	Coordinate MoveToMine(Coordinate X) {
		if (DIS(X,self.pos)<=self.move_range) return X;
		
		bool vis[52][52]{};
		vector<BFDStruct> q;
		int front = 0;
		bool stop=0;
		
		q.push_back(BFDStruct(self.pos,0));
		while (q.size() == front){
			auto M = q[front];
			
			foreach(M.cr, self.move_range, [&](Coordinate c){
				if (!vis[c.x ][c.y ] && myMap[c.x ][c.y ]>=0 && !stop){
					if (X.x == c.x && X.y == c.y) {
						stop = 1;
						return;
					}
					vis[c.x ][c.y ] = 1;
					q.push_back(BFDStruct(c, front));
				}
			});
			if (stop){
				int of;
				do{
					of = front;
					front = q[front].len;
				} while(front);
				return q[of].cr;
			}
			
			front++;
		}
		
		return self.pos;
		
	}

	//¼ÆËã 
	Operation desMove(const Player& player, const Map& map){
		
		SPU = -1;
		if (map.enemy_num!=0) enemy=map.enemy_unit[0];
		for (auto &a:map.barrier) myMap[a.x][a.y] = -1000;
		self = player;
		nowSize = map.nowSize;
		for (auto &m:map.mine){
			if (myMap[m.pos.x ][m.pos.y ] > 0) mines[myMap[m.pos.x ][m.pos.y ]-5] = m.num;
			else {
				mines[ttM] = m.num;
				minesPos[ttM] = m.pos;
				myMap[m.pos.x ][m.pos.y ] = ttM+5;
				
				ttM++;
			}
		}
		foreach(self.pos,self.move_range,[&](const CO& p){
			VIS[p.x ][p.y ] = 1;
		});
		
		/*
		foreach(C(24,24,24),24,[&](const CO& p){
			double num = 0;
			int t= 0;
			foreach(p, 6, [&](const CO& p){
				t++;
				if (!vis[p.x ][p.y ]) num += 3;
				else if (myMap[p.x ][p.y ] > 0) num += mines[myMap[p.x ][p.y ]-5] / 30;
				else num -= 0.4;
			});
			midu[p.x ][p.y ] = num / t;
		});
		*/
		
		if (st == 0){
			//ÔçÆÚ·¢Óý×¼±¸ 
				if (subst == 0){
				
					int* d = getDisToMines(self.pos);
				
					int Min = 123321;
					for (int i=0;i<ttM;i++){
						if (Min > d[i]){
							Min=d[i];
							target = minesPos[i];
						}
					}
					
					delete[]d;
					
				}
				
				if (self.pos.x == target.x && self.pos.y == target.y) st = 1, subst = 0;
				
				
			}
		if (st == 1){
			//ÔçÆÚ·¢Óý
				static Coordinate subtarget;
				static int subDis;
				static int DOPU[][8]={
					{},
					{2,2,2,2,0,2,-1},
					{2,2,2,0,2,2,-1},
					{2,2,0,0,2,2,-1},
					{2,2,2,0,2,2,-1},
					{2,2,0,0,2,2,2,-1},
					{2,2,0,0,2,2,2,-1},
					{-1}
				};
				
				if (subst == 0){
				
					int* d = getDisToMines(self.pos);
				
					int Min = 123321;
					for (int i=0;i<ttM;i++){
						if (Min > d[i] && d[i] != 0){
							Min = d[i];
							subtarget = minesPos[i];
						}
					}
					subDis = Min;
					
					delete[] d;
					
				}
				
				if (DOPU[subDis][subst] == -1) {
					subst = 0;
					st = 2;
				}
				else {
					SPU = DOPU[subDis][subst];	
					if (subst == 4) target = subtarget;
					
					subst ++;		
				}
			}
				
		if (st == 2){
			//·¢Óý
			if (map.enemy_num) st=3;
			else {
				//²»ÒËÔÚÒ»µãÍ£Áô£¬¶à¶àÌ½Ë÷
				int score = -1;
				
				foreach(self.pos, self.move_range ,[&](const CO& p){
						//¶¾È¦ÄÚ 
					if (!S1(p)) return;
					
						//ËùÔÚµÄ¿ó²ú 
						//¿ó²úÃÜ¶È 
						
					int newScore = 0;
					
					newScore += mines[myMap[p.x ][p.y ]-5] / 30 * 30;
					newScore += midu[p.x ][p.y ] * 30;	
					
					if (newScore > score){
						newScore = score;
						target = p;
					}
				});
				
				
			}			
		}
			
		if (st == 3){
			//Èó		1µÐÈËÔÚÊÓÏßÄÚ£¬0Ê§È¥¸ú×Ù 
			subst = map.enemy_num;
			
			if (subst) {
				int score = -1;
				
				foreach(self.pos,self. move_range,[&](const CO& p){
					if (!S0(p)) return;
					
						//ÓëµÐÈË¾àÀë
						//ÊÇ·ñÄÜ±»µÐÈË¿´µ½ 
						//ËùÔÚµÄ¿ó²ú 
						//¿ó²úÃÜ¶È 
						//¶¾È¦ÄÚ 
						
					int newScore = 0;
					
					newScore += DIS(p, self.pos) * 10;
					newScore += (S2(self.pos)) * 70;
					newScore += mines[myMap[p.x ][p.y ]-5] / 30 * 30;
					newScore += midu[p.x ][p.y ] * 30;	
					newScore += S1(p) * 200;
					
					if (newScore > score){
						newScore = score;
						target = p;
					}
				});
			}
			else {
				//ÒÆ¶¯·¶Î§ self.sight_range - enemy.attack_range 
				
				int score = -1;
				
				foreach(self.pos, self.sight_range - enemy.attack_range ,[&](const CO& p){
						//ËùÔÚµÄ¿ó²ú 
						//¿ó²úÃÜ¶È 
						//¶¾È¦ÄÚ 
						
					int newScore = 0;
					
					newScore += mines[myMap[p.x ][p.y ]-5] / 30 * 30;
					newScore += midu[p.x ][p.y ] * 30;	
					newScore += S1(p) * 200	;
					
					if (newScore > score){
						newScore = score;
						target = p;
					}
				});
			}
								
				
			}
		
		 
		
		Operation opr;
		opr.type = 0;
		opr.upgrade = 1;
		
		opr.target = MoveToMine(target);
		
		if (self.hp<=50) opr.upgrade_type = 3;
		else {
			if (st > 1){
				
				if (st==3){
					if (self.move_range < enemy.attack_range + 2) SPU = 0;
					else {
						if (self.sight_range < enemy.attack_range + 2) SPU = 4;
						else {
							if (self.mines >= 110 && self.sight_range < enemy.attack_range + 4) SPU = 4; 
							else SPU = -1;
						}
					}
				}
				else {
					static char vk[]={0,0,6,6,7,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40};
					
					if (self.sight_range < vk[self.move_range]) SPU = 4;
					else SPU = 0;
					
				}
								
			}
			opr.upgrade_type = SPU;
		}
		
		return opr;
	}
	
};

Operation get_operation(const Player& player, const Map& map)
{
    getInfo(player, map);
	static ST SV;
    return SV.desMove(player, map);
}

/*ÒÔÏÂ²¿·ÖÎÞÐèÐÞ¸Ä*/
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
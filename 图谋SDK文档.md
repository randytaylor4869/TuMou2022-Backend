# 图谋SDK文档

## 1.选手提交说明

#### 选手需要填写的函数

#### *get_operation_red*

#### *get_operation_blue*

当选手回合开始后，会执行`get_operation_red`函数或者`get_operation_blue`函数(当选手是红方时执行`get_operation_red`函数,当选手是蓝方时执行`get_operation_blue`函数)。**选手通过该函数来操纵角色做出该回合相应的决策。若选手返回的操作不合法，则不进行任何操作。**

```c++
Operation get_operation_red(const Player& player, const Map& map) 
Operation get_operation_blue(const Player& player, const Map& map) 
```

每个游戏回合的流程为:

* 根据玩家位置更新采矿收入。
* 向红方玩家提供基于其视野的地图，获取红方玩家的操作。
* 查看蓝方是否存活。
* 向蓝方玩家提供基于其视野的地图，获取蓝方玩家的操作。
* 缩圈，并对圈外玩家造成毒伤。
* 检查红蓝双方玩家是否存活。

参数说明:

* `player`:`Player`类。负责存储角色的攻击范围、视野范围、移动范围、采集速度、攻击力、血量以及当前采集到的资源的数据。选手可以通过该参数来查看角色当前的状态。
* `map`:`Map&`类。负责存储游戏地图中的资源点、资源数量、障碍点、障碍点数量、地图大小、视野范围、毒圈边长等数据。选手可以通过该参数来查看地图中**视野范围内节点**当前的状态。

## 2.相关类说明

### Coordinate

```C++
struct Coordinate
{
	int x, y, z;
};
```

`Coordinate`类中存储着某个节点当前坐标的x, y, z值。

### Player

```c++
class Player
{
public:
	int id; //红方为0，蓝方为1
	Coordinate pos;  //当前位置
	int attack_range;   //攻击范围
	int sight_range; //视野范围
	int move_range; //移动范围
	int mine_speed; //采集速度
	int at; //攻击力
	int hp; //血量
	int mines;  // 采集到的资源(用于升级)
};
```

`Player`类中存储着角色的当前位置、攻击范围、视野范围、移动范围、采集速度、攻击力、血量、采集到的资源的数据。选手在每回合决策过程中，可以直接查看传入的参数`player`中的各个变量来获取角色的信息。

### Point

```c++
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
}
```

`Point`类中存储着地图上某个点的坐标信息、资源点信息、障碍物信息、玩家编号信息和是否在地图内的信息。

### Mine

```c++
class Mine {	//资源点
public:
	Coordinate pos;
	int num, belong = 0;   //belong=0为中立,-1蓝方，1红方
	bool available = 1;
	Mine(const int n, Coordinate x) { num = n; pos = x; }
};
```

`Mine`类中存储着资源点的坐标位置、资源的归属方、资源是否可获得的信息。

### Map

```c++
class Map {
public:
	Point data[2 * MAP_SIZE - 1][2 * MAP_SIZE - 1][2 * MAP_SIZE - 1];		//地图信息，data[i][j][k]表示地图一个位置的信息

	std::vector <Mine> mine; //矿物信息
	std::vector<Coordinate> barrier;//障碍物信息
	std::vector<Player> enemy_unit;//地方单位
	int nowSize = MAP_SIZE;			//当前毒圈的边长
	int viewSize = MAP_SIZE; //视野范围
	int mine_num = 0; //矿点数量
	int barrier_num = 0; //障碍物数量
	int enemy_num = 0;//敌军数量
}
```

`Map`类中存储着地图上点的信息、矿产信息、边界信息、敌军信息、当前毒圈边长、角色的视野范围、矿产数量、障碍物数量、敌军数量的信息。**每回合传给玩家的地图信息是限制在角色的视野范围之内的,玩家只能获取局部地图，不可查看全局地图信息。**

### Operation

```c++
class Operation
{
public:
    int type; // 移动(0) 或 攻击(1) 或 none(-1)
    Coordinate target;  // 移动/攻击目标
    bool upgrade; //是否升级
    int upgrade_type; //升级类型：0-移动速度， 1-攻击力， 2-采集速度， 3-回血，4-视野，5-生命值
    Operation() : type(-1), upgrade(false) {};
};
```

`Operation`类存储着玩家每回合的操作信息。玩家每回合可以通过修改`type`变量来选择是要进行移动或攻击或是不进行任何操作。`target`变量为玩家移动或者攻击的目标点。玩家通过修改`update`变量来选择是否升级。通过`upgrade_type`变量来选择升级的类型:0-移动速度， 1-攻击力， 2-采集速度， 3-回血，4-视野，5-生命值。**参赛选手在补充`get_operation_red`和`get_operation_blue`函数的时候需要创建一个`Operation`变量，并通过修改改变量中的上述成员变量来告知系统本回合的操作。**



## 3.样例代码

### 获取游戏信息

#### 角色信息获取

```c++
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
```

#### 地图信息获取

```c++
    //以下变量为map类中的public成员，玩家可以直接从map中获取相关信息
    std::vector<Mine> mine = map.mine;         //矿物信息
    std::vector<Coordinate> barrier;           //障碍物信息
    int nowSize = map.nowSize;                 //地图大小
    int viewSize = map.viewSize;               //视野范围
    int barrier_num = map.barrier_num;         //障碍物数量
    int enemy_num = map.enemy_num;             //敌方数量
```

### 玩家决策

```c++
    //玩家可以根据以上获取的信息来进行决策
    srand(time(0));
    op.type = 0;
    if(mines >= 100) op.upgrade = 1;
    op.type = rand() % 6;
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
    op.type = 0;  //角色默认向远离毒圈的方向移动
    Coordinate center(MAP_SIZE - 1, MAP_SIZE - 1, MAP_SIZE - 1);
    for(int i = 0; i < 6; i++)
    {
        Coordinate tmp;
        tmp.x = x + dx[i];
        tmp.y = y + dy[i];
        tmp.z = z + dz[i];
        if(map.getDistance(tmp, center) < map.getDistance(player.pos, center))
        {
            op.target = tmp;
            return op;
        }
    }
    return op;
```






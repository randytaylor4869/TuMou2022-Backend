# Operation类说明
Operation是每回合玩家返回的操作

int type 操作类型：
    (0)-移动，(1)-攻击，(-1)-不移动或攻击

Coordinate target 移动/攻击目标坐标

bool upgrade 是否升级

int upgrade_type 升级类型：
    0-移动速度， 1-攻击力， 2-采集速度， 3-回血，4-视野，5-生命值

Operation 默认不进行移动、攻击或升级


# Game类说明

## 成员变量

Map map 游戏地图信息

Player player_red, player_blue 双方玩家

int turn 当前回合数

## 成员函数

Operation regulate(Operation op, Player p)
检查玩家操作是否合法。输入代表玩家p给出了op操作，若op操作合法则返回op，否则返回不进行任何动作的操作。

void upgrade(int type, Player& p)
更新玩家p合法的升级。
type代表升级类型：
    0-移动速度， 1-攻击力， 2-采集速度， 3-回血，4-视野，5-生命值。

bool Update()
进行一个回合，流程如下：
a.根据玩家位置更新采矿收入
b.向红方玩家提供基于其视野的地图，获取红方玩家操作
c.检查操作是否合法，并进行相应数值修改
d.检查蓝方玩家是否存活
e.由蓝方操作重复b-d
f.缩圈，并造成毒伤
g.检查双方玩家是否存活
函数返回true，当且仅当有玩家死亡，否则返回false

int proc()
对局入口，每回合调用Update()，最终返回赢家id(red = 0, blue = 1)

# Player类说明

## 成员变量

int id 玩家id：0-红方 1-蓝方

Coordinate pos 玩家当前位置

int attack_range;   攻击范围
int sight_range;    视野范围
int move_range;     移动范围
int mine_speed;     采集速度（单词采集最大值）

int at; 攻击力
int hp; 血量

int mines; 采集到的资源(用于升级)

# Map类说明

## 成员变量

Point data[][][] 地图信息（采用六边形地图，三个坐标）

std::vector <Mine> mine 矿物（采集资源）

std::vector<Coordinate> barrier 障碍物（阻碍移动）

std::vector<Player> enemy_unit 敌方单位
此变量是为了将Map传递给两名选手时，根据视野状况选择是否将敌方位置告知。

int nowSize 当前毒圈边长，初始化为地图大小

int mine_num 矿点数量

int barrier_num 障碍物数量

int enemy_num （视野内）敌人数量

## 成员函数

bool isValid(int x, int y, int z)
bool isValid(Coordinate c)
判断某个坐标是否在地图内 

int getDistance(Coordinate a, Coordinate b)
计算两个坐标之间的距离 

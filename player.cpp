#ifdef __local_test__
#include "lib_bot/Bot.h"
#else
#include <bot.h>
#endif

#include "TuMou2022/Game.h"
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////
//                       BEGINNING OF MACROS                        //
//////////////////////////////////////////////////////////////////////
#include <utility>
#include <queue>
#include <algorithm>
//////////////////////////////////////////////////////////////////////
//                           END OF MACROS                          //
//////////////////////////////////////////////////////////////////////

#define MAP_SIZE 25
#define K (MAP_SIZE - 1)

////////////////////////////////////////////////////////////////////////////
//                       BEGINNING OF COPIED CODES                        //
////////////////////////////////////////////////////////////////////////////

Point myData[2 * MAP_SIZE - 1][2 * MAP_SIZE - 1]; // 可以在此存储角色探索到的地图信息
std::vector<Point> pointsInView;                  // 角色当前视野范围内的点
std::vector<Point> pointsInAttack;                // 角色当前攻击范围内的点
std::vector<Point> pointsInMove;                  // 角色当前移动范围内的点

std::vector<Coordinate> ADJ = {{0, 1, -1}, {1, 0, -1}, {1, -1, 0}, {0, -1, 1}, {-1, 0, 1}, {-1, 1, 0}}; // 从左下方开始，逆时针顺序

int vis[2 * MAP_SIZE - 1][2 * MAP_SIZE - 1]; // used on multiple occasions

void init()
{
    // Executed before getInfo
}

void getInfo(const Player &player, const Map &map)
{
    // Getting points
    pointsInView.clear();
    pointsInAttack.clear();
    pointsInMove.clear();

    // Get points in view range and attack range
    // TODO: can be improved if time permits
    for (int i = 0; i < 2 * MAP_SIZE - 1; i++)
    {
        for (int j = 0; j < 2 * MAP_SIZE - 1; j++)
        {
            int k = 3 * K - i - j;
            Coordinate tmp = {i, j, k};
            if (map.isValid(tmp))
            {
                if (map.getDistance(tmp, player.pos) <= player.sight_range - 1)
                {
                    pointsInView.push_back(map.data[i][j][k]);
                    myData[i][j] = map.data[i][j][k];
                }
                if (map.getDistance(tmp, player.pos) <= player.attack_range)
                {
                    pointsInAttack.push_back(map.data[i][j][k]);
                }
            }
        }
    }

    // Get points in move range
    class compare
    {
    public:
        bool operator()(const std::pair<Coordinate, int> &p1, const std::pair<Coordinate, int> &p2)
        {
            return p1.second > p2.second;
        }
    };

    // Initializations
    std::vector<Coordinate> found;
    std::priority_queue<std::pair<Coordinate, int>, std::vector<std::pair<Coordinate, int>>, compare> q;
    q.push({player.pos, 0});

    for (int i = 0; i < 2 * MAP_SIZE - 1; i++)
    {
        for (int j = 0; j < 2 * MAP_SIZE - 1; j++)
        {
            vis[i][j] = 0;
        }
    }

    while (!q.empty())
    {
        std::pair<Coordinate, int> p = q.top();
        Coordinate tmp = p.first;
        q.pop();

        if (p.second > player.move_range)
        {
            // Unreachable
            break;
        }

        // Check if the point has been discovered
        if (!vis[tmp.x][tmp.y])
        {
            vis[tmp.x][tmp.y] = 1;
            if (map.data[tmp.x][tmp.y][tmp.z].BarrierIdx < 0)
            {
                pointsInMove.push_back(map.data[tmp.x][tmp.y][tmp.z]);
                for (const Coordinate &a : ADJ)
                {
                    if (map.isValid(tmp.x + a.x, tmp.y + a.y, tmp.z + a.z))
                    {
                        q.push({{tmp.x + a.x, tmp.y + a.y, tmp.z + a.z}, p.second + 1});
                    }
                }
            }
        }
    }
}

Operation get_operation(const Player &player, const Map &map)
{
    // Initialization
    init();
    getInfo(player, map);

    // Algorithms begin here
    Operation op;
    // vis[player.pos.x][player.pos.y] = 1;

    // Upgrade type: randomly chosen (if possible)
    srand(time(NULL));
    int uptype = rand() % 6;
    if (player.mines >= UPGRADE_COST[uptype])
    {
        op.upgrade_type = uptype;
        op.upgrade = 1;
    }

    // Attack first if the enemy is in the attacking range
    for (int i = 0; i < pointsInAttack.size(); i++)
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

    // Check for reachable blocks; move to the block when found
    if (pointsInMove.size() > 0)
    {
        int i = rand() % pointsInMove.size();
        op.type = 0;
        op.target.x = pointsInMove[i].x;
        op.target.y = pointsInMove[i].y;
        op.target.z = pointsInMove[i].z;
        return op;
    }

    return op;
}

////////////////////////////////////////////////////////////////////////////
//                           END OF COPIED CODES                          //
////////////////////////////////////////////////////////////////////////////

/*以下部分无需修改*/
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
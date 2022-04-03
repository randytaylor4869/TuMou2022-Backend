#include "Game.h"

#ifdef __PLAYER_RED__
Operation get_operation_red(const Player& player, const Map& map)   // todo : SDK，玩家编写
{
    Operation op;
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

    op.type = 0;
    op.target.x = player.pos.x + 1;
    op.target.y = player.pos.y + 1;
    op.target.z = player.pos.z + 1;
    op.upgrade = 1;
    srand(time(0));
    op.upgrade = rand() % 6;
    return op;
}
#endif

#ifdef __PLAYER_BLUE__
Operation get_operation_blue(const Player& player, const Map& map)   // todo : SDK，玩家编写
{
    Operation op;
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
    for(int i = 0, j = rand()%6, k; i < 6; i++)
    {
        k = (j + i) % 6;
        op.target.x = player.pos.x + dx[k];
        op.target.y = player.pos.y + dy[k];
        op.target.z = player.pos.z + dz[k];
        if(map.isValid(op.target) && map.getDistance(player.pos, op.target) <= move_range)
        {
            //std::cerr << "getit\n";
            break;
        }
    }
    
    op.upgrade = 1;
    op.upgrade = rand() % 6;
    return op;
}
#endif


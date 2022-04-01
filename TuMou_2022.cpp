// TuMou_2022.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include"Game.h"
// #include "Player.h"
#include "player0.cpp"
#include "player1.cpp"

#include <iostream>

Game game;

int main()
{
    bool flag = false;
    while(!flag)
    {
        flag = game.Update();
    }
    return 0;
}



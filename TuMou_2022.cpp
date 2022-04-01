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
    int stat = game.proc();
    std::cerr << "normal end with winner" << stat << std::endl;
    return 0;
}



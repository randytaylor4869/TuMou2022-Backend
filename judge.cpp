#ifdef __local_test__
#include "lib_bot/Bot.h"
#else
#include "bot.h"
#endif

#include "Game.h"
#include <stdio.h>

// bot
int bot_judge_init(int argc, char *const argv[]);
void bot_judge_send(int id, const char *str);
char *bot_judge_recv(int id, int *o_len, int timeout);
void bot_judge_finish();

Game *game;


Operation get_operation(const Player& player, const Map& map) {Operation op; return op;}

int main(int argc, char *argv[])
{
    freopen("output.txt","w",stdout);
    
    /* Initialize players and make sure there are exactly 2 of them */
    int n = bot_judge_init(argc, argv);
    if (n != 2) {
        fprintf(stderr, "Expected 2 players, got %d\n", n);
        exit(1);
    }

    /* Initialize the game */
    srand(time(0));
    int seed = rand();
    game = new Game(-1,seed);

    /* Inform each player which side it is on and the seed to initialize the map*/
    char s[100];
    sprintf(s, "%d %d", 0, seed);
    bot_judge_send(0, s);
    sprintf(s, "%d %d", 1, seed);
    bot_judge_send(1, s);

    /* Play the game */
    int stat = game->proc();

    /* Write the report to stdout */
    if(stat == 0 || stat == 1)
        printf("The winner is player %d\n", stat);
    else
        printf("Game Ends Abnormally with code %d\n", stat);
    //输出json
    nlohmann::json list;
    list["InitialState"] = game->m_init;
    list["list"] = game->m_root;
    std::cout << list.dump(-1) << std::endl;    
    /* Terminate the players */
    bot_judge_finish();
    std::cerr << "finished.\n" << std::endl;
    return 0;
}
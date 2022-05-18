LOGS = p1.log p2.log
JUDGE_EXEC := ./judge
PLAYER1_CMD := ./player1
PLAYER2_CMD := ./player2

#header = $(wildcard *.h) $(wildcard ./json/*.h) $(wildcard ./lib_json/*.h) $(wildcard ./lib_json/*.inl) $(wildcard ./lib_bot/*.h)
#json_src =  $(wildcard ./lib_json/*.cpp)

header = $(wildcard *.h) $(wildcard *.hpp) $(wildcard ./lib_bot/*.h)

default: judge player1 player2 run

solo: judge player1 runsolo

judge: lib_bot/Bot.cpp judge.cpp $(header)
	g++ -D __local_test__ lib_bot/Bot.cpp judge.cpp -o $@

player1: lib_bot/Bot.cpp player1.cpp $(header)
	g++ -D __local_test__ lib_bot/Bot.cpp player1.cpp -o $@

player2: lib_bot/Bot.cpp player2.cpp $(header)
	g++ -D __local_test__ lib_bot/Bot.cpp player2.cpp -o $@

run:
	@ "$(JUDGE_EXEC)" $(JUDGE_ARGS) "$(PLAYER1_CMD)" "$(PLAYER2_CMD)" $(LOGS)

runsolo:
	@ "$(JUDGE_EXEC)" $(JUDGE_ARGS) "$(PLAYER1_CMD)" "$(PLAYER1_CMD)" $(LOGS)

clean: clean_exec clean_log

clean_exec:
	$(RM) -f judge player
	$(RM) -f judge.exe player1.exe player2.exe player1 player2

clean_log:
	$(RM) -f *.log
	$(RM) output.txt


.PHONY: default solo run runsolo clean clean_exec clean_log
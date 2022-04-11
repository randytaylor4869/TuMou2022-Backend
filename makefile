LOGS = p1.log p2.log
JUDGE_EXEC := ./judge
PLAYER_CMD := ./player

#header = $(wildcard *.h) $(wildcard ./json/*.h) $(wildcard ./lib_json/*.h) $(wildcard ./lib_json/*.inl) $(wildcard ./lib_bot/*.h)
#json_src =  $(wildcard ./lib_json/*.cpp)

header = $(wildcard *.h) $(wildcard *.hpp) $(wildcard ./lib_bot/*.h)

default: judge player run

judge: lib_bot/Bot.cpp judge.cpp $(header)
	g++ -D __local_test__ lib_bot/Bot.cpp judge.cpp -o $@

player: lib_bot/Bot.cpp player.cpp $(header)
	g++ -D __local_test__ lib_bot/Bot.cpp player.cpp -o $@

run:
	@ "$(JUDGE_EXEC)" $(JUDGE_ARGS) "$(PLAYER_CMD)" "$(PLAYER_CMD)" $(LOGS)

clean: clean_exec clean_log

clean_exec:
	$(RM) -f judge player
	$(RM) -f judge.exe player.exe

clean_log:
	$(RM) -f *.log
	$(RM) output.txt


.PHONY: default run clean clean_exec clean_log
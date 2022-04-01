
.PHONY: all clean
header = $(wildcard *.h) $(wildcard ./json/*.h) $(wildcard ./lib_json/*.h) $(wildcard ./lib_json/*.inl)
json_src =  $(wildcard ./lib_json/*.cpp)

all: main1 main2

main1: TuMou_2022.cpp player0_red.o player1_blue.o $(json_src) $(header) 
	g++ TuMou_2022.cpp player0_red.o player1_blue.o $(json_src) -o $@
	
main2: TuMou_2022.cpp player0_blue.o player1_red.o $(json_src) $(header)
	g++ TuMou_2022.cpp player0_blue.o player1_red.o $(json_src) -o $@

player0_red.o: player0.cpp $(header)
	g++ -c -D __PLAYER_RED__ -o $@ $<

player0_blue.o: player0.cpp $(header)
	g++ -c -D __PLAYER_BLUE__ -o $@ $<

player1_red.o: player1.cpp $(header)
	g++ -c -D __PLAYER_RED__ -o $@ $<

player1_blue.o: player1.cpp $(header)
	g++ -c -D __PLAYER_BLUE__ -o $@ $<

clean:
	rm -f *.o
	rm -f main1 main2
	rm -f main1.exe main2.exe
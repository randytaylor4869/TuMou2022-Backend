.PHONY: all clean
header = $(wildcar *.h)

all: main1 main2

main1: TuMou_2022.cpp $(header) 
	g++ $< player0_red.o player1_blue.o -o $@
	
main2: TuMou_2022.cpp $(header) 
	g++ $< player0_blue.o player1_red.o -o $@

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
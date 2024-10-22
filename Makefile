all:
	g++ -std=c++23 -w -Weffc++ -pedantic ./connect4_cli.cpp -o test
	clear
	./test


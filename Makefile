all:
	cc main.c `pkg-config --libs ncurses` -O3 -Wall -o fyra

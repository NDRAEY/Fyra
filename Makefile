all:
	cc main.c `pkg-config --libs ncurses` -Wall -o fyra

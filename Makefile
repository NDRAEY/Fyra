all:
	cc main.c `pkg-config --libs ncurses` -Wall -Wsign-compare \
	-Werror -o fyra

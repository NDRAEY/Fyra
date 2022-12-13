#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <curses.h>

#define NAME "Fyra Editor by NDRAEY"
#define NAMELEN strlen(NAME)
#define VERSION "1.0.0"

#define WHITE_ON_BLACK 1
#define BLACK_ON_WHITE 2
#define ERROR_INTERNAL 3

#define BUFFER_INCREMENT 16

size_t scrwidth;
size_t scrheight;

WINDOW* mainwin;
WINDOW* upperbar;
WINDOW* textbox;
WINDOW* lowerbar;

size_t tmp;  // for supressing 'Unused variable' warning

char* buffer = 0;
size_t bufsize = 0;

size_t rema = 0;

int tx, ty;

void init_colors() {
	start_color();

	init_pair(WHITE_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
	init_pair(BLACK_ON_WHITE, COLOR_BLACK, COLOR_WHITE);
	init_pair(ERROR_INTERNAL, COLOR_WHITE, COLOR_RED);
}

void draw_upper_bar() {
	size_t uw = 0;

	wmove(upperbar, 0, 0);
	wattrset(upperbar, COLOR_PAIR(BLACK_ON_WHITE));

	getmaxyx(upperbar, tmp, uw);

	for(size_t i = 0; i < (uw-NAMELEN)/2; i++) {
		waddch(upperbar, ' ');
	}
	wprintw(upperbar, NAME);
	for(size_t i = 0; i < (uw-NAMELEN)/2; i++) {
		waddch(upperbar, ' ');
	}
}

int calc_buf_pos(size_t x, size_t y) {
	size_t mx = 0, my = 0;
	size_t bytecount = 0;

	for(size_t i = 0, le = strlen(buffer); i < le; i++) {
		if(mx == x && my == y) break;

		if(buffer[i] == '\n') {
			if(my==y && x > mx) break;
			my++;
			mx = 0;
		}else{
			mx++;
		}

		bytecount++;
	}

	return bytecount;
}

int max_pos_at_y(size_t y) {
	size_t mx = 0, my = 0;

	for(size_t i = 0, le = strlen(buffer); i < le; i++) {
		if(buffer[i] == '\n') {
			my++;
			if(my-1 == y) break;
			mx = 0;
		}else{
			mx++;
		}
	}
	return mx;
}

void focus_textbox() {
	size_t tx, ty = 0;
	getyx(textbox, ty, tx);

	wmove(textbox, ty, tx);
	wrefresh(textbox);
}

void lower_bar_fill_remaining() {
	size_t lcx, lmx = 0;
	getyx(lowerbar, tmp, lcx);
	getmaxyx(lowerbar, tmp, lmx);

	for(size_t i = 0; i < lmx-lcx; i++) {
		waddch(lowerbar, ' ');
	}
}

void draw_lower_bar() {
	wattrset(lowerbar, COLOR_PAIR(BLACK_ON_WHITE));

	wmove(lowerbar, 0, 0);

	getyx(textbox, ty, tx);
	
	wprintw(lowerbar, "L-%d C-%d // B-%d bytes // WR-%d // %d", 
					  ty, tx, bufsize, calc_buf_pos(tx, ty), rema
	);

	lower_bar_fill_remaining();

	focus_textbox();
}

void manage_buffer() {
	if(strlen(buffer)+1 >= bufsize) {
		bufsize += BUFFER_INCREMENT;
		buffer = realloc(buffer, bufsize);
	}
}

void init_base() {
	struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	scrheight = w.ws_row;
	scrwidth = w.ws_col;

	/////////////

	buffer = calloc(sizeof(char), BUFFER_INCREMENT);
	bufsize = BUFFER_INCREMENT;
}

void destroy_base() {
	memset(buffer, 0, bufsize);
	free(buffer);
	bufsize = 0;
}

void set_text_pos(size_t x, size_t y) {
	wmove(textbox, y, x);
}

size_t get_text_max_y() {
	size_t my = 0;
	for(size_t i = 0, le = strlen(buffer); i < le; i++) {
		if(buffer[i]==0) break;

		if(buffer[i]=='\n') {
			my++;
		}
	}
	return my;
}

void update_textbox() {
	size_t tx = 0, ty = 0;
	getyx(textbox, ty, tx);

	wclear(textbox);
	wmove(textbox, 0, 0);
	wprintw(textbox, "%s", buffer);
	wmove(textbox, ty, tx);

	wrefresh(textbox);
}

char process_character(int ch) {
	// if(ch == KEY_BACKSPACE) { // Backspace
	if(ch == '\b' || ch == 0x7f) { // Backspace
		// set_text_pos();
		size_t tmx = 0;
		getmaxyx(textbox, tmp, tmx);

		if(tx<=0) {
			if(ty<=0) return 1;
			else {
				// tx = tmx-1;
				ty--;
				tx = max_pos_at_y(ty)+1;
			}
		}

		tx--;

		wmove(textbox, ty, tx);
		waddch(textbox, ' ');
		wmove(textbox, ty, tx);

		///////// Lets concat two memory strings but without one character.

		int pos = calc_buf_pos(tx, ty);

		memmove(buffer+pos, buffer+pos+1, bufsize-pos-1);

		return 1;
	}else if(ch==17) { // Ctrl+Q (Ctrl+q)
		endwin();

		printf("Buffer below.\n");
		printf("%s\n", buffer);

		destroy_base();

		exit(0);
	}else if(ch==KEY_UP) {
		size_t correction;

		if(ty > 0) {
			if((correction = max_pos_at_y(ty-1)) < tx) {
				tx = correction;
			}
			ty--;
		}

		focus_textbox();

		return 1;
	}else if(ch==KEY_DOWN) {
		size_t correction;

		if(ty < get_text_max_y()) {
			if(tx > (correction = max_pos_at_y(ty-1))) {
				tx = correction;
			}
			ty++;
		}

		focus_textbox();

		return 1;
	}else if(ch==KEY_LEFT) {
		if(tx > 0) {
			tx--;
		}

		focus_textbox();

		return 1;
	}else if(ch==KEY_RIGHT) {
		if(tx < scrwidth) {
			tx++;
		}

		focus_textbox();

		return 1;

	}
	return 0;
}

void insertchar(unsigned int ch) {
	int pos = calc_buf_pos(tx, ty);

	int remaining = (bufsize-pos)-1;

	if(pos < 0) return;

	memmove(buffer+pos+1, buffer+pos, remaining);
	buffer[pos] = ch;

	if(ch == '\n') {
		ty++;
		tx = 0;
	}else{
		tx++;
	}

	wmove(textbox, ty, tx);

	rema = remaining;
}

int main() {
	mainwin = initscr();
	cbreak();
	noecho();
	raw();
	
	init_base();

	upperbar = newwin(2, scrwidth, 0, 0);
	textbox = newwin(scrheight-2, scrwidth, 2, 0);
	lowerbar = newwin(2, scrwidth, scrheight-2, 0);

	keypad(textbox, TRUE);
	
	if(has_colors()) init_colors();

	draw_upper_bar();
	wrefresh(upperbar);
	
	wrefresh(textbox);

	update_textbox();

	draw_lower_bar();
	wrefresh(lowerbar);

	focus_textbox();

	int ch;

	for(;;) {
		ch = wgetch(textbox);

		if(!process_character(ch)) {
			insertchar(ch);
			
			manage_buffer();
		}

		wmove(textbox, ty, tx);

		update_textbox();
		draw_lower_bar();
		wrefresh(lowerbar);
		focus_textbox();
	}

	endwin();
	
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "yell.h"
#include "whisper.h"

// run at thirty frames per second
#define FRAMES_PER_SEC  (1.0 / 30.0)

#define MAP_H  32
#define MAP_W  64

char *default_art = 
	"O_O\n"
	"/|\\\n"
	"/ \\\n";

char *tree_art =
	".oOOo.\n"
	"oOOOOo\n"
	"OOOOOO\n"
	"oOOOOo\n"
	"'OOOO'\n"
	"  ||  \n";

int main(void) {
	int game,
	    ch,
	    height, width,
	    y, x,
	    i, j;
	clock_t last, current;
	double seconds,
	       delta;
	WINDOW *map_window;
	yell_t yell;
	yevent_t event;

	sprite_t sprite[4];
	sprite_t tree;
	map_t map;

	tree.art = tree_art;
	tree.h = 6;
	tree.x = 32;
	tree.y = 16;

	srand(time(0));

	for (i = 0; i < 4; ++i) {
		sprite[i].art = default_art;
		sprite[i].h = 3;
		sprite[i].y = 4 + rand() % (MAP_H - 8);
		sprite[i].x = 4 + rand() % (MAP_W - 8);
	}

	map.sprite_ll = NULL;

	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	nodelay(stdscr, TRUE);

	if (has_colors() == FALSE) {
		fprintf(stderr, "Terminal does not support colors.\n");

		exit(EXIT_FAILURE);
	}

	yell_connect(&yell);

	getmaxyx(stdscr, height, width);
	map_window = newwin(MAP_H, MAP_W, (height - MAP_H) / 2, (width - MAP_W) / 2);
	wborder(map_window, '*', '*', '*', '*', '*', '*', '*', '*');

	game = true;
	last = clock();

	y = 8;
	x = 8;

	j = 0;

	// game loop
	while (game) {
		// parse yell events
		while (yell_event(&yell, &event))
			switch (event.type) {
			case YEVENT_MESSAGE:

				break;
			default:

				break;
			}

		current = clock();

		if (FRAMES_PER_SEC > (double)(current - last) / CLOCKS_PER_SEC)
			continue;

		last = current;

		clear();
		move(0, 0);

		height = getmaxy(stdscr);
		width = getmaxx(stdscr);

#ifdef DEBUG
		printw("%d, %d\n", height, width);
#endif

		delwin(map_window);
		map_window = newwin(MAP_H, MAP_W, (height - MAP_H) / 2, (width - MAP_W) / 2);
		wborder(map_window, '*', '*', '*', '*', '*', '*', '*', '*');

		if (height <= MAP_H || width <= MAP_W) {
			move(height / 2, (width / 2) - 13);
			printw("Please resize the terminal.\n");
			move((height / 2) + 1, (width / 2) - 20);
			printw("It is not large enough to print the map.\n");

			// still check if the user wants to quit
			while ((ch = getch()) != ERR)
				if (ch == 'q')
					game = false;

			continue;
		}
		
		// get user input
		while ((ch = getch()) != ERR) {
#ifdef DEBUG
			move(1, 0);
			addch(ch);
#endif

			switch (ch) {
			case KEY_RIGHT:
				sprite[j].x++;

				break;
			case KEY_LEFT:
				sprite[j].x--;

				break;
			case KEY_UP:
				sprite[j].y--;

				break;
			case KEY_DOWN:
				sprite[j].y++;

				break;
			case 'w':
				if (j == 0)
					j = 3;
				else
					--j;

				break;
			case 'e':
				if (j == 3)
					j = 0;
				else
					++j;

				break;
			case 'q':
				game = false;

				break;
			default:

				break;
			}
		}

		for (i = 0; i < 4; ++i)
			push_sprite(&map, &sprite[i]);

		push_sprite(&map, &tree);

		print_sprites(map_window, &map);

		move(0,0);

		wrefresh(map_window);
		refresh();
		free_sprites(&map);
	}

	clear();
	endwin();

	return 0;
}

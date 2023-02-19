#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "whisper.h"

#define FRAMES_PER_SEC  (1.0 / 30.0)

FILE *logfile;

void print_log(void) {
	int c;

	fseek(logfile, 0, SEEK_SET);

	c = getc(logfile);

	for (; c != EOF; c = getc(logfile))
		putc(c, stdout);

	fclose(logfile);
}

int main(void) {
	int game,
	    ch,
	    height, width,
	    c, i, j,
	    y_new, x_new;
	clock_t last, current;
	WINDOW *map_window;
	yellself_t self;
	event_t *event_v;
	map_t map;
	char msg[BUFFER_SIZE], buf[BUFFER_SIZE],
	     name[NAME_SIZE], speaker[NAME_SIZE];
	struct player_node *pnode;
	player_t *node_player;
	yellnode_t *node;

	// initialize yell

	atexit(print_log);
	logfile = tmpfile();

	fprintf(logfile, "Whisper started.\n");

	// initialize ncurses

	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	
	if (has_colors() == FALSE) {
		fprintf(stderr, "Terminal does not support colors.\n");

		exit(EXIT_FAILURE);
	}

	echo();
	nodelay(stdscr, FALSE);

	read_input(stdscr, name, "Choose a display name:");

	// initialize yell

	yell_init(&self, name);
	yell_debug(&self, logfile);

	noecho();
	nodelay(stdscr, TRUE);

	//start_color();
	//init_pair(1, COLOR_WHITE, COLOR_BLUE);

	player_create(&map.player, "O_O", NULL, 8, 8);

	map.player_ll = NULL;
	map.sprite_ll = NULL;

	game = true;
	last = 0;

	// game loop
	while (game) {
		// parse yell events
		for (; (event_v = yell_nextevent(&self)) != NULL; yell_freeevent(event_v)) {
			i = 0;

			if (event_v->event->buf[i] == '(') {
				++i;

				for (j = 0; event_v->event->buf[i] != ')'; ++j, ++i) {
					speaker[j] = event_v->event->buf[i];
				}

				// skip the space that follows a name
				i += 2;

				speaker[j] = '\0';
			} else {
				fprintf(logfile, "Strange packet received.\n");

				continue;
			}

			node = yell_node(&self, speaker);

			if (node == NULL) {
				fprintf(logfile, "Invalid speaker: %s\n", speaker);

				continue;
			}

			if (event_v->event->buf[i] == '(') {
				++i;

				sscanf(&event_v->event->buf[i], "%d,%d", &y_new, &x_new);

				node_player = player_node(&map, node);
				player_move(node_player, y_new, x_new);

				continue;
			}

			strcpy(node->event.buf, event_v->event->buf);
			node->event.type = event_v->event->type;
		}

		current = clock();

		if (FRAMES_PER_SEC > (double)(current - last) / CLOCKS_PER_SEC)
			continue;

		last = current;

		fprintf(stderr, "GOT HERE.\n");

		clear();
		move(0, 0);

		height = getmaxy(stdscr);
		width = getmaxx(stdscr);
#ifdef DEBUG
		printw("%d, %d\n", height, width);
#endif

		delwin(map_window);
		map_window = newwin(MAP_H, MAP_W, (height - MAP_H) / 2, (width - MAP_W) / 2);
		box(map_window, 0, 0);

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
			case 'q':
				game = false;

				break;
			case 'y':
				echo();
				nodelay(stdscr, FALSE);

				read_input(stdscr, msg, "What will you yell?");

				sprintf(buf, "(%s) ", self.name);
				strcat(buf, msg);

				yell(&self, buf);

				noecho();
				nodelay(stdscr, TRUE);

				break;
			case 'c':
				echo();
				nodelay(stdscr, FALSE);

				add_peer(&self, &map, stdscr);

				noecho();
				nodelay(stdscr, TRUE);

				break;
			default:
				if (player_ctrl(&map.player, ch)) {
					sprintf(buf, "(%s) (%d,%d)", self.name, map.player.sprite.y, map.player.sprite.x);
					yell(&self, buf);
				}

				break;
			}
		}

		player_update(&map.player);
		push_sprite(&map, &map.player.sprite);

		for (pnode = map.player_ll; pnode != NULL; pnode = pnode->next) {
			player_update(pnode->player);
			push_sprite(&map, &pnode->player->sprite);
		}

		print_sprites(map_window, &map);

		move(height - 1,0);
		printw("Listening on port %d. Unique node name is %s.\n", self.port, self.name);

		wrefresh(map_window);
		refresh();

		empty_sprites(&map);
	}

	yell_exit(&self);

	clear();
	endwin();

	exit(EXIT_SUCCESS);
}

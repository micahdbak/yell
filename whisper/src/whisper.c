#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "whisper.h"

void read_input(WINDOW *window, char *dst, char *description) {
	char buf[1024];
	int h, w,
	    len_desc, len,
	    ch, i;

	len_desc = strlen(description);

	for (buf[0] = '\0', i = 0;;) {
		h = getmaxy(window);
		w = getmaxx(window);

		len = strlen(buf);

		wclear(window);
		mvwprintw(window, h / 2, (w / 2) - (len_desc / 2), "%s", description);
		mvwprintw(window, h / 2 + 1, (w / 2) - (len / 2), "%s", buf);
		wrefresh(window);

		ch = getch();

		if (ch == '\n')
			break;

		if (ch == KEY_BACKSPACE || ch == KEY_DL) {
			i -= 2;

			if (i < -1)
				i = -1;
		} else
		if (isprint(ch))
			buf[i] = ch;

		buf[++i] = '\0';
	}

	strcpy(dst, buf);
}

void display_message(WINDOW *window, char *message) {
	char *prompt = "(Press any key to continue.)";
	int h, w,
	    len, len_prompt;

	h = getmaxy(window);
	w = getmaxx(window);

	len = strlen(message);
	len_prompt = strlen(prompt);

	wclear(window);
	mvwprintw(window, h / 2, (w / 2) - (len / 2), "%s", message);
	mvwprintw(window, h / 2 + 1, (w / 2) - (len_prompt / 2), "%s", prompt);
	wrefresh(window);

	getch();
}

void add_peer(yellself_t *self, map_t *map, WINDOW *window) {
	char addr[256], port[256];
	yellnode_t *node;
	player_t *player;

	read_input(window, addr, "Domain of node (e.g., 127.0.0.1):");
	read_input(window, port, "Port of node (e.g., 5001):");

	fprintf(logfile, "Attempted connection with %s:%s\n", addr, port);

	node = yell_add(self, addr, atoi(port));

	if (node == NULL) {
		display_message(window, "Couldn't connect to node.");

		return;
	}

	display_message(window, "Succesfully connected.");

	player = (player_t *)malloc(sizeof(player_t));
	player_create(player, "x~x", node, rand() % MAP_H, rand() % MAP_W);
	push_player(map, player);
}

void push_sprite(map_t *map, sprite_t *sprite) {
	struct sprite_node *new_node, *node;

	if (map->sprite_ll == NULL) {
		map->sprite_ll = malloc(sizeof(struct sprite_node));

		map->sprite_ll->sprite = sprite;
		map->sprite_ll->next = NULL;

		return;
	}

	new_node = malloc(sizeof(struct sprite_node));
	new_node->sprite = sprite;

	if (sprite->y < map->sprite_ll->sprite->y) {
		new_node->next = map->sprite_ll;
		map->sprite_ll = new_node;

		return;
	}

	for (node = map->sprite_ll; node->next != NULL; node = node->next) {
		if (sprite->y >= node->sprite->y && sprite->y < node->next->sprite->y) {
			new_node->next = node->next;
			node->next = new_node;

			return;
		}
	}

	new_node->next = NULL;
	node->next = new_node;
}

void print_sprites(WINDOW *window, map_t *map) {
	struct sprite_node *node;
	int y, x,
	    i,
	    y_start, x_start;

	for (node = map->sprite_ll; node != NULL; node = node->next) {
		for (i = 0, y = 0;; ++y) {
			y_start = node->sprite->y - node->sprite->y_shift + y;
			x_start = node->sprite->x - node->sprite->x_shift;

			if (y_start < 0 || y_start >= getmaxy(window)) {
				while (node->sprite->art[i] != '\n'
				    && node->sprite->art[i] != '\0')
					++i;

				if (node->sprite->art[i] == '\0')
					break;

				++i;

				continue;
			}

			wmove(window, y_start, x_start >= 0 ? x_start : 0);

			for (x = 0; node->sprite->art[i] != '\n'
			         && node->sprite->art[i] != '\0'; ++i, ++x) {
				if (x_start + x < 0 || x_start + x >= getmaxx(window))
					continue;

				waddch(window, node->sprite->art[i]);
			}

			// skip the new-line character
			++i;
		}
	}
}

void empty_sprites(map_t *map) {
	struct sprite_node *node, *next;

	for (node = map->sprite_ll; node != NULL; node = next) {
		next = node->next;
		free(node);
	}

	map->sprite_ll = NULL;
}

void player_create(player_t *player, char face[4], yellnode_t *node, int y, int x) {
	strcpy(player->face, face);
	player->sprite.art = (char *)malloc(sizeof(char) * ART_SIZE);
	player->sprite.y = y;
	player->sprite.x = x;
	player->sprite.y_shift = 2;
	player->sprite.x_shift = 1;
	player->node = node;
}

int player_ctrl(player_t *player, int ch) {
	int dy, dx;

	dy = (ch == KEY_DOWN) - (ch == KEY_UP);
	dx = (ch == KEY_RIGHT) - (ch == KEY_LEFT);

	player->sprite.y += dy;
	player->sprite.x += dx;

	return dy != 0 || dx != 0;
}

void player_move(player_t *player, int y, int x) {
	player->sprite.y = y;
	player->sprite.x = x;
}

void player_update(player_t *player) {
	static const char *default_art = "\n/|\\\n/ \\";
	
	bzero(player->sprite.art, ART_SIZE);
	strcpy(player->sprite.art, player->face);
	strcat(player->sprite.art, " ");

	if (player->node != NULL)
		strcat(player->sprite.art, player->node->event.buf);

	strcat(player->sprite.art, default_art);
}

player_t *player_node(map_t *map, yellnode_t *yellnode) {
	struct player_node *node;

	for (node = map->player_ll; node != NULL; node = node->next)
		if (node->player->node == yellnode)
			return node->player;

	return NULL;
}

void push_player(map_t *map, player_t *player) {
	struct player_node *node;

	if (map->player_ll == NULL) {
		map->player_ll = (struct player_node *)malloc(sizeof(struct player_node));
		map->player_ll->player = player;
		map->player_ll->next = NULL;
	}

	for (node = map->player_ll; node->next != NULL; node = node->next)
		;

	node->next = (struct player_node *)malloc(sizeof(struct player_node));
	node = node->next;
	node->player = player;
	node->next = NULL;
}

void remove_player(map_t *map, player_t *player) {
	struct player_node *node, *cut;

	if (map->player_ll->player == player) {
		node = map->player_ll;
		map->player_ll = node->next;

		free(node);

		return;
	}

	for (node = map->player_ll; node->next != NULL; node = node->next) {
		if (node->next->player == player) {
			cut = node->next;
			node->next = cut->next;

			free(cut);

			break;
		}
	}
}

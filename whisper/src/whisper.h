/* whisper: A Terminal Game of Telephone
 * Author:  Micah Baker
 * Date:    18 February 2023
 */

#ifndef WHISPER_H
#define WHISPER_H

#include <ncurses.h>

#define DEBUG

typedef struct sprite {
	char *art;
	int y, x, h;
} sprite_t;

typedef struct map {
	struct sprite_node {
		sprite_t *sprite;
		struct sprite_node *next;
	} *sprite_ll;
} map_t;

void push_sprite(map_t *map, sprite_t *sprite);
void print_sprites(WINDOW *window, map_t *map);
void free_sprites(map_t *map);

#endif

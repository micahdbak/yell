/* whisper: A Terminal Game of Telephone
 * Author:  Micah Baker
 * Date:    18 February 2023
 */

#ifndef WHISPER_H
#define WHISPER_H

#include <ncurses.h>
#include <yell.h>

#define ART_SIZE  256

#define MAP_H  32
#define MAP_W  64

#define DEBUG

extern FILE *logfile;

typedef struct sprite {
	char *art;
	int y, x,
	    y_shift, x_shift,
	    h;
} sprite_t;

typedef struct player {
	sprite_t sprite;
	yellnode_t *node;
	char face[4], message[1024];
} player_t;

typedef struct map {
	player_t player;
	struct player_node {
		player_t *player;
		struct player_node *next;
	} *player_ll;
	struct sprite_node {
		sprite_t *sprite;
		struct sprite_node *next;
	} *sprite_ll;
} map_t;

void read_input(WINDOW *window, char *dst, char *description);
void display_message(WINDOW *window, char *message);

void add_peer(yellself_t *self, map_t *map, WINDOW *window);

void push_sprite(map_t *map, sprite_t *sprite);
void print_sprites(WINDOW *window, map_t *map);
void empty_sprites(map_t *map);

void player_create(player_t *player, char face[4], yellnode_t *node, int y, int x);
int player_ctrl(player_t *player, int ch);
void player_move(player_t *player, int y, int x);
void player_update(player_t *player);
player_t *player_node(map_t *map, yellnode_t *node);

void push_player(map_t *map, player_t *player);
void remove_player(map_t *map, player_t *player);

#endif

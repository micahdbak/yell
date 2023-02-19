#include <stdlib.h>

#include "whisper.h"

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

	if (sprite->y <= map->sprite_ll->sprite->y) {
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
	int y, i;

	for (node = map->sprite_ll; node != NULL; node = node->next) {
		i = 0;

		for (y = 0; y < node->sprite->h; ++y) {
			wmove(window, node->sprite->y + y, node->sprite->x);

			for (; node->sprite->art[i] != '\n'; ++i)
				waddch(window, node->sprite->art[i]);

			// skip the new-line character
			++i;
		}
	}
}

void free_sprites(map_t *map) {
	struct sprite_node *node, *next;

	for (node = map->sprite_ll; node != NULL; node = next) {
		next = node->next;
		free(node);
	}

	map->sprite_ll = NULL;
}

#include <stdlib.h>

#include "yell.h"

int yell_connect(yell_t *node) {
	return 0;
}

int yell_message(yell_t *yell, const char *message) {
	return 0;
}

int yell_private(yell_t *yell, ynode_t *node, const char *message) {
	return 0;
}

int yell_event(yell_t *yell, yevent_t *event) {
	event->type = YEVENT_MESSAGE;

	return 0;
}

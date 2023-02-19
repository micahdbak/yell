/* libyell: C implimentation of the yell peer-to-peer protocol
 * Author:  Micah Baker
 * Date:    18 February 2023
 */

#ifndef YELL_H
#define YELL_H

#include <stdint.h>

#define YEVENT_MESSAGE  0b1

typedef struct ynode {
	int id;
} ynode_t;

typedef struct yevent {
	int type;
	uint8_t *message;
} yevent_t;

typedef struct yell {
	struct ynode_array {
		ynode_t *node;
		int capacity, index;
	} nodes;
	struct yevent_array {
		yevent_t *event;
		int capacity, index;
	} events;
} yell_t;

// initiate a connection with a node
int yell_connect(yell_t *yell);

// yell a message to every node
int yell_message(yell_t *yell, const char *message);

// yell a message to a specific node
int yell_private(yell_t *yell, ynode_t *node, const char *message);

// parse events in the yell queue
int yell_event(yell_t *yell, yevent_t *event);

#endif

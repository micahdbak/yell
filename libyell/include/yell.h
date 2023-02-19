/* libyell: C implimentation of the yell peer-to-peer protocol
 * Author:  Micah Baker
 * Date:    18 February 2023
 */

#ifndef YELL_H
#define YELL_H

#include <stdint.h>
#include <netinet/in.h>
#include <pthread.h>

#define YELL_SUCCESS  0
#define YELL_FAILURE  1

#define YELLEVENT_MESSAGE   0b0001
#define YELLEVENT_RESPONSE  0b0010
#define YELLEVENT_PING      0b0100
#define YELLEVENT_OK        0b1000

#define BUFFER_SIZE  1024
#define NAME_SIZE    64

typedef struct yellevent {
	char buf[BUFFER_SIZE];
	int type;
} yellevent_t;

typedef struct yellnode {
	char name[NAME_SIZE];
	int fd;
	struct sockaddr_in addr;
	yellevent_t event;
} yellnode_t;

typedef struct yellevent_node {
	yellevent_t *event;
	struct yellevent_node *next;
} eventnode_t;

typedef struct yellevent_verbose {
	yellevent_t *event;
} event_t;

typedef struct yellself {
	char name[NAME_SIZE];
	int fd, port,
	    close;
	struct sockaddr_in addr;
	pthread_t thread;
	pthread_mutex_t close_mutex, events_mutex;
	struct yellnode_ll {
		yellnode_t *node;
		struct yellnode_ll *next;
	} *nodes;
	struct yellevent_ll {
		eventnode_t *head, *tail;
	} events;
} yellself_t;

// allow other nodes to connect to self
int yell_init(yellself_t *self, const char *name);

// connect to a node, and 
yellnode_t *yell_add(yellself_t *self, const char *addr, int port);

// find node by name
yellnode_t *yell_node(yellself_t *self, const char *name);

// print information about self and connected nodes
void yell_debug(yellself_t *self, FILE *dst);

// yell a message to a specific node
yellevent_t *yell_private(yellself_t *self, yellnode_t *node, const char *message);

// yell a message to every connected node
int yell(yellself_t *self, const char *message);

// parse events in the yell queue
event_t *yell_nextevent(yellself_t *self);

// free event node
void yell_freeevent(event_t *event);

// close all node connections and threads
void yell_exit(yellself_t *self);

#endif

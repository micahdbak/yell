/*********************************************
 **  libyell: C implimentation of the yell  **
 **           node communication protocol.  **
 *********************************************/

#ifndef YELL_H
#define YELL_H

#include <netinet/in.h>
#include <pthread.h>

#include "LL.h"

#define YELL_SUCCESS  0
#define YELL_FAILURE  1

#define MIN_PORT  5000
#define MAX_PORT  5100

#define MAX_CONNECTIONS  100

// each packet holds maximum one kilobyte
#define PACKET_SIZE  1024

enum yell_event_type {
	YET_PING,
	YET_MESSAGE,
	YET_CONNECTION,
	YET_DISCONNECTION
};

struct yell_peer {
	struct sockaddr_in sockaddr;
};

struct yell_event {
	char packet[PACKET_SIZE];
	enum yell_event_type type;
	struct yell_peer *peer;
};

struct yell {
	FILE *log;

	int close;
	pthread_mutex_t close_mutex;

	int sockfd, sockport;
	struct sockaddr_in sockaddr;
	pthread_t listen_thread;

	struct LL events, peers;
	pthread_mutex_t events_mutex, peers_mutex;
};

struct yell_peer *yell_findpeer(struct yell *self, struct sockaddr *sockaddr, socklen_t *addrlen);

int yell_start(FILE *log, struct yell *self);

struct yell_peer *yell_makepeer(const char *addr, int port);

int yell_connect(struct yell *self, struct yell_peer *peer);

int yell_peer(struct yell *self, struct yell_peer *peer, const char *message);

int yell(struct yell *self, const char *message);

struct yell_event *yell_event(struct yell *self);

void yell_freeevent(struct yell_event *event);

void yell_freepeer(struct yell_peer *peer);

void yell_exit(struct yell *self);

void yell_addrf(FILE *file, const char *format, struct sockaddr_in *sockaddr);

void yell_debugf(FILE *file, struct yell *self);

void yell_logf(FILE *file, struct yell *self);

#endif

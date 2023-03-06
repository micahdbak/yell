/*********************************************
 **  libyell: C implimentation of the yell  **
 **           node communication protocol.  **
 *********************************************/

#ifndef YELL_H
#define YELL_H

#include <netinet/in.h>
#include <pthread.h>

#include "yell_LL.h"

#define YELL_SUCCESS  0
#define YELL_FAILURE  1

#define MIN_PORT  5000
#define MAX_PORT  5100

#define MAX_CONNECTIONS  100

#define NAME_SIZE  64

// each packet holds maximum one kilobyte
#define PACKET_SIZE  1024

enum yell_eventtype {
	YET_UNKNOWN       = '\0',
	YET_SUCCESS       = 's',
	YET_FAILURE       = 'f',
	YET_PING          = 'p',
	YET_WHOAREYOU     = 'w',
	YET_MESSAGE       = 'm',
	YET_CONNECTION    = 'c',
	YET_DISCONNECTION = 'd'
};

struct yell_peer {
	char name[NAME_SIZE + 1];
	struct sockaddr_in sockaddr;
};

struct yell_event {
	char packet[PACKET_SIZE + 1];
	enum yell_eventtype type;
	struct yell_peer *peer;
};

struct yell {
	FILE *log;

	char name[NAME_SIZE + 1];

	int close;
	pthread_mutex_t close_mutex;

	int sockfd, sockport;
	struct sockaddr_in sockaddr;

	pthread_t listen_thread;
	void (*handler)(struct yell *, struct yell_event *);

	struct yell_LL events, peers;
	pthread_mutex_t events_mutex, peers_mutex;
};

struct yell_peer *yell_findpeer(struct yell *self, const char *name);
int yell_peer(struct yell *self, struct yell_peer *peer, enum yell_eventtype type, const char *message, char *response);
struct yell_peer *yell_addpeer(struct yell *self, const char *addr, int port);
void yell_freepeer(struct yell_peer *peer);

struct yell_event *yell_event(struct yell *self);
void yell_freeevent(struct yell_event *event);
void yell_handleevent(struct yell_event *);
int yell_start(FILE *log, struct yell *self, const char *name, void (*handler)(struct yell *, struct yell_event *));
int yell(struct yell *self, const char *message);
void yell_exit(struct yell *self);

void yell_peerf(FILE *file, const char *format, const char *name, struct sockaddr_in *sockaddr);
void yell_debugf(FILE *file, struct yell *self);
void yell_logf(FILE *file, struct yell *self);

#endif

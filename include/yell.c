#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "yell.h"

#define YELL_SUCCESS_C        's'
#define YELL_FAILURE_C        'f'
#define YELL_PING_C           'p'
#define YELL_MESSAGE_C        'm'
#define YELL_CONNECTION_C     'c'
#define YELL_DISCONNECTION_C  'd'

struct yell_peer *yell_findpeer(struct yell *self, struct sockaddr *sockaddr, socklen_t *addrlen) {
	struct LL_node *march;
	struct yell_peer *peer;

	pthread_mutex_lock(&self->peers_mutex);

	for (march = self->peers.head; march != NULL; march = march->next) {
		peer = (struct yell_peer *)march->data;

		// found peer
		if (memcmp(&peer->sockaddr, &sockaddr, *addrlen) == 0) {
			pthread_mutex_unlock(&self->peers_mutex);

			return peer;
		}
	}

	pthread_mutex_unlock(&self->peers_mutex);

	// couldn't find peer
	return NULL;
}

int yell_pushevent(struct yell *self, struct yell_peer *peer, const char *packet, char *response) {
	const char *fname = "yell_pushevent()";

	struct yell_event *event;

	event = (struct yell_event *)malloc(sizeof(struct yell_event));

	// memory allocation error
	if (event == NULL) {
		fprintf(self->log, "%s: free(): %s\n", fname, strerror(errno));

		return YELL_FAILURE;
	}

	// set the event's peer
	event->peer = peer;

	// copy the packet into this event (skipping packet type character)
	strcpy(event->packet, &packet[1]);

	// no response by default
	response[0] = '\0';

	// get event type
	switch (packet[0]) {
	// ping
	case YELL_PING_C:
		event->type = YET_PING;

		// pong
		response[0] = YELL_PING_C;
		response[1] = '\0';

		break;
	// message
	case YELL_MESSAGE_C:
		event->type = YET_MESSAGE;

		// respond as successful
		response[0] = YELL_SUCCESS_C;
		response[1] = '\0';

		break;
	// connection
	case YELL_CONNECTION_C:
		// TODO: parse connections

		break;
	// disconnection
	case YELL_DISCONNECTION_C:
		// TODO: parse disconnections

		break;
	// unknown type
	default:
		fprintf(self->log, "%s: Strange packet received.\n", fname);

		// free event
		free(event);

		// respond as failure
		response[0] = YELL_FAILURE_C;
		response[1] = '\0';

		return YELL_FAILURE;
	}

	pthread_mutex_lock(&self->events_mutex);

	// attempt to insert event into linked list
	if (LL_insert(&self->events, LL_TAIL, event) == LL_FAILURE) {
		fprintf(self->log, "%s: Couldn't insert event into linked list.\n", fname);

		// free event
		free(event);

		pthread_mutex_unlock(&self->events_mutex);

		return YELL_FAILURE;
	}

	pthread_mutex_unlock(&self->events_mutex);

	return YELL_SUCCESS;
}

void *yell_listen(void *self_ptr) {
	const char *fname = "yell_listen";

	struct yell       *self = (struct yell *)self_ptr;
	int                peerfd, nbytes;
	socklen_t          addrlen = sizeof(struct sockaddr_in),
			   addrlen_peer;
	char               packet[PACKET_SIZE + 1],
	                   response[PACKET_SIZE + 1];
	struct sockaddr    sockaddr_peer;
	struct yell_peer  *peer;

	for (;;) {
		pthread_mutex_lock(&self->close_mutex);

		// close thread if this is false
		if (self->close)
			break;

		pthread_mutex_unlock(&self->close_mutex);

		// accept a connection queued on the socket
		peerfd = accept(self->sockfd, (struct sockaddr *)&self->sockaddr,
		                &addrlen);

		// check if connection was successful
		if (peerfd < 0) {
			fprintf(self->log, "%s: accept(): %s\n", fname, strerror(errno));

			// continue on maximum queued connections
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			else {
				// TODO: attempt to recover
			}

			break;
		}

		// find which peer this connection is from
		if (getsockname(peerfd, &sockaddr_peer, &addrlen_peer) < 0
		 || (peer = yell_findpeer(self, &sockaddr_peer, &addrlen_peer)) == NULL) {
			fprintf(self->log, "%s: Connection received from unknown peer.\n", fname);

			// close the connection
			close(peerfd);

			continue;
		}

		// attempt to receive a packet
		nbytes = read(peerfd, packet, PACKET_SIZE);

		if (nbytes < 0) {
			fprintf(self->log, "%s: read(): %s\n", fname, strerror(errno));

			// close the connection
			close(peerfd);

			continue;
		}

		// ensure packet is null-terminated
		packet[nbytes] = '\0';

		if (yell_pushevent(self, peer, packet, response) == YELL_FAILURE) {
			// attempt to respond as failed

			response[0] = YELL_FAILURE;
			response[1] = '\0';

			write(peerfd, response, strlen(response));

			// close connection
			close(peerfd);

			continue;
		}

		write(peerfd, response, strlen(response));

		// close connection
		close(peerfd);
	}

	return NULL;
}

int yell_start(FILE *log, struct yell *self) {
	const char *fname = "yell_start";

	// no log file provided
	if (log == NULL) {
		// print to a temporary file
		self->log = tmpfile();
	}

	// open the socket to receive messages
	self->sockfd = socket(AF_INET, SOCK_STREAM, 0);

	// fail if couldn't open socket
	if (self->sockfd < 0) {
		fprintf(self->log, "%s: socket(): %s\n", fname, strerror(errno));

		return YELL_FAILURE;
	}

	// prepare ipv4 internet address

	memset(&self->sockaddr, 0, sizeof(struct sockaddr_in));

	self->sockaddr.sin_family = AF_INET;
	// listen to any incoming connections
	self->sockaddr.sin_addr.s_addr = INADDR_ANY;

	// find an available port to bind with
	for (self->sockport = MIN_PORT; self->sockport < MAX_PORT; ++self->sockport) {
		self->sockaddr.sin_port = htons(self->sockport);

		// attempt to bind
		if (bind(self->sockfd, (struct sockaddr *)&self->sockaddr,
		                       sizeof(struct sockaddr_in)) == 0)
			// break loop on success
			break;
	}

	// couldn't find an available port
	if (self->sockport == MAX_PORT) {
		fprintf(self->log, "%s: bind(): %s\n", fname, strerror(errno));

		// close the socket
		close(self->sockfd);

		return YELL_FAILURE;
	}

	// attempt to start listening for connections
	if (listen(self->sockfd, MAX_CONNECTIONS) < 0) {
		fprintf(self->log, "%s: listen(): %s\n", fname, strerror(errno));

		// close the socket
		close(self->sockfd);

		return YELL_FAILURE;
	}

	// socket is prepared; initialize data

	self->close = 0;
	pthread_mutex_init(&self->close_mutex, NULL);

	// attempt to open listen thread
	if (pthread_create(&self->listen_thread, NULL,
	                   yell_listen, (void *)self) < 0) {
		fprintf(self->log, "%s: pthread_create(): %s\n", fname, strerror(errno));

		// close the socket
		close(self->sockfd);

		return YELL_FAILURE;
	}

	// initialize events linked list
	self->events.head = NULL;
	pthread_mutex_init(&self->events_mutex, NULL);

	// initialize peers linked list
	self->peers.head = NULL;
	pthread_mutex_init(&self->peers_mutex, NULL);

	return YELL_SUCCESS;
}

struct yell_peer *yell_makepeer(const char *addr, int port) {
	const char *fname = "yell_makepeer";

	struct yell_peer *peer;

	peer = (struct yell_peer *)malloc(sizeof(struct yell_peer));

	// memory allocation error
	if (peer == NULL)
		return NULL;

	memset(&peer->sockaddr, 0, sizeof(struct sockaddr_in));

	peer->sockaddr.sin_family = AF_INET;
	peer->sockaddr.sin_addr.s_addr = inet_addr(addr);
	peer->sockaddr.sin_port = htons(port);

	return peer;
}

int yell_connect(struct yell *self, struct yell_peer *peer) {
	const char *fname = "yell_connect";

	pthread_mutex_lock(&self->peers_mutex);	

	// attempt to insert this peer into linked list
	if (LL_insert(&self->peers, LL_TAIL, (void *)peer) == LL_FAILURE) {
		fprintf(self->log, "%s: Couldn't insert peer into linked list.\n", fname);

		pthread_mutex_unlock(&self->peers_mutex);	

		return YELL_FAILURE;
	}

	pthread_mutex_unlock(&self->peers_mutex);	

	return YELL_SUCCESS;
}

int yell_peer(struct yell *self, struct yell_peer *peer, const char *message) {
	const char *fname = "yell_peer";

	char packet[PACKET_SIZE + 1],
	     response[PACKET_SIZE + 1];
	int peerfd, nbytes;

	// attempt to open socket
	peerfd = socket(AF_INET, SOCK_STREAM, 0);

	if (peerfd < 0) {
		fprintf(self->log, "%s: socket(): %s\n", fname, strerror(errno));

		return YELL_FAILURE;
	}

	// attempt to connect to peer
	if (connect(peerfd, (struct sockaddr *)&peer->sockaddr,
	                    sizeof(struct sockaddr_in)) < 0) {
		fprintf(self->log, "%s: connect(): %s\n", fname, strerror(errno));

		// close socket
		close(peerfd);

		return YELL_FAILURE;
	}

	// connected to peer

	// write message
	if (write(peerfd, message, strlen(message)) < 0) {
		fprintf(self->log, "%s: write(): %s\n", fname, strerror(errno));

		// close socket
		close(peerfd);

		return YELL_FAILURE;
	}

	// read response
	nbytes = read(peerfd, packet, PACKET_SIZE);

	if (nbytes < 0) {
		fprintf(self->log, "%s: read(): %s\n", fname, strerror(errno));

		// close socket
		close(peerfd);

		return YELL_FAILURE;
	}

	packet[nbytes] = '\0';

	// peer responded to packet with another response
	if (packet[0] != YELL_SUCCESS) {
		// attempt to push this packet as an event
		if (yell_pushevent(self, peer, packet, response) == YELL_FAILURE) {
			// TODO: handle this
		}

		// attempt to respond
		write(peerfd, response, strlen(response));
	}

	// close socket
	close(peerfd);

	return YELL_SUCCESS;
}

int yell(struct yell *self, const char *message) {
	struct LL_node *march;

	pthread_mutex_lock(&self->peers_mutex);

	// for every connected peer..
	for (march = self->peers.head; march != NULL; march = march->next)
		// ...yell the message to that peer
		yell_peer(self, (struct yell_peer *)march->data, message);

	pthread_mutex_unlock(&self->peers_mutex);

	return YELL_SUCCESS;
}

struct yell_event *yell_event(struct yell *self) {
	struct yell_event *event;

	pthread_mutex_lock(&self->events_mutex);

	event = (struct yell_event *)LL_remove(&self->events, LL_HEAD);

	pthread_mutex_unlock(&self->events_mutex);

	return event;
}

void yell_freeevent(struct yell_event *event) {
	free(event);
}

void yell_freepeer(struct yell_peer *peer) {
	free(peer);
}

void yell_exit(struct yell *self) {
	struct yell_event *event;
	struct yell_peer *peer;

	// signal for listen thread to close

	pthread_mutex_lock(&self->close_mutex);

	self->close = 1;

	pthread_mutex_unlock(&self->close_mutex);

	// join the listen thread
	pthread_join(self->listen_thread, NULL);

	// stop listening for connections
	close(self->sockfd);

	pthread_mutex_destroy(&self->close_mutex);
	pthread_mutex_destroy(&self->events_mutex);
	pthread_mutex_destroy(&self->peers_mutex);

	while ((event = LL_remove(&self->events, LL_HEAD)) != NULL)
		yell_freeevent(event);

	while ((peer = LL_remove(&self->events, LL_HEAD)) != NULL)
		yell_freepeer(peer);
}

void yell_addrf(FILE *file, const char *format, struct sockaddr_in *sockaddr) {
	char addr[INET_ADDRSTRLEN];
	int port;

	inet_ntop(sockaddr->sin_family, &sockaddr->sin_addr, addr, INET_ADDRSTRLEN);
	port = ntohs(sockaddr->sin_port);

	for (; *format != '\0'; ++format) {
		switch (*format) {
		// print address
		case '@':
			if (*(format + 1) == '@') {
				++format;
				putc('@', file);
			} else
				fprintf(file, "%s", addr);

			break;
		// print port
		case '%':
			if (*(format + 1) == '%') {
				++format;
				putc('%', file);
			} else
				fprintf(file, "%d", port);

			break;
		// print character
		default:
			putc(*format, file);

			break;
		}
	}
}

void yell_debugf(FILE *file, struct yell *self) {
	struct LL_node *march;
	struct yell_peer *peer;
	int npeers;

	fprintf(file, "--- begin yell debug ---\n");

	fprintf(file, "Self is:\n");
	yell_addrf(file, "\t@:%\n", &self->sockaddr);

	pthread_mutex_lock(&self->peers_mutex);

	fprintf(file, "Connected to:\n");

	npeers = 0;

	for (march = self->peers.head; march != NULL; march = march->next) {
		peer = (struct yell_peer *)march->data;
		yell_addrf(file, "\t@:%\n", &peer->sockaddr);

		++npeers;
	}

	fprintf(file, "\t... %d peers.\n", npeers);

	pthread_mutex_unlock(&self->peers_mutex);

	fprintf(file, "--- end yell debug ---\n");
}

void yell_logf(FILE *file, struct yell *self) {
	int c;

	fseek(self->log, 0, SEEK_SET);

	c = getc(self->log);

	for (; c != EOF; c = getc(self->log))
		putc(c, file);
}

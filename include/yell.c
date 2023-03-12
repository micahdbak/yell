#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "yell.h"

struct yell_peer *yell_findpeer(struct yell *self, const char *name) {
	struct yell_LL_node *march;
	struct yell_peer *peer;

	pthread_mutex_lock(&self->peers_mutex);

	for (march = self->peers.head; march != NULL; march = march->next) {
		peer = (struct yell_peer *)march->data;

		// found peer
		if (strcmp(peer->name, name) == 0) {
			pthread_mutex_unlock(&self->peers_mutex);

			return peer;
		}
	}

	pthread_mutex_unlock(&self->peers_mutex);

	// couldn't find peer
	return NULL;
}

int yell_pushpeer(struct yell *self, struct yell_peer *peer) {
	const char *fname = "yell_pushpeer()";

	pthread_mutex_lock(&self->peers_mutex);	

	// attempt to insert this peer into linked list
	if (yell_LL_insert(&self->peers, YELL_LL_TAIL, (void *)peer) == YELL_LL_FAILURE) {
		fprintf(self->log, "%s: Couldn't insert peer into linked list.\n", fname);

		free(peer);

		pthread_mutex_unlock(&self->peers_mutex);	

		return YELL_FAILURE;
	}

	pthread_mutex_unlock(&self->peers_mutex);

	return YELL_SUCCESS;
}

int yell_topeer(struct yell *self, struct yell_peer *peer, enum yell_eventtype type, const char *message, char *response) {
	const char *fname = "yell_topeer";

	char packet[PACKET_SIZE + 1];
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

	// create packet
	if (message == NULL)
		sprintf(packet, "%c%s;%d;", (char)type, self->name, self->sockport);
	else
		sprintf(packet, "%c%s;%d;%s", (char)type, self->name, self->sockport, message);

	// write message
	if (write(peerfd, packet, strlen(packet)) < 0) {
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

	if (response != NULL)
		strcpy(response, packet);
	else
	if (packet[0] != YET_SUCCESS)
		fprintf(self->log, "%s: Unhandled response.\n", fname);

	// close socket
	close(peerfd);

	return YELL_SUCCESS;
}

struct yell_event *yell_makeevent(struct yell *self, const char *packet, struct sockaddr_in sockaddr) {
	const char *fname = "yell_makeevent()";

	struct yell_event *event;
	char name[NAME_SIZE + 1];
	struct yell_peer *peer;
	int i, j,
	    sockport;

	event = (struct yell_event *)malloc(sizeof(struct yell_event));

	// memory allocation error
	if (event == NULL) {
		fprintf(self->log, "%s: Memory allocation error.\n", fname);

		return NULL;
	}

	// set event type
	event->type = packet[0];

	// read peer name
	for (i = 1, j = 0; packet[i] != ';' && packet[i] != '\0'; ++i, ++j) {
		// name size is exceeded
		if (j == NAME_SIZE) {
			// skip characters until a semicolon or end of string is reached
			while (packet[i] != ';' && packet[i] != '\0')
				++i;

			break;
		}

		name[j] = packet[i];
	}

	name[j] = '\0';

	// check for invalid syntax
	if (packet[i] == '\0') {
		free(event);

		return NULL;
	}

	// don't read the semicolon
	++i;

	// read the port of this node
	for (sockport = 0; packet[i] != ';' && packet[i] != '\0'; ++i) {
		if (!isdigit(packet[i])) {
			// skip characters until a semicolon or end of string is reached
			while (packet[i] != ';' && packet[i] != '\0')
				++i;

			break;
		}

		sockport = sockport * 10 + packet[i] - '0';
	}

	// check for invalid syntax
	if (sockport == 0 || packet[i] == '\0') {
		free(event);

		return NULL;
	}

	// attempt to find the peer associated with this packet
	peer = yell_findpeer(self, name);

	if (peer == NULL) {
		// create the peer

		peer = (struct yell_peer *)malloc(sizeof(struct yell_peer));

		// memory allocation error
		if (peer == NULL) {
			fprintf(self->log, "%s: Memory allocation error.\n", fname);

			free(event);

			return NULL;
		}

		peer->sockaddr = sockaddr;
		peer->sockaddr.sin_port = htons(sockport);
		peer->sockport = sockport;
		strcpy(peer->name, name);

		// push this peer
		yell_pushpeer(self, peer);
	}

	// set the event's peer
	event->peer = peer;

	if (packet[i] == '\0')
		// syntax error... assume no body provided
		event->packet[0] = '\0';
	else
		// copy body of packet to event->packet
		strcpy(event->packet, &packet[i + 1]);

	return event;
}

int yell_pushevent(struct yell *self, struct yell_event *event) {
	const char *fname = "yell_pushevent()";

	pthread_mutex_lock(&self->events_mutex);

	// attempt to insert event into linked list
	if (yell_LL_insert(&self->events, YELL_LL_TAIL, event) == YELL_LL_FAILURE) {
		fprintf(self->log, "%s: Couldn't insert event into linked list.\n", fname);

		pthread_mutex_unlock(&self->events_mutex);

		return YELL_FAILURE;
	}

	pthread_mutex_unlock(&self->events_mutex);

	return YELL_SUCCESS;
}

struct yell_event *yell_nextevent(struct yell *self) {
	struct yell_event *event;

	pthread_mutex_lock(&self->events_mutex);

	event = (struct yell_event *)yell_LL_remove(&self->events, YELL_LL_HEAD);

	pthread_mutex_unlock(&self->events_mutex);

	return event;
}

void *yell_listen(void *self_ptr) {
	const char *fname = "yell_listen";

	struct yell *self;  // information on self
	int len;

	int                peerfd, nbytes;               // peer socket, number of bytes read
	struct sockaddr_in sockaddr_self, sockaddr_peer; // addresses
	socklen_t          addrlen, addrlen_peer;        // size of sockaddr

	struct yell_LL_node *node;
	struct yell_peer    *peer;

	char packet[PACKET_SIZE + 1],   // received packet
	     response[PACKET_SIZE + 1], // packet to send
	     name[NAME_SIZE + 1],       // name of peer
	     peer_addrstr[512];

	struct yell_event *event; // created event from a packet

	self = (struct yell *)self_ptr;

	addrlen = sizeof(struct sockaddr_in);

	for (;;) {
		// safety measure---on occasion, self->sockaddr is overwritten in errors
		memcpy(&sockaddr_self, &self->sockaddr, sizeof(struct sockaddr_in));

		// accept a connection queued on the socket
		peerfd = accept(self->sockfd, (struct sockaddr *)&sockaddr_self, &addrlen);

		// check if this thread should close

		pthread_mutex_lock(&self->close_mutex);

		if (self->close)
			break;

		pthread_mutex_unlock(&self->close_mutex);

		// bad file descriptor
		if (peerfd < 0) {
			// no incoming connections; check if thread should close
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;

			// there is a different error; log then break
			fprintf(self->log, "%s: %s\n", fname, strerror(errno));

			break;
		}

		// get the address of the received connection
		getsockname(peerfd, (struct sockaddr *)&sockaddr_peer, &addrlen_peer);

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

		// create event from packet
		event = yell_makeevent(self, packet, sockaddr_peer);

		// couldn't create an event---peer is probably sus
		if (event == NULL) {
			close(peerfd);

			continue;
		}

		response[0] = YET_FAILURE;
		response[1] = '\0';

		switch (event->type) {
		case YET_PING:
			// respond by pinging back
			response[0] = YET_PING;

			break;
		case YET_WHOAREYOU:
			// respond with name of self
			strcpy(response, self->name);

			break;
		case YET_MESSAGE:
			response[0] = YET_SUCCESS;
		
			break;
		case YET_CONNECT:
			response[0] = '\0';

			pthread_mutex_lock(&self->peers_mutex);	

			// for every connected peer
			for (node = self->peers.head; node != NULL; node = node->next) {
				peer = (struct yell_peer *)node->data;

				// do not tell peer of its own existence
				if (peer == event->peer) {
					if (node->next == NULL)
						// don't terminate the list with a semicolon
						response[strlen(response) - 1] = '\0';

					continue;
				}

				// prepare peer address string
				if (peer->sockaddr.sin_addr.s_addr == INADDR_ANY)
					strcpy(peer_addrstr, "127.0.0.1");
				else
					inet_ntop(peer->sockaddr.sin_family, &peer->sockaddr.sin_addr, peer_addrstr, INET_ADDRSTRLEN);
				len = strlen(peer_addrstr);
				peer_addrstr[len] = ':';
				sprintf(peer_addrstr + len + 1, "%d", ntohs(peer->sockaddr.sin_port));

				// concatenate this peer to the response
				strcat(response, peer_addrstr);

				if (node->next != NULL)
					strcat(response, ";");
			}

			pthread_mutex_unlock(&self->peers_mutex);	

			printf("%s\n", response);

			break;
		case YET_DISCONNECT:
			// TODO: deal with disconnections
			response[0] = YET_SUCCESS;

			break;
		default:
		case YET_UNKNOWN:
			fprintf(self->log, "%s: Unknown packet event type.\n", fname);

			continue;
		}

		if (write(peerfd, response, strlen(response)) < 0)
			fprintf(self->log, "%s: write(): %s\n", fname, strerror(errno));

		// close connection
		close(peerfd);

		// handle the event
		if (self->event_handler(self, event) == YELL_FAILURE) {
			event = NULL;

			free(event);
		}
	}

	return NULL;
}

int yell_start(FILE *log, struct yell *self, const char *name, int (*event_handler)(struct yell *, struct yell_event *)) {
	const char *fname = "yell_start";
	int nchars;

	// no log file provided
	if (log == NULL) {
		// print to a temporary file
		self->log = tmpfile();
	} else {
		self->log = log;
	}

	// copy the name
	strncpy(self->name, name, NAME_SIZE);
	nchars = strlen(name);

	if (nchars > NAME_SIZE)
		self->name[NAME_SIZE] = '\0';
	else
		self->name[nchars] = '\0';

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

	// set event handler
	if (event_handler == NULL)
		self->event_handler = yell_pushevent;
	else
		self->event_handler = event_handler;

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

struct yell_peer *yell_addpeer(struct yell *self, const char *addr, int port) {
	const char *fname = "yell_addpeer";

	struct yell_peer *peer;
	char name[NAME_SIZE + 1];

	peer = (struct yell_peer *)malloc(sizeof(struct yell_peer));

	// memory allocation error
	if (peer == NULL) {
		fprintf(self->log, "%s: Memory allocation error.\n", fname);

		return NULL;
	}

	memset(&peer->sockaddr, 0, sizeof(addr));

	peer->sockaddr.sin_family = AF_INET;
	peer->sockaddr.sin_addr.s_addr = inet_addr(addr);
	peer->sockaddr.sin_port = htons(port);
	peer->sockport = port;

	// receive node's name
	if (yell_topeer(self, peer, YET_WHOAREYOU, NULL, name) == YELL_FAILURE) {
		fprintf(self->log, "%s: Couldn't message peer.\n", fname);

		free(peer);

		return NULL;
	}

	// message successful; copy name and push peer
	strncpy(peer->name, name, NAME_SIZE);
	peer->name[strlen(name)] = '\0';
	yell_pushpeer(self, peer);

	return peer;
}

int yell_connect(struct yell *self, const char *addr, int port) {
	const char *fname = "yell_connect";

	struct yell_peer *peer;
	char peers[PACKET_SIZE + 1],
	     addrstr[INET_ADDRSTRLEN];
	int i, j,
	    sockport;

	// get the first peer
	peer = yell_addpeer(self, addr, port);

	// receive information about other peers
	if (yell_topeer(self, peer, YET_CONNECT, NULL, peers) == YELL_FAILURE) {
		fprintf(self->log, "%s: Couldn't message peer.\n", fname);

		return YELL_FAILURE;
	}

	// add other peers
	for (i = 0; peers[i] != '\0';) {
		// get address
		for (j = 0; peers[i] != ':' && peers[i] != '\0'; ++i, ++j)
			addrstr[j] = peers[i];

		addrstr[j] = '\0';

		if (peers[i] == '\0')
			break;

		// get port
		if (sscanf(peers + i + 1, "%d", &sockport) <= 0) {
			while (peers[i] != ';' && peers[i] != '\0')
				++i;

			continue;
		}

		printf("%s:%d\n", addrstr, sockport);

		yell_addpeer(self, addrstr, sockport);

		// go to the end of this entry
		while (peers[i] != ';' && peers[i] != '\0')
			++i;

		if (peers[i] == '\0')
			break;

		++i;
	}

	return YELL_SUCCESS;
}

int yell(struct yell *self, const char *message) {
	struct yell_LL_node *march;

	pthread_mutex_lock(&self->peers_mutex);

	// for every connected peer..
	for (march = self->peers.head; march != NULL; march = march->next)
		// ...yell the message to that peer
		yell_topeer(self, (struct yell_peer *)march->data, YET_MESSAGE, message, NULL);

	pthread_mutex_unlock(&self->peers_mutex);

	return YELL_SUCCESS;
}

void yell_exit(struct yell *self) {
	const char *fname = "yell_exit()";

	struct yell_event *event;
	struct yell_peer *peer;

	pthread_mutex_lock(&self->close_mutex);

	// set the close variable
	self->close = 1;

	pthread_mutex_unlock(&self->close_mutex);

	// shutdown and close the socket
	shutdown(self->sockfd, SHUT_RD);
	close(self->sockfd);

	// join the listen thread
	pthread_join(self->listen_thread, NULL);

	pthread_mutex_destroy(&self->close_mutex);
	pthread_mutex_destroy(&self->events_mutex);
	pthread_mutex_destroy(&self->peers_mutex);

	while ((event = yell_LL_remove(&self->events, YELL_LL_HEAD)) != NULL)
		free(event);

	while ((peer = yell_LL_remove(&self->events, YELL_LL_HEAD)) != NULL)
		free(peer);

	fprintf(self->log, "%s: Exited.\n", fname);
}

void yell_peerf(FILE *file, const char *format, const char *name, struct sockaddr_in sockaddr) {
	char addr[INET_ADDRSTRLEN];
	int port;

	if (sockaddr.sin_addr.s_addr == INADDR_ANY)
		strcpy(addr, "127.0.0.1");
	else
		inet_ntop(sockaddr.sin_family, &sockaddr.sin_addr, addr, INET_ADDRSTRLEN);

	port = ntohs(sockaddr.sin_port);

	for (; *format != '\0'; ++format) {
		if (*format == '$') {
			switch (*++format) {
			// print name
			case 'n':
				if (name == NULL)
					break;

				fprintf(file, "%s", name);

				break;
			// print address
			case 'a':
				if (*(format + 1) == '@') {
					++format;
					putc('@', file);
				} else
					fprintf(file, "%s", addr);

				break;
			// print port
			case 'p':
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
		} else
			putc(*format, file);
	}
}

void yell_debugf(FILE *file, struct yell *self) {
	struct yell_LL_node *march;
	struct yell_peer *peer;
	int npeers;

	fprintf(file, "--- begin yell debug ---\n");

	fprintf(file, "Self is:\n");
	yell_peerf(file, "\t$n@$a:$p\n", self->name, self->sockaddr);

	pthread_mutex_lock(&self->peers_mutex);

	fprintf(file, "Connected to:\n");

	npeers = 0;

	for (march = self->peers.head; march != NULL; march = march->next) {
		peer = (struct yell_peer *)march->data;
		yell_peerf(file, "\t$n@$a:$p\n", peer->name, peer->sockaddr);

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

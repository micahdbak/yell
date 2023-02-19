#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "yell.h"

#define PORT  5000

#define MAX_CONNECTIONS  100

#define PING_MESSAGE  "What is your name?"
#define OK_MESSAGE    "OK"

void *yell_recv(void *self_ptr);

int yell_init(yellself_t *self, const char *name) {
	int sockopt;

	strcpy(self->name, name);
	self->fd = socket(AF_INET, SOCK_STREAM, 0);

	if (self->fd < 0) {
		perror("yell_init(): socket()");

		return YELL_FAILURE;
	}

	// socket option should be true (1)
	sockopt = 1;

	if (setsockopt(self->fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&sockopt, sizeof(int)) < 0) {
		perror("yell_init(): setsockopt()");

		return YELL_FAILURE;
	}

	bzero(&self->addr, sizeof(struct sockaddr_in));

	self->addr.sin_family = AF_INET;
	self->addr.sin_addr.s_addr = INADDR_ANY;

	for (self->port = PORT;; ++self->port) {
		self->addr.sin_port = htons(self->port);

		if (bind(self->fd, (struct sockaddr *)&self->addr,
		                   sizeof(struct sockaddr_in)) < 0) {
			perror("yell_init(): bind()");
			fprintf(stderr, "yell_init(): bind(): Port was %d.\n", self->port);
		} else
			break;
	}

	if (listen(self->fd, MAX_CONNECTIONS) < 0) {
		perror("yell_init(): listen()");

		return YELL_FAILURE;
	}

	self->nodes = NULL;

	self->close = 0;
	pthread_mutex_init(&self->close_mutex, NULL);

	self->events.head = NULL;
	self->events.tail = NULL;
	pthread_mutex_init(&self->events_mutex, NULL);

	pthread_create(&self->thread, NULL, yell_recv, (void *)self);

	return YELL_SUCCESS;
}

int yell_push_event(yellself_t *self, yellevent_t *event) {
	// assumes that self->events is safe to write
	
	if (self->events.head == NULL) {
		self->events.head = (eventnode_t *)malloc(sizeof(eventnode_t));

		if (self->events.head == NULL) {
			perror("yell_push_event(): malloc()");

			return YELL_FAILURE;
		}

		self->events.head->event = event;
		self->events.head->next = NULL;
		self->events.tail = self->events.head;
	} else {
		self->events.tail->next = (eventnode_t *)malloc(sizeof(eventnode_t));

		if (self->events.tail->next == NULL) {
			perror("yell_push_event(): malloc()");

			return YELL_FAILURE;
		}

		self->events.tail = self->events.tail->next;
		self->events.tail->event = event;
		self->events.tail->next = NULL;
	}

	return YELL_SUCCESS;
}

void *yell_recv(void *self_ptr) {
	yellself_t *self = (yellself_t *)self_ptr;
	int node_fd, push_r, bytes;
	socklen_t len;
	char buf[BUFFER_SIZE], name[NAME_SIZE];
	yellevent_t *event;

	len = sizeof(struct sockaddr_in);

	for (;;) {
		pthread_mutex_lock(&self->close_mutex);

		// close thread if this is false
		if (self->close)
			break;

		pthread_mutex_unlock(&self->close_mutex);

		node_fd =
		accept(self->fd, (struct sockaddr *)&self->addr, &len);

		if (node_fd < 0) {
			perror("yell_recv(): accept()");

			break;
		}

		bzero(buf, BUFFER_SIZE);
		bytes = read(node_fd, buf, BUFFER_SIZE);

		if (bytes < 0) {
			perror("yell_recv(): read()");

			break;
		}

		buf[bytes] = '\0';

		event = (yellevent_t *)malloc(sizeof(yellevent_t));

		if (event == NULL) {
			perror("yell_recv(): malloc()");

			close(node_fd);

			break;
		}

		strcpy(event->buf, buf);
		event->type = YELLEVENT_MESSAGE;

		if (strcmp(buf, PING_MESSAGE) == 0) {
			strcpy(buf, self->name);
			write(node_fd, buf, strlen(buf));

			event->type += YELLEVENT_PING;
		} else {
			strcpy(buf, OK_MESSAGE);
			write(node_fd, buf, strlen(buf));
		}

		pthread_mutex_lock(&self->events_mutex);
		push_r = yell_push_event(self, event);
		pthread_mutex_unlock(&self->events_mutex);

		if (push_r == YELL_FAILURE) {
			perror("yell_recv(): yell_push_event()");

			free(event);
			close(node_fd);
			
			break;
		}

		// close connection
		close(node_fd);
	}

	// clean up
	
	return NULL;
}

yellnode_t *yell_add(yellself_t *self, const char *addr, int port) {
	yellnode_t *node;
	int sockopt, bytes;
	struct yellnode_ll *march;
	char buf[BUFFER_SIZE];

	node = (yellnode_t *)malloc(sizeof(yellnode_t));

	if (node == NULL) {
		perror("yell_add(): malloc()");

		return NULL;
	}

	node->fd = socket(AF_INET, SOCK_STREAM, 0);

	if (node->fd == -1) {
		perror("yell_add(): socket()");

		return NULL;
	}

	// socket option should be true (1)
	sockopt = 1;

	if (setsockopt(node->fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&sockopt, sizeof(int)) < 0) {
		perror("yell_add(): setsockopt()");

		return NULL;
	}

	bzero(&node->addr, sizeof(struct sockaddr_in));

	node->addr.sin_family = AF_INET;
	node->addr.sin_addr.s_addr = inet_addr(addr);
	node->addr.sin_port = htons(port);

	if (connect(node->fd, (struct sockaddr *)&node->addr,
	                      sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "yell_add(): Could not connect to node at %s over port %d.\n", addr, port);

		return NULL;
	}

	strcpy(buf, PING_MESSAGE);

	if (write(node->fd, buf, strlen(buf)) < 0) {
		perror("yell_add(): send()");

		return NULL;
	}

	bzero(buf, BUFFER_SIZE);
	bytes = read(node->fd, buf, BUFFER_SIZE);

	if (bytes < 0) {
		perror("yell_add(): read()");

		return NULL;
	}

	buf[bytes] = '\0';
	strcpy(node->name, buf);

	// close the socket; ping was OK
	close(node->fd);

	bzero(node->event.buf, BUFFER_SIZE);
	node->event.type = YELLEVENT_MESSAGE;

	if (self->nodes == NULL) {
		self->nodes = (struct yellnode_ll *)malloc(sizeof(struct yellnode_ll));
		self->nodes->node = node;
		self->nodes->next = NULL;

		return node;
	}

	for (march = self->nodes; march->next != NULL; march = march->next)
		if (strcmp(march->node->name, node->name) == 0) {
			perror("yell_add(): Node name already exists.");

			return NULL;
		}

	if (strcmp(march->node->name, node->name) == 0) {
		perror("yell_add(): Node name already exists.");

		return NULL;
	}

	march->next = (struct yellnode_ll *)malloc(sizeof(struct yellnode_ll));
	march = march->next;
	march->node = node;
	march->next = NULL;

	return node;
}

yellnode_t *yell_node(yellself_t *self, const char *name) {
	struct yellnode_ll *march;

	for (march = self->nodes; march != NULL; march = march->next)
		if (strcmp(march->node->name, name) == 0)
			break;

	return march == NULL ? NULL : march->node;
}

void yell_debug(yellself_t *self, FILE *dst) {
	char addr[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &self->addr.sin_addr, addr, INET_ADDRSTRLEN);

	fprintf(dst, "--- libyell debug information ---\n"
			"Listening as %s on %d.\n",
			addr, ntohs(self->addr.sin_port));
}

yellevent_t *yell_private(yellself_t *self, yellnode_t *node, const char *message) {
	int bytes;

	node->fd = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(node->fd, (struct sockaddr *)&node->addr,
	                      sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "yell_private(): connect(): Could not message node.\n");

		return NULL;
	}

	if (write(node->fd, message, strlen(message)) < 0) {
		perror("yell_private(): write()");

		return NULL;
	}

	bzero(node->event.buf, BUFFER_SIZE);
	bytes = read(node->fd, node->event.buf, BUFFER_SIZE);

	if (bytes < 0) {
		perror("yell_private(): read()");

		return NULL;
	}

	node->event.buf[bytes] = '\0';
	node->event.type = YELLEVENT_MESSAGE;

	if (strcmp(node->event.buf, OK_MESSAGE) == 0)
		node->event.type += YELLEVENT_OK;

	return &node->event;
}

int yell(yellself_t *self, const char *message) {
	struct yellnode_ll *node;

	for (node = self->nodes; node != NULL; node = node->next)
		yell_private(self, node->node, message);

	return YELL_SUCCESS;
}

event_t *yell_nextevent(yellself_t *self) {
	eventnode_t *node;
	event_t *event_v;

	pthread_mutex_lock(&self->events_mutex);

	if (self->events.head == NULL) {
		pthread_mutex_unlock(&self->events_mutex);

		return NULL;
	}

	node = self->events.head;
	self->events.head = node->next;

	if (self->events.tail == node)
		self->events.tail = node->next;

	event_v = (event_t *)malloc(sizeof(event_t));
	event_v->event = node->event;
	free(node);
	
	pthread_mutex_unlock(&self->events_mutex);

	return event_v;
}

void yell_freeevent(event_t *event_v) {
	free(event_v->event);
	free(event_v);
}

void yell_exit(yellself_t *self) {

}

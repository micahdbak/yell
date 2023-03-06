#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <yell.h>

#define LINE_SIZE  512

#define PEERADDRF  "($a:$p) $n"

void chat_handler(struct yell *self, struct yell_event *event) {
	// erase line and return to the beginning
	printf("\33[2K\r");

	switch (event->type) {
	case YET_MESSAGE:
		if (event->peer == NULL)
			printf("[unknown] ");
		else
			yell_peerf(stdout, PEERADDRF ": ", event->peer->name, &event->peer->sockaddr);

		printf("%s\n", event->packet);

		break;
	default:
		printf("Received event.\n");

		break;
	}

	// print prompt
	yell_peerf(stdout, PEERADDRF " > ", self->name, &self->sockaddr);
	fflush(stdout);

	free(event);
}

int main(void) {
	FILE *yell_log;
	struct yell self;
	struct yell_event *event;
	struct yell_peer *peer;
	char name[NAME_SIZE + 1],
	     line[LINE_SIZE], *body;
	int c, i;

	printf("What's your name?\n> ");

	for (i = 0; i <= NAME_SIZE; ++i) {
		c = getc(stdin);

		if (c == '\n')
			break;

		name[i] = c;
	}

	if (c != '\n') {
		while (c != '\n')
			c = getc(stdin);
	}

	name[i] = '\0';

	if (yell_start(stderr, &self, name, chat_handler) == YELL_FAILURE) {
		printf("Failure starting yell.\n");

		return EXIT_FAILURE;
	}

	for (;;) {
		// handle yell events

		/*
		event = yell_event(&self);

		while (event != NULL) {
			switch (event->type) {
			case YET_MESSAGE:
				if (event->peer == NULL)
					printf("[unknown] ");
				else
					yell_peerf(stdout, "$n@$a:$p ", event->peer->name, &event->peer->sockaddr);

				printf("%s\n", event->packet);

				break;
			default:

				break;
			}

			yell_freeevent(event);
			event = yell_event(&self);
		}
		*/

		// print command prompt
		yell_peerf(stdout, PEERADDRF " > ", self.name, &self.sockaddr);
		fflush(stdout);

		// read a line

		c = getc(stdin);

		for (i = 0; c != '\n' && c != EOF; c = getc(stdin))
			line[i++] = c;

		line[i] = '\0';

		// get command

		for (i = 0; !isspace(line[i]) && line[i] != '\0'; ++i)
			;

		/* Two components in one array:
		 * first component is a command,
		 * second component is the arguments. */

		if (line[i] == '\0')
			// no arguments provided
			body = &line[i];
		else
			body = &line[i + 1];
		
		line[i] = '\0';

		// exit command
		if (strcmp(line, "exit") == 0)
			break;

		// debug command
		if (strcmp(line, "debug") == 0) {
			yell_debugf(stdout, &self);

			continue;
		}

		// yell command
		if (strcmp(line, "yell") == 0) {
			yell(&self, body);

			printf("\033[A\33[2K\r");
			yell_peerf(stdout, PEERADDRF ": ", self.name, &self.sockaddr);
			printf("%s\n", body);

			continue;
		}

		// connect command
		if (strcmp(line, "connect") == 0) {
			for (i = 0; body[i] != ':' && body[i] != '\0'; ++i)
				;

			if (body[i] == '\0') {
				printf("Improper peer address: \"%s\".\n"
				       "Should be HOST:PORT; e.g., 127.0.0.1:5000.\n", body);

				continue;
			}

			// split the body into two: body is host, &body[i + 1] is port
			body[i] = '\0';

			peer = yell_addpeer(&self, body, atoi(&body[i + 1]));

			if (peer == NULL) {
				printf("Couldn't create peer from address \"%s:%s\".\n"
				       "Ensure that address is in the form HOST:PORT; "
				       "e.g., 127.0.0.1:5000.\n", body, &body[i + 1]);

				continue;
			}

			yell_peerf(stdout, "Added peer $n@$a:$p.\n", peer->name, &peer->sockaddr);

			continue;
		}
	}

	yell_exit(&self);

	return EXIT_SUCCESS;
}

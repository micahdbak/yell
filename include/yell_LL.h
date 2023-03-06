/*****************
 ** linked list **
 *****************/

#ifndef YELL_LL_H
#define YELL_LL_H

#define YELL_LL_HEAD  0
#define YELL_LL_TAIL  -1

#define YELL_LL_SUCCESS  0
#define YELL_LL_FAILURE  1

struct yell_LL {
	struct yell_LL_node {
		void *data;
		struct yell_LL_node *next;
	} *head, *tail;
};

int yell_LL_insert(struct yell_LL *LL, int index, void *data);
void *yell_LL_remove(struct yell_LL *LL, int index);

#endif

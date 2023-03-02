/*****************
 ** linked list **
 *****************/

#ifndef LL_H
#define LL_H

#define LL_HEAD  0
#define LL_TAIL  -1

#define LL_SUCCESS  0
#define LL_FAILURE  1

struct LL {
	struct LL_node {
		void *data;
		struct LL_node *next;
	} *head, *tail;
};

int LL_insert(struct LL *LL, int index, void *data);
void *LL_remove(struct LL *LL, int index);

#endif

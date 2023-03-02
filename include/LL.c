#include <stdlib.h>

#include "LL.h"

int LL_insert(struct LL *LL, int index, void *data) {
	struct LL_node *node, *march;

	node = (struct LL_node *)malloc(sizeof(struct LL_node));

	// memory allocation error
	if (node == NULL)
		return LL_FAILURE;

	node->data = data;
	node->next = NULL;

	if (LL->head == NULL) {
		LL->head = node;
		LL->tail = LL->head;

		return LL_SUCCESS;
	}

	switch (index) {
	// insert at head of linked list
	case LL_HEAD:
		node->next = LL->head;
		LL->head = node;

		break;
	// insert at tail of linked list
	case LL_TAIL:
		LL->tail->next = node;
		LL->tail = node;

		break;
	// insert at certain index
	default:
		march = LL->head;

		for (int i = 0; i < index - 1; ++i) {
			march = march->next;

			if (march == NULL)
				break;
		}

		// index out of range
		if (march == NULL) {
			free(node);

			return LL_FAILURE;
		}

		// on the off chance that we are inserting at the tail: set the tail
		if (march == LL->tail)
			LL->tail = node;

		node->next = march->next;
		march->next = node;

		break;
	}

	return LL_SUCCESS;
}

void *LL_remove(struct LL *LL, int index) {
	struct LL_node *node, *march;
	void *data = NULL;

	if (LL->head == NULL)
		return NULL;

	// the index to remove is the tail, but the tail is the head, remove the head instead
	if (index == LL_TAIL && LL->tail == LL->head)
		index = LL_HEAD;

	switch (index) {
	// remove head of linked list
	case LL_HEAD:
		// remove the head
		node = LL->head;
		LL->head = node->next;

		// keep the data and free the node
		data = node->data;
		free(node);

		break;
	// remove tail of linked list
	case LL_TAIL:
		// for each node, until the next node is the tail
		for (march = LL->head; march->next != LL->tail; march = march->next)
			;

		// remove the tail
		node = LL->tail;
		LL->tail = march;
		march->next = NULL;

		// keep the data and free the node
		data = node->data;
		free(node);

		break;
	default:
		march = LL->head;

		// march along the linked list until the next node is at the specified index
		for (int i = 0; i < index - 1; ++i) {
			march = march->next;

			// index is out of range
			if (march == NULL)
				return NULL;
		}

		// index is out of range
		if (march->next == NULL)
			return NULL;

		// remove the node
		node = march->next;
		march->next = node->next;

		// check if this node is the tail
		if (node == LL->tail)
			// update tail
			LL->tail = march;

		// keep the data and free the node
		data = node->data;
		free(node);
	}

	return data;
}

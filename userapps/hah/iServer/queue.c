#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "queue.h"
#include "mem.h"

/* A really simple generic threaded blocking FIFO queue */

struct queue *queue_new(void)
{
	struct queue *queue;

	queue		= mem_malloc(sizeof(struct queue), M_ZERO);
	queue->head	= NULL;
	queue->tail	= NULL;
	queue->length = 0;

	if (pthread_mutex_init(&queue->mutex, NULL)) {
		mem_free(queue);
		return NULL;
	}

	if (pthread_cond_init(&queue->cond, NULL)) {
		mem_free(queue);
		return NULL;
	}

	return queue;
}

int queue_push(struct queue *queue, void *data)
{
	struct queue_node *node;

	if (pthread_mutex_lock(&queue->mutex))
		return -1;

	node		= mem_malloc(sizeof(struct queue_node), M_ZERO);
	node->data	= data;
	node->next	= NULL;

	if (queue->tail != NULL)
		queue->tail->next = node;

	queue->tail = node;

	if (queue->head == NULL)
		queue->head = node;

	queue->length++;
	pthread_mutex_unlock(&queue->mutex);
	pthread_cond_broadcast(&queue->cond);

	return 0;
}

void *queue_pop(struct queue *queue)
{
	struct queue_node *node;
	void *data;

	if (pthread_mutex_lock(&queue->mutex))
		return NULL;

	/* Block until there is something in the queue */
	while (queue->head == NULL)
		pthread_cond_wait(&queue->cond, &queue->mutex);

	node		= queue->head;
	queue->head	= node->next;

	if (queue->head == NULL)
		queue->tail = NULL;

	data		= node->data;
	queue->length--;
	mem_free(node);

	pthread_mutex_unlock(&queue->mutex);
	pthread_cond_broadcast(&queue->cond);

	return data;
}

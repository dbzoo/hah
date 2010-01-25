#ifndef __QUEUE_H__
#define __QUEUE_H__

struct queue_node {
	void *			data;
	struct queue_node *	next;
};

struct queue {
	struct queue_node *	head;
	struct queue_node *	tail;

	pthread_mutex_t		mutex;
	pthread_cond_t		cond;	
};


struct queue *queue_new(void);
int queue_push(struct queue *, void *);
void *queue_pop(struct queue *);

#endif

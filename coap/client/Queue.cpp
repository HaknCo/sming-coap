//  based on NodeMCU CoAP module
#include "user_config.h"
#include "queue.h"

static inline coap_queue_t *
coapMallocNode(void) {
	return (coap_queue_t *)os_zalloc(sizeof(coap_queue_t));
}

void coapFreeNode(coap_queue_t *node) {
	os_free(node);
}

int coapAddNode(coap_queue_t **queue, coap_queue_t *node) {
	coap_queue_t *p, *q;
	if ( !queue || !node )
		return 0;

	/* set queue head if empty */
	if ( !*queue ) {
		*queue = node;
		return 1;
	}

	/* replace queue head if node's time is less than head's time */
	q = *queue;
	if (node->t < q->t) {
		node->next = q;
		*queue = node;
		q->t -= node->t;		/* make q->t relative to node->t */
		return 1;
	}

	/* search for right place to insert */
	do {
		node->t -= q->t;		/* make node-> relative to q->t */
		p = q;
		q = q->next;
	} while (q && q->t <= node->t);

	/* insert new item */
	if (q) {
		q->t -= node->t;		/* make q->t relative to node->t */
	}
	node->next = q;
	p->next = node;
	return 1;
}

coap_queue_t * coapGetNodeById( coap_queue_t **queue, const coap_tid_t nodeId) {

	debugf("Get Node by id: %d", nodeId);

	coap_queue_t *p, *q, *node;

	if (!queue)
		return 0;

	if (!*queue)  // if empty
		return 0;

	node = *queue;

	/* search for right node to remove */
	while (node && node->id != nodeId) {
		node = node->next;
	}

	if (node->id == nodeId) return node;

	return 0;
}

int coapDeleteNode(coap_queue_t *node) {
	if ( !node )
		return 0;

	delete node->pdu; // free node object
	coapFreeNode(node);

	return 1;
}

void coapDeleteAllNodes(coap_queue_t *queue) {
	if ( !queue )
		return;

	coapDeleteAllNodes( queue->next );
	coapDeleteNode( queue );
}

coap_queue_t * coapAllocNode(void) {
	coap_queue_t *node;
	node = coapMallocNode();

	if ( ! node ) {
		return NULL;
	}

	os_memset(node, 0, sizeof(*node));
	return node;
}

coap_queue_t * coapPeekNext( coap_queue_t *queue ) {
	if ( !queue )
		return NULL;

	return queue;
}

coap_queue_t * coapPopNext( coap_queue_t **queue ) {		// this function is called inside timeout callback only.
	coap_queue_t *next;

	if ( !(*queue) )
		return NULL;

	next = *queue;
	*queue = (*queue)->next;
	// if (queue) {
	//   queue->t += next->t;
	// }
	next->next = NULL;
	return next;
}

int coapRemoveNode( coap_queue_t **queue, const coap_tid_t nodeId) {

	debugf("Remove Node with id: %d", nodeId);

	coap_queue_t *p, *q, *node;

	if (!queue)
		return 0;

	if (!*queue)  // if empty
		return 0;

	q = *queue;
	if (q->id == nodeId) {
		node = q;
		*queue = q->next;
		node->next = NULL;
		if (*queue) {
			(*queue)->t += node->t;
		}
		coapDeleteNode(node);
		return 1;
	}

	/* search for right node to remove */
	while (q && q->id != nodeId) {
		p = q;
		q = q->next;
	}

	/* find the node */
	if (q) {
		node = q; /* save the node */
		p->next = q->next;  /* remove the node */
		q = q->next;
		node->next = NULL;
		if (q) {   // add node->t to the node after.
			q->t += node->t;
		}
		coapDeleteNode(node);
		return 1;
	}
	return 0;
}

int coapQueueSize(coap_queue_t **queue) {

	int size = 0;
	coap_queue_t *node;

	if (!queue)
		return 0;

	if (!*queue)  // if empty
		return 0;

	while (node) {
		size++;
		node = node->next;
	}

	return size;
}

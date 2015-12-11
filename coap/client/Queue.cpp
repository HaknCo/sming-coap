//  based on NodeMCU CoAP module
#include "user_config.h"
#include "queue.h"

static inline coap_queue_t *
coap_malloc_PDU(void) {
	return (coap_queue_t *)os_zalloc(sizeof(coap_queue_t));
}

void coap_free_PDU(coap_queue_t *PDU) {
	os_free(PDU);
}

int coap_insert_PDU(coap_queue_t **queue, coap_queue_t *PDU) {
	coap_queue_t *p, *q;
	if ( !queue || !PDU )
		return 0;

	/* set queue head if empty */
	if ( !*queue ) {
		*queue = PDU;
		return 1;
	}

	/* replace queue head if PDU's time is less than head's time */
	q = *queue;
	if (PDU->t < q->t) {
		PDU->next = q;
		*queue = PDU;
		q->t -= PDU->t;		/* make q->t relative to PDU->t */
		return 1;
	}

	/* search for right place to insert */
	do {
		PDU->t -= q->t;		/* make PDU-> relative to q->t */
		p = q;
		q = q->next;
	} while (q && q->t <= PDU->t);

	/* insert new item */
	if (q) {
		q->t -= PDU->t;		/* make q->t relative to PDU->t */
	}
	PDU->next = q;
	p->next = PDU;
	return 1;
}

int coap_delete_PDU(coap_queue_t *PDU) {
	if ( !PDU )
		return 0;

	delete PDU->pdu; // free PDU object
	coap_free_PDU(PDU);

	return 1;
}

void coap_delete_all(coap_queue_t *queue) {
	if ( !queue )
		return;

	coap_delete_all( queue->next );
	coap_delete_PDU( queue );
}

coap_queue_t * coap_new_PDU(void) {
	coap_queue_t *PDU;
	PDU = coap_malloc_PDU();

	if ( ! PDU ) {
		return NULL;
	}

	os_memset(PDU, 0, sizeof(*PDU));
	return PDU;
}

coap_queue_t * coap_peek_next( coap_queue_t *queue ) {
	if ( !queue )
		return NULL;

	return queue;
}

coap_queue_t * coap_pop_next( coap_queue_t **queue ) {		// this function is called inside timeout callback only.
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

int coap_remove_PDU( coap_queue_t **queue, const coap_tid_t id) {

	debugf("Remove Node with id: %d", id);

	coap_queue_t *p, *q, *PDU;
	if ( !queue )
		return 0;
	if ( !*queue )  // if empty
		return 0;

	q = *queue;
	if (q->id == id) {
		PDU = q;
		*queue = q->next;
		PDU->next = NULL;
		if(*queue){
			(*queue)->t += PDU->t;
		}
		coap_delete_PDU(PDU);
		return 1;
	}

	/* search for right PDU to remove */
	while (q && q->id != id) {
		p = q;
		q = q->next;
	}

	/* find the PDU */
	if (q) {
		PDU = q; /* save the PDU */
		p->next = q->next;  /* remove the PDU */
		q = q->next;
		PDU->next = NULL;
		if (q)   // add PDU->t to the PDU after.
		{
			q->t += PDU->t;
		}
		coap_delete_PDU(PDU);
		return 1;
	}
	return 0;
}

#ifndef _NODE_H
#define _NODE_H 1

#include "Coap.h"
#include "Hash.h"

struct coap_queue_t;
typedef uint32 timestamp;

/*
1. queue(first)->t store when to send PDU for the next time, it's a base(absolute) time
2. queue->next->t store the delta between time and base-time.  queue->next->t = timeout + now - basetime
3. PDU->next->t store the delta between time and previous->t.  PDU->next->t = timeout + now - PDU->t - basetime
4. time to fire:   10,   15,    18,    25
		 PDU->t:   10,    5,     3,     7
*/

typedef struct coap_queue_t {
  struct coap_queue_t *next;

  timestamp t;	        /**< when to send PDU for the next time */
  unsigned char retransmit_cnt;	/**< retransmission counter, will be removed when zero */
  unsigned int timeout;		/**< the randomized timeout value */

  coap_tid_t id;			/**< unique transaction id */

  CoapPDU *pdu;				/**< the CoAP PDU to send */
} coap_queue_t;

void coap_free_PDU(coap_queue_t *PDU);

/** Adds PDU to given queue, ordered by PDU->t. */
int coap_insert_PDU(coap_queue_t **queue, coap_queue_t *PDU);

/** Destroys specified PDU. */
int coap_delete_PDU(coap_queue_t *PDU);

/** Removes all items from given queue and frees the allocated storage. */
void coap_delete_all(coap_queue_t *queue);

/** Creates a new PDU suitable for adding to the CoAP sendqueue. */
coap_queue_t *coap_new_PDU(void);

coap_queue_t *coap_pop_next( coap_queue_t **queue );

int coap_remove_PDU( coap_queue_t **queue, const coap_tid_t id);

#endif

#ifndef _NODE_H
#define _NODE_H 1

#include "CoapRequest.h"

#include "Coap.h"

#include "Hash.h"

struct coap_queue_t;
typedef uint32 timestamp;

// forward declaration
class CoapRequest;

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
  CoapRequest *req;			/** Request containing the connection */
} coap_queue_t;

void coapFreeNode(coap_queue_t *node);

/** Adds node to given queue, ordered by node->t. */
int coapAddNode(coap_queue_t **queue, coap_queue_t *node);

/** Retrieves a node from the queue by id */
coap_queue_t * coapGetNodeById( coap_queue_t **queue, const coap_tid_t nodeId);

/** Destroys specified node. */
int coapDeleteNode(coap_queue_t *node);

/** Removes all items from given queue and frees the allocated storage. */
void coapDeleteAllNodes(coap_queue_t *queue);

/** Creates a new PDU suitable for adding to the CoAP transmission queue. */
coap_queue_t *coapAllocNode(void);

coap_queue_t *coapPopNext( coap_queue_t **queue );

int coapRemoveNode( coap_queue_t **queue, const coap_tid_t id);

/** counts the number of nodes in the queue */
int coapQueueSize(coap_queue_t **queue);

#endif

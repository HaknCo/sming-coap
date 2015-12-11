#ifndef _HASH_H
#define _HASH_H 1

#include "Coap.h"

typedef unsigned char coap_key_t[4];

/* CoAP transaction id */
/*typedef unsigned short coap_tid_t; */
typedef int coap_tid_t;
#define COAP_INVALID_TID -1

void createTransactionID(const uint32 ip, const uint32 port, CoapPDU &pdu, coap_tid_t *id);

#endif

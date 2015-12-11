#include "user_config.h"
#include "Hash.h"
#include "Coap.h"

/* Caution: When changing this, update COAP_DEFAULT_WKC_HASHKEY
 * accordingly (see int coap_hash_path());
 */
void coap_hash(const unsigned char *s, unsigned int len, coap_key_t h) {
  size_t j;

  while (len--) {
    j = sizeof(coap_key_t)-1;
  
    while (j) {
      h[j] = ((h[j] << 7) | (h[j-1] >> 1)) + h[j];
      --j;
    }

    h[0] = (h[0] << 7) + h[0] + *s++;
  }
}

void createTransactionID(const uint32 ip, const uint32 port, CoapPDU &pdu, coap_tid_t *id) {
	coap_key_t h;
	os_memset(h, 0, sizeof(coap_key_t));

	/* Compare the transport address. */
	coap_hash((const unsigned char *)&(port), sizeof(port), h);
	coap_hash((const unsigned char *)&(ip), sizeof(ip), h);

	/* Compare the message id */
	uint16_t message_id = pdu.getMessageID();
	coap_hash((const unsigned char *)&(message_id), sizeof(pdu.getMessageID()), h);
	*id = ((h[0] << 8) | h[1]) ^ ((h[2] << 8) | h[3]);
}

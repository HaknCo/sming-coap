/*
 * client.h
 *
 *  Created on: 03.12.2015
 *      Author: hco
 */

#ifndef COAP_CLIENT_COAP_REQUEST_H_
#define COAP_CLIENT_COAP_REQUEST_H_

#include "Coap.h"
#include "Hash.h"
#include "Queue.h"

#include <SmingCore/SmingCore.h>

// enable COAP PROTOCOL DEBUGGING
#define COAP_DEBUG 1

class CoapRequest;

// Delegate used to pass the response to the client
typedef Delegate<void(bool successful, CoapRequest* req, CoapPDU *pdu)> CoapRxDelegate;

class CoapRequest : protected UdpConnection {

private:
	struct ip_addr remote_ip;
	int remote_port = -1;

	coap_tid_t tid = COAP_INVALID_TID;

	bool connected = false;

	void onSent(void *arg);

	static void staticDnsResponse(const char *name, struct ip_addr *ip, void *arg);

public:
	CoapRequest(struct ip_addr host, int port);
	CoapRequest(struct ip_addr host, int port, CoapRxDelegate onResponseCb);
	virtual ~CoapRequest();

	int getTransactionId();
	bool isConnected();

	virtual void close();

	coap_tid_t sendPDU(CoapPDU &pdu);

protected:

	void onReceive(pbuf *pdata, IPAddress remoteIP, uint16 remotePort);

	CoapRxDelegate onResponseCb = nullptr;
};

#endif /* COAP_CLIENT_COAP_REQUEST_H_ */

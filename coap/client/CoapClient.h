/*
 * client.h
 *
 *  Created on: 03.12.2015
 *      Author: hco
 */

#ifndef COAP_CLIENT_COAP_CLIENT_H_
#define COAP_CLIENT_COAP_CLIENT_H_

#include "Coap.h"
#include "Hash.h"
#include "Queue.h"

#include <SmingCore/SmingCore.h>

// enable COAP PROTOCOL DEBUGGING
#define COAP_DEBUG 1

// enable convenience methods
#define COAP_CONVENIENCE 1

#define SYS_TIME_MAX (0xFFFFFFFF / 1000)

#define COAP_DEFAULT_RESPONSE_TIMEOUT  2 /* response timeout in seconds */
#define COAP_DEFAULT_MAX_RETRANSMIT    4 /* max number of retransmissions */
#define COAP_TICKS_PER_SECOND 1000    // ms
#define DEFAULT_MAX_TRANSMIT_WAIT   90

// Delegate used to pass the response to the caller
typedef Delegate<void(bool successful, CoapPDU pdu)> CoapResponseDelegate;

class CoapClient {

private:
	// PDU Queue
	coap_queue_t *pQueue = NULL;

	// unique message Id.
	// TODO create a message Id per Resource
	unsigned short message_id;

	// holds callback functions of the callers
	HashMap<int, CoapResponseDelegate> callbacks;

	int  check_token(CoapPDU &pdu);

	static void staticDnsResponse(const char *name, struct ip_addr *ip, void *arg);

	/** Retransmission handling */
	Timer retransmissionTimer;
	timestamp basetime = 0;

	void timerRestart();
	void timerStop();
	void timerTick();
	void timerUpdate();
	void timerElapsed(timestamp *diff);

	CoapPDU& builPDU(CoapPDU::Type conn_type, CoapPDU::Code method, URL uri, CoapPDU::ContentFormat format, String payload);
	int request(CoapPDU::Type conn_type, CoapPDU::Code method,
			String URI,
			CoapPDU::ContentFormat format, String payload="",
			CoapResponseDelegate callback=nullptr);

public:
	CoapClient();
	virtual ~CoapClient();

	// Methods
	int get(CoapPDU::Type connType, String uri, CoapResponseDelegate responseCallback);
	int post(CoapPDU::Type connType, String uri, CoapPDU::ContentFormat format, String payload, CoapResponseDelegate responseCallback);
	int put(CoapPDU::Type conn_type, String uri, CoapPDU::ContentFormat format, String payload, CoapResponseDelegate responseCallback);
	int del(CoapPDU::Type conn_type, String URI, CoapResponseDelegate responseCallback);
	int observe(CoapPDU::Type conn_type, String URI, CoapResponseDelegate responseCallback);

#ifdef COAP_CONVENIENCE
	// convenience methods
	int get(String uri, CoapResponseDelegate responseCallback);

	int post(String uri, CoapResponseDelegate responseCallback);
	int post(CoapPDU::Type connType, String uri, CoapResponseDelegate responseCallback);
	int post(CoapPDU::Type connType, String uri, String payload, CoapResponseDelegate responseCallback);

	int put(CoapPDU::Type conn_type, String uri, CoapPDU::ContentFormat format, String payload);
#endif

protected:

	// process request
	coap_tid_t doRequest(CoapRequest &req, CoapPDU &pdu);

	// CoapRxDelegate implementation that is called when a request receives a PDU
	// Callback used tie requests and client together.
	void onResponse(bool successful, CoapRequest *request, CoapPDU *pdu);
};

#endif /* COAP_CLIENT_COAP_CLIENT_H_ */

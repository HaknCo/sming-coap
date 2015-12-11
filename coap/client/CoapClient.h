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

#define SYS_TIME_MAX (0xFFFFFFFF / 1000)

#define COAP_DEFAULT_RESPONSE_TIMEOUT  2 /* response timeout in seconds */
#define COAP_DEFAULT_MAX_RETRANSMIT    4 /* max number of retransmissions */
#define COAP_TICKS_PER_SECOND 1000    // ms
#define DEFAULT_MAX_TRANSMIT_WAIT   90


class CoapClient;

// Delegate constructor usage: (&YourClass::method, this)
typedef Delegate<void(CoapClient& client, String message)> CoapResponseDelegate;


class CoapClient : protected UdpConnection {

private:
	unsigned short message_id;

	// PDU Queue
	coap_queue_t *pQueue = NULL;

	void onSent(void *arg);
	void coap_response_handler(void *arg, char *pdata, unsigned short len);
	int  check_token(CoapPDU &pdu);

	static void staticDnsResponse(const char *name, struct ip_addr *ip, void *arg);

	/** Retranmission handling */
	Timer retransmissionTimer;
	timestamp basetime = 0;

	void timerRestart();
	void timerStop();
	void timerTick();
	void timerUpdate();
	void timerElapsed(timestamp *diff);

	CoapPDU& builPDU(CoapPDU::Type conn_type, CoapPDU::Code method, URL uri, CoapPDU::ContentFormat format, String payload);
	int request(CoapPDU::Type conn_type, CoapPDU::Code method, String URI, CoapPDU::ContentFormat format, String payload="");

public:
	CoapClient();
	CoapClient(CoapResponseDelegate onResponseCb);
	virtual ~CoapClient();

	int get(CoapPDU::Type conn_type, String URI);
	int post(CoapPDU::Type conn_type, String uri, CoapPDU::ContentFormat format, String payload);
	int put(CoapPDU::Type conn_type, String uri, CoapPDU::ContentFormat format, String payload);
	int del(CoapPDU::Type conn_type, String URI);

protected:

	coap_tid_t sendPDU(CoapPDU &pdu);
	coap_tid_t sendPDUConfirmed(CoapPDU &pdu);

	void onReceive(pbuf *pdata, IPAddress remoteIP, uint16 remotePort);

	CoapResponseDelegate onResponseCb = nullptr;
};

#endif /* COAP_CLIENT_COAP_CLIENT_H_ */

/*
Copyright (c)  2015  Hakan Coskun, http://www.blueonshop.de
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Coap.h"
#include "CoapRequest.h"
#include "CoapClient.h"
#include "Queue.h"


// SMING includes
#include <Wiring/WMath.h>
#include <SmingCore/SmingCore.h>


CoapClient::CoapClient() {

	debugf("CoapClient: ctor is called.\n");

	message_id = (unsigned short) rand();   // calculate only once

	retransmissionTimer.initializeMs(1000, TimerDelegate(&CoapClient::timerTick, this));
}

CoapClient::~CoapClient() {
	debugf("CoapClient: destructor is called.\n");
}

// Callback called by the CoapResponse class
void CoapClient::onResponse(bool successful, CoapRequest *request, CoapPDU *pdu) {
	coap_tid_t tid = request->getTransactionId();

	debugf("CoapClient: onResponse (%d, %d).\n", successful, request->getTransactionId());

	if (! successful) {
		ERROR("CoapClient: malformed CoAP packet");
		return;
	}

	/* check if this is a response to our original request */
	//	if (!check_token(&pkt)) {
	//		debugf("wrong token\n");
	//		/* drop if this was just some message, or send RST in case of notification */
	//		if (recvPDU->getType() == CoapPDU::COAP_CONFIRMABLE || recvPDU->getType() == CoapPDU::COAP_NON_CONFIRMABLE) {
	//			// TODO send connection reset
	//			// coap_send_rst(pkt);  // send RST response
	//			// or, just ignore it.
	//		}
	//		goto end;
	//	}

	if (pdu->getType() == CoapPDU::COAP_RESET) {
		debugf("got RST\n");
		// goto end;
	}

	coap_queue_t *node = coapGetNodeById(&pQueue, tid);

	if (node) {
		debugf("CoapClient: Found request for transaction id\n");

		if (node->pdu->getType() == CoapPDU::COAP_NON_CONFIRMABLE) {
			// we got a response, we are done
			debugf("CoapClient: Got response for NONCONFIRMABLE MESSAGE\n");
		} else if (node->pdu->getType() == CoapPDU::COAP_CONFIRMABLE) {
			debugf("CoapClient: Got response for CONFIRMABLE MESSAGE\n");

			if (pdu->getType() == CoapPDU::COAP_ACKNOWLEDGEMENT) {
				debugf("CoapClient: got ACK\n");
			} else {
				debugf("CoapClient: got Message but waiting for ACK\n");
			}
		}
		/* transaction done, remove the PDU from queue */

		// stop timer
		timerStop();

		if (callbacks.contains(tid)) {
				callbacks[tid](true, *pdu);
				callbacks.remove(tid);
				// remove the PDU
				debugf("CoapClient: remove node for transaction %d.\n", tid);
				// close request, TODO what about observe
				request->close();
				coapRemoveNode(&pQueue, tid);
		} else {
			debugf("got unknown message Id\n");
			// TODO RESET
		}

		// calculate time elapsed
		timerUpdate();
		timerRestart();

		if (COAP_RESPONSE_CLASS(pdu->getCode()) == 2) {
			/* There is no block option set, just read the data and we are done. */
			debugf("%d.%02d\t", (pdu->getCode() >> 5), pdu->getCode() & 0x1F);
			pdu->printHex();
		} else if (COAP_RESPONSE_CLASS(pdu->getCode()) >= 4) {
			debugf("%d.%02d\t", (pdu->getCode() >> 5), pdu->getCode() & 0x1F);
			pdu->printHex();
		}
	} else {
		debugf("CoapClient: Could not found request for transaction id\n");
		// TODO send reset
	}
}

int CoapClient::request(CoapPDU::Type conn_type, CoapPDU::Code method,
		String uri, CoapPDU::ContentFormat format, String payload /* "" */, CoapResponseDelegate callback /* nullptr */) {

	debugf("CoapClient: REQUEST (%d) (%s)\n", method, uri.c_str());
	debugf("CoapClient: CONN_TYPE (%d)\n", conn_type);
	debugf("CoapClient: FORMAT (%d)\n", format);

	if ( conn_type != CoapPDU::COAP_CONFIRMABLE && conn_type != CoapPDU::COAP_NON_CONFIRMABLE ) {
		debugf("CoapClient: wrong connection type, setting to CONFIRMABLE.\n");
		conn_type = CoapPDU::COAP_CONFIRMABLE ; // default to CONFIRMABLE
	}

	URL resource(uri);

	if (resource.Protocol.equals(DEFAULT_URL_PROTOCOL)) {
		resource.Protocol = COAP_DEFAULT_SCHEME;
	}

	if (resource.Port == 80) {
		resource.Port = COAP_DEFAULT_PORT;
	}

	if (resource.Host.length() == 0)
		return debugf("CoapClient: wrong URI format.");

	struct ip_addr resolvedIP;
	int result = dns_gethostbyname(resource.Host.c_str(), &resolvedIP, staticDnsResponse, (void*) this);

	if (result == ERR_OK)  {
		// Documentation says this will be the result if the string is already
		// an ip address in dotted decimal form or if the host is found in dns cache.
		// however I'm not sure if the dns cache is working since this never seems to
		// be called for a host lookup other than an ip address.
		// Doesn't really matter since the loockup will be fast anyways, the host
		// is most likely found in the dns cache of the next PDU the query is sent to.
		CoapPDU& pdu = builPDU(conn_type, method, resource, format, payload);

#ifdef COAP_DEBUG
		pdu.printHuman();
#endif
		// connect to server
		CoapRequest *req = new CoapRequest(resolvedIP, resource.Port, CoapRxDelegate(&CoapClient::onResponse, this));

		coap_tid_t tid = doRequest(*req, pdu);

		callbacks[tid] = callback;
	} else if (result == ERR_INPROGRESS)  {
		// TODO store request and start retry timer
		debugf("DNS IP lookup in progress.");
	} else {
		debugf("DNS lookup error occurred.");
	}

	return 0;
}

coap_tid_t CoapClient::doRequest(CoapRequest &req, CoapPDU &pdu) {

	coap_queue_t *queueNode;
	coap_tid_t tid = COAP_INVALID_TID;
	timestamp diff;
	uint32 r;

	queueNode = coapAllocNode();
	if (!queueNode) {
		debugf("CoapClient: sendPDUConfirmed, insufficient memory\n");
		return COAP_INVALID_TID;
	}

	if (pdu.getType() == CoapPDU::COAP_CONFIRMABLE) {
		debugf("CoapClient: Send PDU confirmed\n");

		/* Set timer for pdu retransmission. If this is the first element in
		 * the retransmission queue, the base time is set to the current
		 * time and the retransmission time is PDU->timeout. If there is
		 * already an entry in the transmission queue, we must check if this PDU is
		 * to be retransmitted earlier. Therefore, PDU->timeout is first
		 * normalized to the timeout and then inserted into the queue with
		 * an adjusted relative time.
		 */

		queueNode->retransmit_cnt = 0;

		r = rand();

		/* add randomized RESPONSE_TIMEOUT to determine retransmission timeout */
		queueNode->timeout = COAP_DEFAULT_RESPONSE_TIMEOUT * COAP_TICKS_PER_SECOND +
				(COAP_DEFAULT_RESPONSE_TIMEOUT >> 1) *
				((COAP_TICKS_PER_SECOND * (r & 0xFF)) >> 8);

	} else {
		debugf("CoapClient: Send PDU non-confirmed\n");
		// Queue non-confirmable PDUs in order handle failed transmissions
		queueNode->timeout = COAP_DEFAULT_RESPONSE_TIMEOUT * COAP_TICKS_PER_SECOND;
	}

	queueNode->req = &req;
	queueNode->pdu = &pdu;

#ifdef COAP_DEBUG
	debugf("CoapClient: Sending ...\n");
	pdu.printHex();
#endif

	// send packet
    queueNode->id = req.sendPDU(pdu);

	timerStop();
	timerUpdate();
	queueNode->t = queueNode->timeout;
	coapAddNode(&pQueue, queueNode);
	timerRestart();

	return queueNode->id;
}

CoapPDU& CoapClient::builPDU(CoapPDU::Type conn_type, CoapPDU::Code method, URL resource,
		CoapPDU::ContentFormat format, String payload /* "" */) {
	// construct CoAP packet
	CoapPDU *pdu = new CoapPDU();
	pdu->setVersion(1);
	pdu->setType(conn_type);
	pdu->setCode(method);
	pdu->setToken((uint8_t*)"\3\2\1\1", 4);
	pdu->setMessageID(message_id);
	pdu->setURI((char*) resource.getPathWithQuery().c_str(), resource.getPathWithQuery().length());
	pdu->setContentFormat(format);

	if (payload.length() > 0) {
		debugf("CoapClient: PDU has payload\n");
		debugf("%s\n", payload.c_str());
		pdu->setPayload((unsigned char*) payload.c_str(), payload.length());
	}

	message_id++;

	return *pdu;
};

void CoapClient::staticDnsResponse(const char *name, struct ip_addr *ip, void *arg) {
	// DNS has been resolved

	CoapClient *self = (CoapClient*) arg;

	// TODO get last request and repeat
	if (ip != NULL) {
		// We do a new request since the last one was never done.
		//self->internalRequestTime(*ip);
	}
}

/** Retransmission handling */

void CoapClient::timerRestart() {
	if (pQueue) {
		// if there is a PDU in the queue, set timeout to its ->t.
		retransmissionTimer.setIntervalMs(pQueue->t);
		retransmissionTimer.startOnce();
	}
}

void CoapClient::timerStop() {
	retransmissionTimer.stop();
}

void CoapClient::timerTick() {
	coap_queue_t *node = coapPopNext(&pQueue);

	// non-confirmable PDU could not be sent, inform callback
	if (node->pdu->getType() == CoapPDU::COAP_NON_CONFIRMABLE) {
		debugf("CoapClient: non-confirmable PDU with transaction id %d could not be sent\n", node->id);
		// call callback
		if (callbacks.contains(node->id)) {
			callbacks[node->id](false, *node->pdu);
		}
		// remove PDU
		//   coapRemoveNode(&pQueue, node->id);
		coapDeleteNode(node);
	} else {
		/* re-initialize timeout when maximum number of retransmissions are not reached yet */
		if (node->retransmit_cnt < COAP_DEFAULT_MAX_RETRANSMIT) {
			node->retransmit_cnt++;
			node->t = node->timeout << node->retransmit_cnt;

			debugf("CoapClient: retransmission #%d of transaction %d\n", node->retransmit_cnt, node->id);

			// re-transmit PDU
			node->id = node->req->sendPDU(*node->pdu);
			if (COAP_INVALID_TID == node->id) {
				debugf("CoapClient: error sending PDU\n");
				coapDeleteNode(node);
			} else {
				coapAddNode(&pQueue, node);
			}
		} else {
			debugf("CoapClient: maximum retransmissions for transaction %d reached.\n", node->id);
			/* Maximum number of retransmissions reached. Inform the callback and remove PDU */
			if (callbacks.contains(node->id)) {
				callbacks[node->id](false, *node->pdu);
			}
			coapDeleteNode(node);
		}
	}

	timerRestart();
}

void CoapClient::timerUpdate() {
	timestamp diff = 0;
	coap_queue_t *first = pQueue;
	timerElapsed(&diff); // update: basetime = now, diff = now - oldbase, means time elapsed

	if (first) {
		// diff ms time is elapsed, re-calculate the first PDU->t
		if (first->t >= diff){
			first->t -= diff;
		} else {
			first->t = 0;  // when timer enabled, time out almost immediately
		}
	}
}

void CoapClient::timerElapsed(timestamp *diff) {
	timestamp now = system_get_time() / 1000;   // coap_tick_t is in ms. also sys_timer

	if (now >= basetime) {
		*diff = now-basetime;
	} else {
		*diff = now + SYS_TIME_MAX -basetime;
	}
	basetime = now;
}

// Methods
int CoapClient::get(String uri, CoapResponseDelegate responseCallback) {
	return get(CoapPDU::COAP_NON_CONFIRMABLE, uri, responseCallback);
}

int CoapClient::get(CoapPDU::Type connType, String uri, CoapResponseDelegate responseCallback) {
	debugf("CoapClient: GET (%s)", uri.c_str());
	return this->request(connType, CoapPDU::COAP_GET, uri, CoapPDU::ContentFormat::COAP_CONTENT_FORMAT_TEXT_PLAIN, "", responseCallback);
}

int CoapClient::post(String uri, CoapResponseDelegate responseCallback) {
	return post(CoapPDU::COAP_NON_CONFIRMABLE, uri, responseCallback);
}

int CoapClient::post(CoapPDU::Type connType, String uri, CoapResponseDelegate responseCallback) {
	return post(connType, uri, "", responseCallback);
}

int CoapClient::post(CoapPDU::Type connType, String uri, String payload, CoapResponseDelegate responseCallback) {
	return post(connType, uri, CoapPDU::ContentFormat::COAP_CONTENT_FORMAT_APP_JSON, payload, responseCallback);
}

int CoapClient::post(CoapPDU::Type connType, String uri, CoapPDU::ContentFormat format, String payload, CoapResponseDelegate responseCallback) {
	debugf("CoapClient: POST (%s)", uri.c_str());
	return this->request(connType, CoapPDU::COAP_POST, uri, format, payload, responseCallback);
}

int CoapClient::put(CoapPDU::Type conn_type, String uri, CoapPDU::ContentFormat format, String payload, CoapResponseDelegate responseCallback) {
	debugf("CoapClient: PUT (%s)", uri.c_str());
	return this->request(conn_type, CoapPDU::COAP_PUT, uri, format, payload, responseCallback);
}

int CoapClient::del(CoapPDU::Type conn_type, String uri, CoapResponseDelegate responseCallback) {
	debugf("CoapClient: DELETE (%s)", uri.c_str());
	return this->request(conn_type, CoapPDU::COAP_DELETE, uri, CoapPDU::ContentFormat::COAP_CONTENT_FORMAT_TEXT_PLAIN, "", responseCallback);
}

int CoapClient::observe(CoapPDU::Type conn_type, String uri, CoapResponseDelegate responseCallback) {
	debugf("CoapClient: OBSERVE (%s)", uri.c_str());
	return this->request(conn_type, CoapPDU::COAP_GET, uri, CoapPDU::ContentFormat::COAP_CONTENT_FORMAT_TEXT_PLAIN, "", responseCallback);
}


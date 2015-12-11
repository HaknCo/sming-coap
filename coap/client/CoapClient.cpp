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
#include "CoapClient.h"
#include "Queue.h"


// SMING includes
#include <Wiring/WMath.h>
#include <SmingCore/SmingCore.h>

void CoapClient::onSent(void *arg) {
	debugf("CoapClient: onSent is called.\n");
}

CoapClient::CoapClient() : CoapClient(nullptr) {
}

CoapClient::CoapClient(CoapResponseDelegate onResponseCb) {

	debugf("CoapClient: ctor is called.\n");

	message_id = (unsigned short) rand();   // calculate only once

	this->onResponseCb = onResponseCb;

	retransmissionTimer.initializeMs(1000, TimerDelegate(&CoapClient::timerTick, this));
}

CoapClient::~CoapClient() {
	debugf("CoapClient: destructor is called.\n");
}

void CoapClient::onReceive(pbuf *pdata, IPAddress remoteIP, uint16 remotePort) {

	debugf("CoapClient: onReceive.\n");
	coap_tid_t id = COAP_INVALID_TID;

	CoapPDU *recvPDU = new CoapPDU((uint8_t*)pdata->payload, pdata->tot_len);
	if (recvPDU->validate()!=1) {
		ERROR("Malformed CoAP packet");
		return;
	}

#ifdef COAP_DEBUG
	INFO("Valid CoAP PDU received");
	recvPDU->printHuman();
#endif
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

	if (recvPDU->getType() == CoapPDU::COAP_RESET) {
		debugf("got RST\n");
		goto end;
	}

	createTransactionID((uint32) remoteIP, (uint32) remotePort, *recvPDU, &id);

	/* transaction done, remove the PDU from queue */

	// stop timer
	timerStop();

	// remove the PDU
	coap_remove_PDU(&pQueue, id);

	// calculate time elapsed
	timerUpdate();
	timerRestart();

	if (COAP_RESPONSE_CLASS(recvPDU->getCode()) == 2) {
		/* There is no block option set, just read the data and we are done. */
		debugf("%d.%02d\t", (recvPDU->getCode() >> 5), recvPDU->getCode() & 0x1F);
		recvPDU->printHex();
	} else if (COAP_RESPONSE_CLASS(recvPDU->getCode()) >= 4) {
		debugf("%d.%02d\t", (recvPDU->getCode() >> 5), recvPDU->getCode() & 0x1F);
		recvPDU->printHex();
	}

	end:
	if (!pQueue) { // if there is no PDU pending in the queue, disconnect from host.
		//if (pesp_conn->proto.udp->remote_port || pesp_conn->proto.udp->local_port)
			//espconn_delete(pesp_conn);
		//TODO disconnect
	}
}

int CoapClient::request(CoapPDU::Type conn_type, CoapPDU::Code method,
		String uri, CoapPDU::ContentFormat format, String payload /* "" */) {
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

	debugf("UDP port is set: %d.\n", resource.Port);

	struct ip_addr resolvedIP;
	int result = dns_gethostbyname(resource.Host.c_str(), &resolvedIP, staticDnsResponse, (void*) this);

	if (result == ERR_OK)  {
		// Documentation says this will be the result if the string is already
		// an ip address in dotted decimal form or if the host is found in dns cache.
		// however I'm not sure if the dns cache is working since this never seems to
		// be called for a host lookup other than an ip address.
		// Doesn't really matter since the loockup will be fast anyways, the host
		// is most likely found in the dns cache of the next PDU the query is sent to.
		debugf("Host len(%d):", resource.Host.length());
		debugf("%s", resource.Host.c_str());
		debugf("\n");

		debugf("UDP ip is set: ");
		debugf(IPSTR, IP2STR(&resolvedIP.addr));
		debugf("\n");

		CoapPDU& pdu = builPDU(conn_type, method, resource, format, payload);

#ifdef COAP_DEBUG
		pdu.printHuman();
#endif

		// connect to server
		this->connect(resolvedIP, resource.Port);
		coap_tid_t tid = COAP_INVALID_TID;

		if (pdu.getType() == CoapPDU::COAP_CONFIRMABLE ){
			tid = sendPDUConfirmed(pdu);
		} else {
			tid = sendPDU(pdu);
			if (tid == COAP_INVALID_TID) {
				coap_remove_PDU(&pQueue, tid);
			}
		}

	} else if (result == ERR_INPROGRESS)  {
		// TODO store request and start retry timer
		debugf("DNS IP lookup in progress.");
	} else {
		debugf("DNS lookup error occurred.");
	}

	return 0;
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
		debugf("PDU has payload\n");
		debugf("%s\n", payload.c_str());
		pdu->setPayload((unsigned char*) payload.c_str(), payload.length());
	}

	message_id++;

	return *pdu;
};

coap_tid_t CoapClient::sendPDU(CoapPDU &pdu) {
	coap_tid_t id = COAP_INVALID_TID;

	//if (pdu == nullptr) return id;

	createTransactionID((uint32) this->udp->remote_ip.addr, (uint32) this->udp->remote_port, pdu, &id);

	debugf("CoapClient: Send PDU with transaction id %d\n", id);

#ifdef COAP_DEBUG
	debugf("CoapClient: Sending ...\n");
	pdu.printHex();
#endif

	// send packet
	send((char*) pdu.getPDUPointer(), pdu.getPDULength());

	INFO("Packet sent");

	return id;
}

coap_tid_t CoapClient::sendPDUConfirmed(CoapPDU &pdu) {
	  coap_queue_t *PDU;
	  timestamp diff;
	  uint32 r;

	  debugf("CoapClient: Send PDU confirmed\n");

	  PDU = coap_new_PDU();
	  if (!PDU) {
	    debugf("CoapClient: sendPDUConfirmed, insufficient memory\n");
	    return COAP_INVALID_TID;
	  }

	  PDU->retransmit_cnt = 0;
	  PDU->id = sendPDU(pdu);
	  if (COAP_INVALID_TID == PDU->id) {
	    debugf("CoapClient: sendPDUConfirmed, error sending PDU\n");
	    coap_free_PDU(PDU);
	    return COAP_INVALID_TID;
	  }

	  r = rand();

	  /* add randomized RESPONSE_TIMEOUT to determine retransmission timeout */
	  PDU->timeout = COAP_DEFAULT_RESPONSE_TIMEOUT * COAP_TICKS_PER_SECOND +
	    (COAP_DEFAULT_RESPONSE_TIMEOUT >> 1) *
	    ((COAP_TICKS_PER_SECOND * (r & 0xFF)) >> 8);

	  PDU->pdu = &pdu;

	  /* Set timer for pdu retransmission. If this is the first element in
	   * the retransmission queue, the base time is set to the current
	   * time and the retransmission time is PDU->timeout. If there is
	   * already an entry in the sendqueue, we must check if this PDU is
	   * to be retransmitted earlier. Therefore, PDU->timeout is first
	   * normalized to the timeout and then inserted into the queue with
	   * an adjusted relative time.
	   */
	  timerStop();
	  timerUpdate();
	  PDU->t = PDU->timeout;
	  coap_insert_PDU(&pQueue, PDU);
	  timerRestart();
	  return PDU->id;
}

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
	if (pQueue) { // if there is a PDU in the queue, set timeout to its ->t.
		retransmissionTimer.setIntervalMs(pQueue->t);
		retransmissionTimer.startOnce();
	}
}

void CoapClient::timerStop() {
	retransmissionTimer.stop();
}

void CoapClient::timerTick() {

  coap_queue_t *PDU = coap_pop_next( &pQueue );
  /* re-initialize timeout when maximum number of retransmissions are not reached yet */
  if (PDU->retransmit_cnt < COAP_DEFAULT_MAX_RETRANSMIT) {
    PDU->retransmit_cnt++;
    PDU->t = PDU->timeout << PDU->retransmit_cnt;

    debugf("** retransmission #%d of transaction %d\n", PDU->retransmit_cnt, PDU->pdu->getMessageID());
    PDU->id = sendPDU(*(PDU->pdu));
    if (COAP_INVALID_TID == PDU->id) {
      debugf("retransmission: error sending PDU\n");
      coap_delete_PDU(PDU);
    } else {
      coap_insert_PDU(&pQueue, PDU);
    }
  } else {
    /* And finally delete the PDU */
    coap_delete_PDU( PDU );
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

  if(now >= basetime){
    *diff = now-basetime;
  } else {
    *diff = now + SYS_TIME_MAX -basetime;
  }
  basetime = now;
}

// REST Operations
int CoapClient::get(CoapPDU::Type conn_type, String uri) {
	debugf("CoapClient: GET (%s)", uri.c_str());
	return this->request(conn_type, CoapPDU::COAP_GET, uri, CoapPDU::ContentFormat::COAP_CONTENT_FORMAT_TEXT_PLAIN);
}

int CoapClient::post(CoapPDU::Type conn_type, String uri, CoapPDU::ContentFormat format, String payload) {
	debugf("CoapClient: POST (%s)", uri.c_str());
	return this->request(conn_type, CoapPDU::COAP_POST, uri, format, payload);
}

int CoapClient::put(CoapPDU::Type conn_type, String uri, CoapPDU::ContentFormat format, String payload) {
	debugf("CoapClient: PUT (%s)", uri.c_str());
	return this->request(conn_type, CoapPDU::COAP_PUT, uri, format, payload);
}

int CoapClient::del(CoapPDU::Type conn_type, String uri) {
	debugf("CoapClient: DELETE (%s)", uri.c_str());
	return this->request(conn_type, CoapPDU::COAP_DELETE, uri, CoapPDU::ContentFormat::COAP_CONTENT_FORMAT_TEXT_PLAIN);
}

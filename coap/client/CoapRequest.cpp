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
#include "Queue.h"


// SMING includes
#include <Wiring/WMath.h>
#include <SmingCore/SmingCore.h>

void CoapRequest::onSent(void *arg) {
	debugf("CoapRequest: onSent is called.\n");
}

CoapRequest::CoapRequest(struct ip_addr remote_ip, int remote_port) : CoapRequest(remote_ip, remote_port, nullptr) {
}

CoapRequest::CoapRequest(struct ip_addr remote_ip, int remote_port, CoapRxDelegate onResponseCb) {

	debugf("CoapRequest: ctor is called.\n");

	this->remote_ip = remote_ip;
	this->remote_port = remote_port;

	debugf("CoapRequest: Remote IP: ");
	debugf(IPSTR, IP2STR(&this->remote_ip.addr));

	debugf("CoapRequest: Remote Port: %d.\n", this->remote_port);

	this->onResponseCb = onResponseCb;
}

CoapRequest::~CoapRequest() {
	debugf("CoapRequest: destructor is called.\n");
}

int CoapRequest::getTransactionId() {
	return this->tid;
}

/**
 * called by UDPConnection when a packet arrives
 */
void CoapRequest::onReceive(pbuf *pdata, IPAddress remoteIP, uint16 remotePort) {

	debugf("CoapRequest: onReceive.\n");
	coap_tid_t id = COAP_INVALID_TID;

	CoapPDU *recvPDU = new CoapPDU((uint8*)pdata->payload, pdata->tot_len);
	if (recvPDU->validate()!=1) {
		ERROR("malformed CoAP packet");
		// inform client about wrong PDU
		onResponseCb(false, this, recvPDU);
		return;
	}

#ifdef COAP_DEBUG
	INFO("Valid CoAP PDU received");
	recvPDU->printHuman();
#endif

	// pass PDU to client
	onResponseCb(true, this, recvPDU);
}

coap_tid_t CoapRequest::sendPDU(CoapPDU &pdu) {

	if (!connected) {
		debugf("CoapRequest: connect to IP: ");
		debugf(IPSTR, IP2STR(&(this->remote_ip.addr)));
		debugf("Port: %d\n", this->remote_port);

		if (connect(this->remote_ip, this->remote_port)) {
			connected = true;
			createTransactionID((uint32) this->udp->remote_ip.addr, (uint32) this->udp->remote_port, pdu, &tid);
			debugf("CoapRequest: request tid=%d, connected to ", tid);
					debugf(IPSTR, IP2STR(&(this->udp->remote_ip.addr)));
					debugf("Port: %d\n", this->udp->remote_port);
		} else {
			createTransactionID((uint32) this->remote_ip.addr, (uint32) this->remote_port, pdu, &tid);
		}
	} else {
		createTransactionID((uint32) this->udp->remote_ip.addr, (uint32) this->udp->remote_port, pdu, &tid);
	}

	debugf("CoapRequest: free memory: %d\n", system_get_free_heap_size());
	debugf("CoapRequest: Send PDU with transaction id %d\n", tid);
	debugf("CoapRequest: PDU size %d\n", pdu.getPDULength());

	// sendTo(this->remote_ip, this->remote_port, (char*) pdu.getPDUPointer(), pdu.getPDULength());

	// send packet
	send((char*) pdu.getPDUPointer(), pdu.getPDULength());

	return tid;
}

void CoapRequest::close() {
	debugf("CoapRequest: close request\n");
	UdpConnection::close();
}

void CoapRequest::staticDnsResponse(const char *name, struct ip_addr *ip, void *arg) {
	// DNS has been resolved

	CoapRequest *self = (CoapRequest*) arg;

	// TODO get last request and repeat
	if (ip != NULL) {
		// We do a new request since the last one was never done.
		//self->internalRequestTime(*ip);
	}
}

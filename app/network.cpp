/*
 * network.cpp
 *
 *  Created on: 07.09.2015
 *      Author: Hakan Coskun <hakan.coskun@bluenshop.de>
 */

#include <user_config.h>
#include "CoapClient.h"
#include <wifi.h>
#include <SmingCore/SmingCore.h>

extern "C" {
  #include <coap_server.h>
}

Timer pushTimer;

CoapClient coapClient;

void pushTime() {
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();
	json.set("timestamp", (int) SystemClock.now());

	String msg;
	json.printTo(msg);

	coapClient.post(CoapPDU::COAP_CONFIRMABLE, "coap://192.168.1.1/json", CoapPDU::ContentFormat::COAP_CONTENT_FORMAT_APP_JSON, msg);
	//coapClient.put(CoapPDU::COAP_CONFIRMABLE, "coap://192.168.1.1/json", CoapPDU::ContentFormat::COAP_CONTENT_FORMAT_APP_JSON, msg);
	//coapClient.get(CoapPDU::COAP_NON_CONFIRMABLE, "coap://192.168.1.1/esp8266");
	//coapClient.del(CoapPDU::COAP_NON_CONFIRMABLE, "coap://192.168.1.1/esp8266");
}

// Will be called when system initialization was completed
void startServers() {
	debugf("System ready, start servers\n");
	start_coap_server();

       	pushTimer.initializeMs(5000, pushTime);
        pushTimer.start();
}

void networkSetup() {
    wifiSetup();
    // Run WEB server on system ready
    System.onReady(startServers);
}

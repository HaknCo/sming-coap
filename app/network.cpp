/*
 * network.cpp
 *
 *  Created on: 07.09.2015
 *      Author: Hakan Coskun <hakan.coskun@bluenshop.de>
 */

#include <wifi.h>
#include <SmingCore/SmingCore.h>

extern "C" {
  #include <coap_server.h>
}

// Will be called when system initialization was completed
void startServers() {
	debugf("System ready, start servers\n");
	start_coap_server();
}

void networkSetup() {
    wifiSetup();
    // Run WEB server on system ready
    System.onReady(startServers);
}

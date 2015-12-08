/*
 * wifi.h
 *
 *  Created on: 15.10.2015
 *      Author: Hakan Coskun <hakan.coskun@blueonshop.de>
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <SmingCore/SmingCore.h>

BssList wifiScan();

// Will be called when WiFi station was connected to AP
void connectOk();
void connectFail();

void configureAP();
void configureStation();

void wifiSetup();

#endif /* WIFI_H_ */

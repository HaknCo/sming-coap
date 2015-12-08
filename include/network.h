/*
 * network.h
 *
 *  Created on: 07.09.2015
 *      Author: Hakan Coskun <hakan.coskun@blueonshop.de>
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include <SmingCore/SmingCore.h>

// NETWORK CONFIG
bool      dhcp = true;
IPAddress ip;
IPAddress netmask;
IPAddress gateway;

// online status
bool   online = false;

// Will be called when system initialization was completed
void startServers();

void networkSetup();

#endif /* NETWORK_H_ */

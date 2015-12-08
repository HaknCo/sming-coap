/*
 * wifi_config.h
 *
 *  Created on: 15.10.2015
 *      Author: Hakan Coskun <hakan.coskun@blueonshop.de>
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <SmingCore/SmingCore.h>

#define maxMissedConnections 3

// WIFI
bool   wifiEnabled = true;

// AP
bool   wifiAPEnabled = true;
String wifiAPSSID = "ESP";
String wifiAPPassword = "";
int    wifiAPChannel = 6;
bool   wifiAPHiddden = false;

// Station
bool   wifiStationEnabled = true;
String wifiStationSSID = "";
String wifiStationPassword = "";

// NETWORK CONFIG
extern bool dhcp;
extern IPAddress ip;
extern IPAddress netmask;
extern IPAddress gateway;

// online status
extern bool online;

#endif /* WIFI_H_ */

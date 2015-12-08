/*
 * wifi.cpp
 *
 *  Created on: 15.10.2015
 *      Author: Hakan Coskun <hakan.coskun@blueonshop.de>
 */

#include <SmingCore/SmingCore.h>

#include <user_config.h>
//#include <network.h>
#include <wifi_config.h>
#include <wifi.h>

// network list
BssList networks;

static uint8 missedConnections = 0;

/**
 * WIFI scan completed callback function
 */
void ICACHE_FLASH_ATTR wifiScanCompleted(bool succeeded, BssList list) {

	debugf("WIFI scan completed, %d networks found\n", list.count());

	if (succeeded) {
		networks.clear();
		for (int i = 0; i < list.count(); i++)
			if (!list[i].hidden && list[i].ssid.length() > 0)
				networks.add(list[i]);
	}

	networks.sort([](const BssInfo& a, const BssInfo& b)
	{	return b.rssi - a.rssi;});
}

BssList wifiScan() {
	WifiStation.startScan(wifiScanCompleted);
	return networks;
}

// Will be called when WiFi station was connected to AP
void connectOk() {
	debugf("WIRELESS CONNECTION ESTABLISHED");

	missedConnections = 0;

	Serial.println("\r\n=== WIFI CLIENT CONNECTED ===");
	Serial.println(WifiStation.getIP());
	Serial.println("==============================\r\n");
}

void connectFail() {
	Serial.println("STATION IS NOT CONNECTED");

	missedConnections++;

	online = false;

	if (missedConnections > maxMissedConnections) {
		Serial.println("Max. missed connections reached");

		// start access point
		if (! WifiAccessPoint.isEnabled())
			WifiAccessPoint.enable(true);
	} else {
		WifiStation.waitConnection(connectOk, 10, connectFail);
	}
}

void configureAP() {
	debugf("configure WIFI AP\n");

	// default SSID
	if (wifiAPSSID.length() == 0) {
			char ssid[64];
			sprintf(ssid, "ESP%d", system_get_chip_id());
			wifiAPSSID = ssid;
	}

	if (wifiEnabled) {
		WifiAccessPoint.enable(wifiAPEnabled);

		if (wifiAPPassword.length() < 1)
			WifiAccessPoint.config(wifiAPSSID, wifiAPPassword, AUTH_OPEN, wifiAPHiddden, wifiAPChannel);
		else {
			WifiAccessPoint.config(wifiAPSSID, wifiAPPassword, AUTH_WPA_WPA2_PSK, wifiAPHiddden, wifiAPChannel);
		}
		debugf("Configure WIFI AP: SSID=%s, PASS=%s\n", wifiAPSSID.c_str(), wifiAPPassword.c_str());
	} else {
		debugf("WIFI is disabled\n");
		// Shutdown AP interface
		WifiAccessPoint.enable(false);
	}
	debugf("(re-)configured WIFI AP\n");
}

void configureStation() {
	debugf("configure WIFI Station\n");

	if (wifiEnabled) {
		WifiStation.enable(wifiStationEnabled);
		WifiStation.config(wifiStationSSID, wifiStationPassword);
		debugf("Configure WIFI STA: SSID=%s, PASS=%s\n", wifiStationSSID.c_str(), wifiStationPassword.c_str());
	} else {
		debugf("WIFI is disabled\n");
		// Shutdown station interface
		WifiStation.disconnect();
		WifiStation.enable(false);
	}

	WifiStation.enableDHCP(dhcp);
	if (!dhcp && !ip.isNull())
		WifiStation.setIP(ip, netmask, gateway);
	debugf("(re-)configured WIFI station\n");
}

void wifiSetup() {
	configureAP();

	configureStation();

	// Run our method when station was connected to AP
	WifiStation.waitConnection(connectOk, 30, connectFail);
	WifiStation.startScan(wifiScanCompleted);
}

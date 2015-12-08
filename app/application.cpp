/*
 * application.cpp
 *
 *  Created on: 06.09.2015
 *      Author: Hakan Coskun <hakan.coskun@blueonshop.de>
 */

#include <network.h>

#include <SmingCore/SmingCore.h>

void init() {
	Serial.printf("Starting Smart Sensor\n");
	Serial.printf("app: start init. Heap size: %d\n", xPortGetFreeHeapSize());

	networkSetup();

	Serial.printf("app: after init. Heap size: %d\n", xPortGetFreeHeapSize());
}

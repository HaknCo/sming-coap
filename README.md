# sming-coap

This project provides a CoAP Server (based on Lobaro CoAP Server https://github.com/Lobaro/lobaro-coap) and a own CoAP client 
implementation.

The client is written in C++ and builds upon cantcoap (https://github.com/staropram/cantcoap) for PDU construction.

## Methods

Following methods are provided.

 1. GET
 2. POST
 3. PUT
 4. DEL


## Planned features

 1. Observe
 2. Blockwise Transfer

Requirements:
  - Sming Framework (https://github.com/SmingHub/Sming)
  - NodeMCU or Olimex ESP8266-EVB (other modules should also work, not tested yet)
  

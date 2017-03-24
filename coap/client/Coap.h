#pragma once
#pragma clang diagnostic ignored "-Wdeprecated-writable-strings"
#pragma clang diagnostic ignored "-Wconstant-logical-operand"
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"

// Copyright (c) 2013, Ashley Mills.
// modified by Hakan Coskun

#include "dbg.h"

#include <SmingCore/SmingCore.h>

/* pre-defined constants that reflect defaults for CoAP */

#define COAP_DEFAULT_RESPONSE_TIMEOUT  2 /* response timeout in seconds */
#define COAP_DEFAULT_MAX_RETRANSMIT    4 /* max number of retransmissions */
#define COAP_DEFAULT_PORT           5683 /* CoAP default UDP port */
#define COAP_DEFAULT_MAX_AGE          60 /* default maximum object lifetime in seconds */
#ifndef COAP_MAX_PDU_SIZE
#define COAP_MAX_PDU_SIZE           1400 /* maximum size of a CoAP PDU */
#endif /* COAP_MAX_PDU_SIZE */

#define COAP_DEFAULT_VERSION           1 /* version of CoAP supported */
#define COAP_DEFAULT_SCHEME        "coap" /* the default scheme for CoAP URIs */

/** well-known resources URI */
#define COAP_DEFAULT_URI_WELLKNOWN ".well-known/core"

#ifdef __COAP_DEFAULT_HASH
/* pre-calculated hash key for the default well-known URI */
#define COAP_DEFAULT_WKC_HASHKEY   "\345\130\144\245"
#endif

#define COAP_HDR_SIZE 4
#define COAP_OPTION_HDR_BYTE 1

/* CoAP result codes (HTTP-Code / 100 * 40 + HTTP-Code % 100) */

/* As of draft-ietf-core-coap-04, response codes are encoded to base
 * 32, i.e.  the three upper bits determine the response class while
 * the remaining five fine-grained information specific to that class.
 */
#define COAP_RESPONSE_CODE(N) (((N)/100 << 5) | (N)%100)

/* Determines the class of response code C */
#define COAP_RESPONSE_CLASS(C) (((C) >> 5) & 0xFF)


// CoAP PDU format

//   0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |Ver| T |  TKL  |      Code     |          Message ID           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |   Token (if any, TKL bytes) ...
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |   Options (if any) ...
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |1 1 1 1 1 1 1 1|    Payload (if any) ...
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class CoapPDU {

	public:
		/// CoAP message types. Note, values only work as enum.
		enum Type {
			COAP_CONFIRMABLE=0x00,
			COAP_NON_CONFIRMABLE=0x10,
			COAP_ACKNOWLEDGEMENT=0x20,
			COAP_RESET=0x30
		};

		// CoAP response codes.
		enum Code {
			COAP_EMPTY=0x00,
			COAP_GET,
			COAP_POST,
			COAP_PUT,
			COAP_DELETE,
			COAP_LASTMETHOD=0x1F,
			COAP_CREATED=0x41,
			COAP_DELETED,
			COAP_VALID,
			COAP_CHANGED,
			COAP_CONTENT,
			COAP_BAD_REQUEST=0x80,
			COAP_UNAUTHORIZED,
			COAP_BAD_OPTION,
			COAP_FORBIDDEN,
			COAP_NOT_FOUND,
			COAP_METHOD_NOT_ALLOWED,
			COAP_NOT_ACCEPTABLE,
			COAP_PRECONDITION_FAILED=0x8C,
			COAP_REQUEST_ENTITY_TOO_LARGE=0x8D,
			COAP_UNSUPPORTED_CONTENT_FORMAT=0x8F,
			COAP_INTERNAL_SERVER_ERROR=0xA0,
			COAP_NOT_IMPLEMENTED,
			COAP_BAD_GATEWAY,
			COAP_SERVICE_UNAVAILABLE,
			COAP_GATEWAY_TIMEOUT,
			COAP_PROXYING_NOT_SUPPORTED,
			COAP_UNDEFINED_CODE=0xFF
		};

		/// CoAP option numbers.
		enum Option {
			COAP_OPTION_IF_MATCH=1,
			COAP_OPTION_URI_HOST=3,
			COAP_OPTION_ETAG,
			COAP_OPTION_IF_NONE_MATCH,
			COAP_OPTION_OBSERVE,
			COAP_OPTION_URI_PORT,
			COAP_OPTION_LOCATION_PATH,
			COAP_OPTION_URI_PATH=11,
			COAP_OPTION_CONTENT_FORMAT,
			COAP_OPTION_MAX_AGE=14,
			COAP_OPTION_URI_QUERY,
			COAP_OPTION_ACCEPT=17,
			COAP_OPTION_LOCATION_QUERY=20,
			COAP_OPTION_BLOCK2=23,
			COAP_OPTION_BLOCK1=27,
			COAP_OPTION_SIZE2,
			COAP_OPTION_PROXY_URI=35,
			COAP_OPTION_PROXY_SCHEME=39,
			COAP_OPTION_SIZE1=60
		};

		/// CoAP content-formats.
		enum ContentFormat {
			COAP_CONTENT_FORMAT_TEXT_PLAIN = 0,   /* text/plain (UTF-8) */
			COAP_CONTENT_FORMAT_APP_LINK  = 40,   /* application/link-format */
			COAP_CONTENT_FORMAT_APP_XML,          /* application/xml */
			COAP_CONTENT_FORMAT_APP_OCTET,        /* application/octet-stream */
			COAP_CONTENT_FORMAT_APP_RDF_XML,      /* application/rdf+xml */
			COAP_CONTENT_FORMAT_APP_EXI   = 47,   /* application/exi  */
			COAP_CONTENT_FORMAT_APP_JSON  = 50,   /* application/json  */
			COAP_CONTENT_FORMAT_APP_CBOR  = 60    /* application/cbor  */
		};


		/// Sequence of these is returned by CoapPDU::getOptions()
		struct CoapOption {
			uint16 optionDelta;
			uint16 optionNumber;
			uint16 optionValueLength;
			int totalLength;
			uint8 *optionPointer;
			uint8 *optionValuePointer;
		};

		// construction and destruction
		CoapPDU();
		CoapPDU(uint8 *pdu, int pduLength);
		CoapPDU(uint8 *buffer, int bufferLength, int pduLength);
		~CoapPDU();
		int reset();
		int validate();

		// version
		int setVersion(uint8 version);
		uint8 getVersion();

		// message type
		void setType(CoapPDU::Type type);
		CoapPDU::Type getType();

		// tokens
		int setTokenLength(uint8 tokenLength);
		int getTokenLength();
		uint8* getTokenPointer();
		int setToken(uint8 *token, uint8 tokenLength);

		// message code
		void setCode(CoapPDU::Code code);
		CoapPDU::Code getCode();
		CoapPDU::Code httpStatusToCode(int httpStatus);

		// message ID
		int setMessageID(uint16 messageID);
		uint16 getMessageID();

		// options
		int addOption(uint16 optionNumber, uint16 optionLength, uint8 *optionValue);
		// gets a list of all options
		CoapOption* getOptions();
		int getNumOptions();
		// shorthand helpers
		int setURI(char *uri);
		int setURI(char *uri, int urilen);
		int getURI(char *dst, int dstlen, int *outLen);
		int addURIQuery(char *query);

		// content format helper
		int setContentFormat(CoapPDU::ContentFormat format);

		// payload
		uint8* mallocPayload(int bytes);
		int setPayload(uint8 *value, int len);
		uint8* getPayloadPointer();
		int getPayloadLength();
		uint8* getPayloadCopy();

		// pdu
		int getPDULength();
		uint8* getPDUPointer();
		void setPDULength(int len);

		// debugging
		static void printBinary(uint8 b);
		void print();
		void printBin();
		void printHex();
		void printOptionHuman(uint8 *option);
		void printHuman();
		void printPDUAsCArray();

	private:
		// variables
		uint8 *_pdu;
		int _pduLength;

		int _constructedFromBuffer;
		int _bufferLength;

		uint8 *_payloadPointer;
		int _payloadLength;

		int _numOptions;
		uint16 _maxAddedOptionNumber;

		// functions
		void shiftPDUUp(int shiftOffset, int shiftAmount);
		void shiftPDUDown(int startLocation, int shiftOffset, int shiftAmount);
		uint8 codeToValue(CoapPDU::Code c);

		// option stuff
		int findInsertionPosition(uint16 optionNumber, uint16 *prevOptionNumber);
		int computeExtraBytes(uint16 n);
		int insertOption(int insertionPosition, uint16 optionDelta, uint16 optionValueLength, uint8 *optionValue);
		uint16 getOptionDelta(uint8 *option);
		void setOptionDelta(int optionPosition, uint16 optionDelta);
		uint16 getOptionValueLength(uint8 *option);
};

/*
#define COAP_CODE_EMPTY 0x00

// method codes 0.01-0.31
#define COAP_CODE_GET 	0x01
#define COAP_CODE_POST 	0x02
#define COAP_CODE_PUT 	0x03
#define COAP_CODE_DELETE 0x04

// Response codes 2.00 - 5.31
// 2.00 - 2.05
#define COAP_CODE_CREATED 0x41
#define COAP_CODE_DELETED 0x42
#define COAP_CODE_VALID   0x43
#define COAP_CODE_CHANGED 0x44
#define COAP_CODE_CONTENT 0x45

// 4.00 - 4.15
#define COAP_CODE_BAD_REQUEST                0x80
#define COAP_CODE_UNAUTHORIZED               0x81
#define COAP_CODE_BAD_OPTION                 0x82
#define COAP_CODE_FORBIDDEN                  0x83
#define COAP_CODE_NOT_FOUND                  0x84
#define COAP_CODE_METHOD_NOT_ALLOWED         0x85
#define COAP_CODE_NOT_ACCEPTABLE             0x86
#define COAP_CODE_PRECONDITION_FAILED        0x8C
#define COAP_CODE_REQUEST_ENTITY_TOO_LARGE   0x8D
#define COAP_CODE_UNSUPPORTED_CONTENT_FORMAT 0x8F

// 5.00 - 5.05
#define COAP_CODE_INTERNAL_SERVER_ERROR      0xA0
#define COAP_CODE_NOT_IMPLEMENTED            0xA1
#define COAP_CODE_BAD_GATEWAY                0xA2
#define COAP_CODE_SERVICE_UNAVAILABLE        0xA3
#define COAP_CODE_GATEWAY_TIMEOUT            0xA4
#define COAP_CODE_PROXYING_NOT_SUPPORTED     0xA5
*/

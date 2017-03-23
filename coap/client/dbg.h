#pragma once

#include <user_config.h>

//#define COAP_DEBUG 1
//#undef COAP_DEBUG

#define DBG_NEWLINE "\n"

#define LOG_COAP_INFO(...) os_printf(__VA_ARGS__); os_printf(DBG_NEWLINE);
#define LOG_COAP_INFOX(...); os_printf(__VA_ARGS__);
#define LOG_COAP_ERROR(...) os_printf(__VA_ARGS__); os_printf(DBG_NEWLINE);

#ifdef COAP_DEBUG
	#define LOG_COAP_DBG(...) os_printf("%s:%d ",__FILE__,__LINE__); os_printf(__VA_ARGS__); os_printf("\r\n");
	#define LOG_COAP_DBGX(...) os_printf(__VA_ARGS__);
	#define LOG_COAP_DBGLX(...) os_printf("%s:%d ",__FILE__,__LINE__); os_printf(__VA_ARGS__);
	#define LOG_COAP_DBG_PDU() printBin();
#else
	#define LOG_COAP_DBG(...) {};
	#define LOG_COAP_DBGX(...) {};
	#define LOG_COAP_DBGLX(...) {};
	#define LOG_COAP_DBG_PDU() {};
#endif

#pragma once

#include <user_config.h>

//#define DEBUG 1
//#undef DEBUG

#define DBG_NEWLINE "\n"

#define INFO(...) os_printf(__VA_ARGS__); os_printf(DBG_NEWLINE);
#define INFOX(...); os_printf(__VA_ARGS__);
#define ERROR(...) os_printf(__VA_ARGS__); os_printf(DBG_NEWLINE);

#ifdef DEBUG
	#define DBG(...) os_printf("%s:%d ",__FILE__,__LINE__); os_printf(__VA_ARGS__); os_printf("\r\n");
	#define DBGX(...) os_printf(__VA_ARGS__);
	#define DBGLX(...) os_printf("%s:%d ",__FILE__,__LINE__); os_printf(__VA_ARGS__);
	#define DBG_PDU() printBin();
#else
	#define DBG(...) {};
	#define DBGX(...) {};
	#define DBGLX(...) {};
	#define DBG_PDU() {};
#endif

#if !defined(_SNMP_INTERACTIVE_H_)
#define _SNMP_INTERACTIVE_H_

#include "ZQSnmp.h"

struct InterActiveHead 
{
    unsigned long  _serviceSendSeq;
	unsigned long  _agentRecvSeq;
	unsigned long  _lastLossCount;//last time: _lastLossCount = _serviceSendSeq - _agentRecvSeq
};

struct InterActiveContent 
{
    u_char  request[ZQSNMP_MSG_LEN_MAX];//if add member, request must be end.
};

typedef struct InterActive 
{
	struct InterActiveHead     _head;
	struct InterActiveContent  _content;
}InterActive;


#endif//_SNMP_INTERACTIVE_H_
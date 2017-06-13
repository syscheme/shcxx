#ifndef __ZQ_COMMON_TRANSFER_FD_H__
#define __ZQ_COMMON_TRANSFER_FD_H__

#include "eloop_file.h"

namespace ZQ {
	namespace LIPC {
// -----------------------------
// class TransferFdClient
// -----------------------------
class TransferFdClient:public ZQ::eloop::Pipe
{
public:
	TransferFdClient();
	~TransferFdClient();
	virtual void OnConnected(ZQ::eloop::Handle::ElpeError status);
	virtual void OnWrote(ZQ::eloop::Handle::ElpeError status);

};

// -----------------------------
// class TransferFdService
// -----------------------------
class TransferFdService:public ZQ::eloop::Pipe
{
public:
	TransferFdService();
	~TransferFdService();
	virtual void doAccept(ZQ::eloop::Handle::ElpeError status);
	virtual void OnRead(ssize_t nread, const char *buf);
};

}}

#endif
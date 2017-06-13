#include "TransferFd.h"

namespace ZQ {
	namespace LIPC {
// -----------------------------
// class TransferFdClient
// -----------------------------

TransferFdClient::TransferFdClient()
{
}
	
TransferFdClient::~TransferFdClient()
{
}
	 
void TransferFdClient::OnConnected(ZQ::eloop::Handle::ElpeError status)
{

}
	
void TransferFdClient::OnWrote(ZQ::eloop::Handle::ElpeError status)
{

}


// -----------------------------
// class TransferFdService
// -----------------------------
TransferFdService::TransferFdService()
{

}

TransferFdService::~TransferFdService()
{

}

void TransferFdService::doAccept(ZQ::eloop::Handle::ElpeError status)
{

}

void TransferFdService::OnRead(ssize_t nread, const char *buf)
{

}

}}
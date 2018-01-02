#include "eloop_threadpool.h"

namespace ZQ {
namespace eloop {

ThreadRequest::ThreadRequest(Loop& loop)
		:_loop(loop)
{
	work();
}

int ThreadRequest::work()
{
	uv_work_t* req = (uv_work_t *)context_ptr();
	return uv_queue_work(_loop.context_ptr(),req,_cbWork,_cbAfterWrok);
}

void ThreadRequest::_cbWork(uv_work_t *req)
{
	ThreadRequest *h = static_cast<ThreadRequest *>(req->data);
	if (NULL != h)
		h->doWork();
}

void ThreadRequest::_cbAfterWrok(uv_work_t *req,int status)
{
	ThreadRequest *h = static_cast<ThreadRequest *>(req->data);
	if (NULL != h)
		h->OnAfterWork(status);
}
	
} } // namespace ZQ::eloop
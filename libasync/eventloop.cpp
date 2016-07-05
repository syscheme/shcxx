
#include <TimeUtil.h>
#include "eventloop.h"
#include "socket.h"

namespace LibAsync {

	const char* ErrorCodeToStr( ErrorCode c ) {
		switch( c ) {
		case ERR_ERROR:		return "Generic Error";
		case ERR_EOF:		return "EOF";
		case ERR_INVAID:	return "Invalid Paramater";
		case ERR_ADDRINUSE:	return "Address In Use";
		case ERR_TIMEOUT:	return "Timed Out";
		case ERR_CONNREFUSED:	return "Connection Refused";
		case ERR_RECVFAIL:	return "Recv Failed";
		case ERR_SENDFAIL:	return "Send Fail";
		case ERR_EAGAIN:	return "Work In Progress";
		case ERR_SOCKETVAIN:	return "Socket Closed/Operation In Progress";
		case ERR_MEMVAIN:	return "Not Enough Memory";
		case ERR_EPOLLREGISTERFAIL:	return "EPOLL Error";
		case ERR_EPOLLEXCEPTION:		return "EPOLL Exception";
		case ERR_EOF2:		return "EOF2";
		case ERR_EOF3:		return "EOF3";
		default:			return "Unknown Error";
		}
	}

    void BufferHelper::adjust(size_t sentSize)
    {
		assert(sentSize <= _dataSize);
		_dataSize -= sentSize;
		AsyncBufferS::iterator it = _bufs.begin();
		for( ; it != _bufs.end() ; it ++ ) {
			if( sentSize < it->len) {
				it->len -= sentSize;
				it->base += sentSize;
				break;
			}
			sentSize -= it->len;
		}
		_bufs.erase(_bufs.begin(), it );
    }

	AsyncBufferS BufferHelper::getAt(size_t size) {
		AsyncBufferS ret;
		AsyncBufferS::const_iterator it = _bufs.begin();
		for (; size > 0 && it != _bufs.end(); it++) {
			if (size < it->len) {
				AsyncBuffer buf = *it;
				buf.len = size;
				ret.push_back(buf);
				break;
			}
			ret.push_back(*it);
		}
		return ret;
	}


    bool BufferHelper::isEOF( ) const
    {
        return _dataSize == 0;
    }

	size_t buffer_size( const AsyncBufferS& bufs) {
		size_t size = 0;
		AsyncBufferS::const_iterator it = bufs.begin();
		while( it != bufs.end()) {
			size += it->len;
			it++;
		}
		return size;
	}

	EventLoop::EventLoop( ZQ::common::Log&log, int cpuid, LoopCenter* center, int loopId )
		:mCpuId(cpuid),
		mLog(log),
		mPreTime(0),
		mLoopCenter(center),
		mLoopId(loopId)
	{
		createLoop();
	}

	EventLoop::~EventLoop(void)
	{
		destroyLoop();
	}

	void EventLoop::addAsyncWork(AsyncWork::Ptr work) {
		assert(work != NULL);
		bool bPosted = false;
		{
			ZQ::common::MutexGuard gd(mLocker);
			mAsyncWorks.push_back(work);
			bPosted = mbAsyncWorkMessagePosted;
		}
		if(!bPosted)
			wakeupLoop();
	}

	bool EventLoop::addTimer( Timer::Ptr t ) {
		TimerInfo info(t);
		if(info.target <= 0)
			return false;
		bool bWakeUp = false;
		{
			ZQ::common::MutexGuard gd(mLocker);
			TIMERINFOS::iterator it = mTimers.find(info);
			if( it != mTimers.end())
				mTimers.erase(it);
			mTimers.insert(info);
			if( info.target < mNextWakeup){
				bWakeUp = true;
				mNextWakeup = info.target;
			}
		}
		if( bWakeUp)
			wakeupLoop();
		return true;
	}

	void EventLoop::removeTimer( Timer::Ptr t ){
		TimerInfo info(t);
		ZQ::common::MutexGuard gd(mLocker);
		mTimers.erase(info);
	}

	bool EventLoop::start( ){
		mNextWakeup = ZQ::common::now()+10*1000;
		mbRunning = true;
		return ZQ::common::NativeThread::start();
	}

	void EventLoop::stop() {
		mbRunning = false;
		wakeupLoop();
		waitHandle(-1);
	}
	int EventLoop::run() {
		if( mCpuId >= 0 ) {
#ifdef ZQ_OS_LINUX
			int policy;
			struct sched_param param;

			pthread_getschedparam(pthread_self(), &policy, &param);
			param.sched_priority = sched_get_priority_max(policy);
			pthread_setschedparam(pthread_self(), policy, &param);

			cpu_set_t cpuset;
			pthread_t thread = pthread_self();
			CPU_ZERO(&cpuset);
			CPU_SET( mCpuId, &cpuset);
			int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
			assert( rc == 0 );
#elif defined ZQ_OS_MSWIN      
            DWORD_PTR mask = 1 << mCpuId;
            SetThreadAffinityMask(GetCurrentThread(), mask);
#endif
		}

		std::vector<Timer::Ptr> expiredTimers;
		expiredTimers.reserve(4096);
		while(mbRunning) {
			expiredTimers.clear();
			processEvent(mNextWakeup);
			if(!mbRunning)//如果EventLoop被停止了，那么就直接退出
				break;
			{
				ZQ::common::MutexGuard gd(mLocker);
				mbAsyncWorkMessagePosted = false;
			}
			//continue process timer event
			int64 timeNow = ZQ::common::now();			
			{
				/*
				先把当前已经到期的timer全部取出，然后在一个一个的调用
				onTimer，这样可以防止在onTimer的实现中调用updateTimer(0)之类的值，
				导致该Timer不断被执行
				*/
				ZQ::common::MutexGuard gd(mLocker);
				while(true) {
					if(mTimers.empty()) {
						mNextWakeup = ZQ::common::now() + 10 * 1000;//configurable ?
						break;
					}
					TIMERINFOS::iterator it = mTimers.begin();
					if(it->target <= timeNow) {
						expiredTimers.push_back(it->timer);
						mTimers.erase(it);
					} else {
						//直接这样设置mNextWakeup做可能会造成如下问题
						//当下一个即将被执行的timer的到期时间实际上已经非常短，例如还有1ms
						//但是接下来要执行的expiredTimers的onTimer耗时较长。
						//那么下一个被执行的timer其实已经过期很长时间了。
						//但是考虑到我们不需要那么精确的计时，所以我们忽略这个问题
						mNextWakeup = it->target;
						break;
					}
				}
			}
			std::vector<Timer::Ptr>::iterator itExpired = expiredTimers.begin();
			while( itExpired != expiredTimers.end()) {
				(*itExpired)->onTimerEvent();
				itExpired++;
			}
			expiredTimers.clear();

			// process one-shot async works
			std::list<AsyncWork::Ptr> asyncWorks;
			{
				ZQ::common::MutexGuard gd(mLocker);
				asyncWorks.swap(mAsyncWorks);
			}
			std::list<AsyncWork::Ptr>::const_iterator itAsyncWork = asyncWorks.begin();
			for( ; itAsyncWork != asyncWorks.end(); itAsyncWork++ ) {
				AsyncWork::Ptr work = *itAsyncWork;
				work->onWorkExecute();
			}
			asyncWorks.clear();
		}
		mAsyncWorks.clear();
		mTimers.clear();
		return 0;
	}

	///asycn work
	AsyncWork::AsyncWork(EventLoop& loop)
	:mLoop(loop),
	mWorkQueued(false) {
	}

	AsyncWork::~AsyncWork( ) {
	}

	void AsyncWork::queueWork() {
		mLoop.addAsyncWork(this);
	}

	void AsyncWork::onWorkExecute( ) {
		mWorkQueued = false;
		if(mFuncCB) {
			mFuncCB();
		} else {
			onAsyncWork();
		}
	}

	void AsyncWork::bindCB( FUNC_ONASYNCWORK cb ) {
		mFuncCB = cb;
	}

	AsyncWork::Ptr AsyncWork::create(EventLoop& loop) {
		return new AsyncWork(loop);
	}
	//////////////////////////////////////////////////////
	/// Timer
	Timer::Timer(EventLoop& loop)
	:mLoop(loop),
	mTarget(0){
	}
	
	Timer::Ptr Timer::create( EventLoop& loop) {
		return new Timer(loop);
	}

	Timer::~Timer(){
		cancel();
	}

	void Timer::update(uint64 delta){
		if(mTarget !=0) {
			mLoop.removeTimer(this);
		}

		mTarget = ZQ::common::now() + delta;
		mLoop.addTimer(this);
	}
	
	void Timer::cancel() {
		mLoop.removeTimer(this);
		mTarget = 0;
	}

	void Timer::onTimerEvent() {
		if(mFuncCB) {
			mFuncCB();
		} else {
			onTimer();
		}
	}

	void Timer::bindCB( FUNC_ONTIMER cb ) {
		mFuncCB = cb;
	}
	

	

}//namespace LibAsync
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312

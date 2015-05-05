
#include <TimeUtil.h>
#include "eventloop.h"
#include "socket.h"

namespace LibAsync {

	size_t buffer_size( const AsyncBufferS& bufs) {
		size_t size = 0;
		AsyncBufferS::const_iterator it = bufs.begin();
		while( it != bufs.end()) {
			size += it->len;
			it++;
		}
		return size;
	}

	EventLoop::EventLoop( int cpuid )
		:mCpuId(cpuid)
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
			if(!mbRunning)//���EventLoop��ֹͣ�ˣ���ô��ֱ���˳�
				break;
			{
				ZQ::common::MutexGuard gd(mLocker);
				mbAsyncWorkMessagePosted = false;
			}
			//continue process timer event
			int64 timeNow = ZQ::common::now();			
			{
				/*
				�Ȱѵ�ǰ�Ѿ����ڵ�timerȫ��ȡ����Ȼ����һ��һ���ĵ���
				onTimer���������Է�ֹ��onTimer��ʵ���е���updateTimer(0)֮���ֵ��
				���¸�Timer���ϱ�ִ��
				*/
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
						//ֱ����������mNextWakeup�����ܻ������������
						//����һ��������ִ�е�timer�ĵ���ʱ��ʵ�����Ѿ��ǳ��̣����绹��1ms
						//���ǽ�����Ҫִ�е�expiredTimers��onTimer��ʱ�ϳ���
						//��ô��һ����ִ�е�timer��ʵ�Ѿ����ںܳ�ʱ���ˡ�
						//���ǿ��ǵ����ǲ���Ҫ��ô��ȷ�ļ�ʱ���������Ǻ����������
						mNextWakeup = it->target;
						break;
					}
				}
			}
			std::vector<Timer::Ptr>::iterator itExpired = expiredTimers.begin();
			while( itExpired != expiredTimers.end()) {
				(*itExpired)->onTimer();
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
		onAsyncWork();
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

	

}//namespace LibAsync
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312

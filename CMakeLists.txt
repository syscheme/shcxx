cmake_minimum_required(VERSION 2.8)

project(shcxx)

add_definitions("-lpthread -g")

add_library(shcxx SHARED 
	Exception.cpp TimeUtil.cpp
	linux_com.cpp FileSystemOp.cpp SystemUtils.cpp
	ExpatMap.cpp expatxx.cpp expatxxx.cpp
	XMLPreferenceEx.cpp XmlRpcSerializer.cpp 
	md5.cpp MD5CheckSumUtil.cpp Semaphore.cpp
	Guid.cpp strHelper.cpp urlstr.cpp Variant.cpp
	NativeThread.cpp NativeThreadPool.cpp 
	Log.cpp FileLog.cpp PollingTimer.cpp DynSharedObj.cpp 
	InetAddr.cpp UDPSocket.cpp Socket.cpp UDPDuplex.cpp
	Scheduler.cpp rwlock_Linux.cpp RuleEngine.cpp
	)

target_link_libraries (shcxx pthread dl expat uuid rt)


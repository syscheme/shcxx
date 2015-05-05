cmake_minimum_required(VERSION 2.8)

project(ZQCommon)

add_library(ZQCommon SHARED Exception.cpp TimeUtil.cpp urlstr.cpp Variant.cpp
							linux_com.cpp FileSystemOp.cpp SystemUtils.cpp Log.cpp
							DynSharedObj.cpp ExpatMap.cpp expatxx.cpp expatxxx.cpp
							Guid.cpp md5.cpp MD5CheckSumUtil.cpp Semaphore.cpp 
							NativeThread.cpp FileLog.cpp PollingTimer.cpp XMLPreferenceEx.cpp
							InetAddr.cpp UDPSocket.cpp Socket.cpp strHelper.cpp UDPDuplex.cpp
							NativeThreadPool.cpp Scheduler.cpp rwlock_Linux.cpp XmlRpcSerializer.cpp 
							RuleEngine.cpp	)

target_link_libraries (ZQCommon pthread dl expat uuid rt)
SUBDIRS(DataPostHouse)
SUBDIRS(HttpEngine)

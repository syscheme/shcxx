cmake_minimum_required(VERSION 2.8)

project(shcxx)

# the configured options and settings for Tutorial
# #define Tutorial_VERSION_MAJOR @Tutorial_VERSION_MAJOR@
# #define Tutorial_VERSION_MINOR @Tutorial_VERSION_MINOR@
set (shcxx_VERSION_MAJOR 1)
set (shcxx_VERSION_MINOR 0)

# add the binary tree to the search path for include files so that we will find xxxconfig.h
# include_directories("${PROJECT_BINARY_DIR}")

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

install (TARGETS shcxx
		DESTINATION bin)

install (FILES Log.h 
		DESTINATION include/shcxx)

include(CTest)

# does the application run
add_test (TutorialRuns Tutorial 25)
# does it sqrt of 25
add_test (TutorialComp25 Tutorial 25)
set_tests_properties (TutorialComp25 PROPERTIES PASS_REGULAR_EXPRESSION "25 is 5")
# does it handle negative numbers
add_test (TutorialNegative Tutorial -25)
set_tests_properties (TutorialNegative PROPERTIES PASS_REGULAR_EXPRESSION "-25 is 0")
# does it handle small numbers
add_test (TutorialSmall Tutorial 0.0001)
set_tests_properties (TutorialSmall PROPERTIES PASS_REGULAR_EXPRESSION "0.0001 is 0.01")
# does the usage message work?
add_test (TutorialUsage Tutorial)
set_tests_properties (TutorialUsage PROPERTIES PASS_REGULAR_EXPRESSION "Usage:.*number")

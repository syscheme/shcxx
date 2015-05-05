include  $(top_srcdir)/config/compileflags.mk

AUTOMAKE_OPTIONS=foreign
define  run-yacc
yacc $(firstword $^)
mv y.tab.c $@
endef

SUBDIRS:=HttpEngine DataPostHouse Rtsp/RtspParser RtspLib/RtspClientLib

lib_LTLIBRARIES=libZQCommon.la

libZQCommon_la_SOURCES=Exception.cpp TimeUtil.cpp  urlstr.cpp  Variant.cpp \
	linux_com.cpp FileSystemOp.cpp SystemUtils.cpp  \
	Log.cpp DynSharedObj.cpp ExpatMap.cpp expatxx.cpp  \
	expatxxx.cpp Guid.cpp md5.cpp MD5CheckSumUtil.cpp \
	Semaphore.cpp NativeThread.cpp FileLog.cpp PollingTimer.cpp  \
	XMLPreferenceEx.cpp InetAddr.cpp UDPSocket.cpp Socket.cpp  \
	strHelper.cpp UDPDuplex.cpp NativeThreadPool.cpp Scheduler.cpp  \
	rwlock_Linux.cpp  XmlRpcSerializer.cpp RuleEngine.cpp  \
	TCPSocket.cpp  RTSPClient.cpp  RTSPSession.cpp  Pointer.cpp  \
	DiskIOPerf.cpp   BufferPool.cpp   HttpClient.cpp
	 
#libZQCommon_la_LDFLAGS=@lib_versioninfo@
AM_LDFLAGS=-lpthread -ldl -lrt -luuid @_expat_dir@/lib/libexpat.a
.PHONY:clean all test
clean-local:
	-rm -rf $(csbin_dir)/$(lib_LTLIBRARIES:%.la=%)*  $(cssyb_dir)/$(lib_LTLIBRARIES:%.la=%)*
tsflag:="so"
goal:=libZQCommon.so
include  $(top_srcdir)/config/endpros.mk

TOPDIR := ..

include $(TOPDIR)/build/defines.mk

SOLIB := ZQCommon

OBJS := Exception.os       \
        TimeUtil.os        \
		urlstr.os          \
		Variant.os         \
		linux_com.os       \
        FileSystemOp.os    \
		SystemUtils.os     \
		Log.os             \
		DynSharedObj.os    \
        ExpatMap.os        \
		expatxx.os         \
		expatxxx.os        \
		Guid.os            \
		md5.os             \
		MD5CheckSumUtil.os \
		Semaphore.os       \
        NativeThread.os    \
		FileLog.os         \
		PollingTimer.os    \
		XMLPreferenceEx.os \
        InetAddr.os        \
		UDPSocket.os       \
		Socket.os          \
		strHelper.os       \
		UDPDuplex.os       \
		NativeThreadPool.os\
        Scheduler.os       \
		rwlock_Linux.os    \
		XmlRpcSerializer.os\
		RuleEngine.os	   \
		TCPSocket.os       \
		RTSPClient.os      \
		RTSPSession.os     \
		Pointer.os         \
		DiskIOPerf.os      \
		BufferPool.os      \
		HttpClient.os				\
		SystemInfo.os

LDFLAGS += -lpthread -ldl -lrt -luuid $(_expat_dir)/lib/libexpat.a

include $(TOPDIR)/build/common.mk

# vim: ts=4 sw=4 bg=dark

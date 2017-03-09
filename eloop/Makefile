TOPDIR := ../..

include $(TOPDIR)/build/defines.mk

SOLIB	:= eloop

OBJS	:= eloop.os eloop_net.os eloop_lock.os eloop_file.os eloop_threadpool.os

INCDIR += . .. ./libuv_1.9.1/include/ ./libuv_1.9.1/src ./libuv_1.9.1/src/unix 

LINKDIR += ./libuv_1.9.1/out/Release
LDFLAGS += -lZQCommon -luv -lpthread 

include $(TOPDIR)/build/common.mk

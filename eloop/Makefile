TOPDIR := ../..

include $(TOPDIR)/build/defines.mk

SOLIB	:= eloop

OBJS	:= eloop.os eloop_net.os eloop_lock.os eloop_file.os eloop_threadpool.os LIPC.os UnixSocket.os jsoncpp.cpp

INCDIR += . .. $(_uv_dir)/include

LINKDIR += $(_uv_dir)/lib64
LDFLAGS += -lZQCommon -luv -lpthread 

include $(TOPDIR)/build/common.mk

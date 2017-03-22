TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

TARGET   := hlsget

OBJS    := hlsget.o Download.o

INCDIR += $(TOPDIR)/Common/eloop $(TOPDIR)/Common/eloop/Http/LibHttp $(TOPDIR)/Common/eloop/libuv_1.9.1/include/ $(TOPDIR)/Common/eloop/libuv_1.9.1/src $(TOPDIR)/Common/eloop/libuv_1.9.1/src/unix

LINKDIR += $(TOPDIR)/Common/eloop/libuv_1.9.1/out/Release $(TOPDIR)/Common/eloop/Http/LibHttp $(TOPDIR)/Common/eloop
LDFLAGS += -lZQCommon -lHttp -leloop -luv -lpthread

include $(TOPDIR)/build/common.mk


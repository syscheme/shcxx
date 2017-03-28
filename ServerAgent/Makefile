TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

TARGET := ServerAgent

OBJS := ServerAgent.o ServerApp.o #http_parser.o HttpConnection.o HttpMessage.o HttpServer.o 
INCDIR += . ../eloop/libuv_1.9.1/include ../eloop/Http/LibHttp ../eloop
VPATH += ../eloop/Http/LibHttp ../eloop

LINKDIR += ../eloop/libuv_1.9.1/out/Release ../eloop/Http/LibHttp ../eloop .
LDFLAGS += -leloop -lHttp -lZQSnmp -lZQCommon -lboost_regex

LINKDIR += $(TOPDIR)/Common/eloop/libuv_1.9.1/out/Release $(TOPDIR)/Common/eloop $(TOPDIR)/Common/eloop/Http/LibHttp 

include $(TOPDIR)/build/common.mk

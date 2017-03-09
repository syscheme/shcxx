TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

LIB := Http
OBJS := HttpServer.o HttpClient.o HttpConnection.o HttpMessage.o http_parser.o

INCDIR += ../.. $(TOPDIR)/Common/eloop $(TOPDIR)/Common/eloop/libuv_1.9.1/include $(TOPDIR)/Common/eloop/libuv_1.9.1/src $(TOPDIR)/Common/eloop/libuv_1.9.1/src/unix

include $(TOPDIR)/build/common.mk



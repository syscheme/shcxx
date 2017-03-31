TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

LIB := Http
OBJS := HttpServer.o HttpClient.o HttpConnection.o HttpMessage.o http_parser.o

INCDIR += $(_uv_dir)/include $(TOPDIR)/Common/ $(TOPDIR)/Common/eloop 

include $(TOPDIR)/build/common.mk



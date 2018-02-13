TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

LIB := Http
OBJS := TCPServer.o RTSPMessage.o RTSPConnection.o RTSPServer.o RTSPClient.o HttpServer.o HttpClient.o HttpConnection.o HttpMessage.o http_parser.o RedisClient.o

INCDIR += $(_uv_dir)/include $(TOPDIR)/Common/ $(TOPDIR)/Common/eloop 

include $(TOPDIR)/build/common.mk



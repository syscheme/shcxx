TOPDIR := ../..

include $(TOPDIR)/build/defines.mk

SOLIB	:= async
 
OBJS	:= socket.os socket_linux.os eventloop.os eventloop_linux.os \
         http_parser.os http.os httpparser.os udpsocket_linux.os udpsocket.os

INCDIR += . ..  

LDFLAGS += -lZQCommon -lboost_regex

include $(TOPDIR)/build/common.mk

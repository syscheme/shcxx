TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

TARGET	:= socketclient
 
OBJS	:= socket.o socket_linux.o eventloop.o eventloop_linux.o client.o clientMain.o writeThread.o

INCDIR += . ../.. ../../..

VPATH += :.:../..

LDFLAGS += -lZQCommon 

include $(TOPDIR)/build/common.mk

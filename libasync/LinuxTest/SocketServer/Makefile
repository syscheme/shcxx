TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

TARGET	:= socketserver
 
OBJS	:= socket_linux.o socket.o eventloop.o eventloop_linux.o  DoSocket.o SocketServer.o Main.o

INCDIR += . ../.. ../../..

VPATH += :.:../..

LDFLAGS += -L. -lZQCommon

include $(TOPDIR)/build/common.mk
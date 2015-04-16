TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

TARGET	:= HttpImplCurl
 
OBJS	:= C2Downloader.o AllocateServer.o getopt.o ScriptParser.o ServerManager.o main.o

INCDIR += . .. ../.. ../WinTest/C2Downloader $(TOPDIR)/Common

VPATH += ..:../WinTest/C2Downloader

LDFLAGS += -lZQCommon -lasync

include $(TOPDIR)/build/common.mk
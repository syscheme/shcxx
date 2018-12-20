TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

TARGET   := multi_echo_server 

OBJS    := main.o MultiEchoServer.o 

INCDIR += $(TOPDIR)/Common/eloop $(_uv_dir)/include


LINKDIR += $(_uv_dir)/lib64 $(TOPDIR)/Common/eloop
LDFLAGS += -lZQCommon -luv -leloop -lpthread

include $(TOPDIR)/build/common.mk


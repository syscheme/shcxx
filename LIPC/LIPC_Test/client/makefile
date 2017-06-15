TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

TARGET := client

OBJS	:= client.o

INCDIR += . ../../LIPC $(_uv_dir)/include $(_jsoncpp_dir)/include $(TOPDIR)/Common $(TOPDIR)/Common/eloop

LINKDIR += $(_uv_dir)/lib64 $(_jsoncpp_dir)/libs

LDFLAGS += -lZQCommon -luv -leloop -lLIPC -lpthread 

include $(TOPDIR)/build/common.mk
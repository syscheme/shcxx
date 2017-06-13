TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

SOLIB	:= LIPC

OBJS	:= LIPC.os TransferFd.os

INCDIR += . .. $(_uv_dir)/include $(TOPDIR)/Common $(TOPDIR)/Common/eloop

LINKDIR += $(_uv_dir)/lib64

LDFLAGS += -lZQCommon -luv -leloop -lpthread 

include $(TOPDIR)/build/common.mk
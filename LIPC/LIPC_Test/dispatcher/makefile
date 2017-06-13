TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

TARGET := Dispatcher

OBJS	:= main.o

INCDIR += . ../../LIPC $(_uv_dir)/include $(TOPDIR)/Common $(TOPDIR)/Common/eloop

LINKDIR += $(_uv_dir)/lib64

LDFLAGS += -lZQCommon -luv -leloop -lLIPC -lpthread 

include $(TOPDIR)/build/common.mk
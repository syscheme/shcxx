TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

TARGET   := hlsget

OBJS    := hlsget.o Download.o

INCDIR += $(TOPDIR)/Common/eloop $(TOPDIR)/Common/eloop/Http/LibHttp $(_uv_dir)/include

LINKDIR += $(_uv_dir)/lib64 $(TOPDIR)/Common/eloop/Http/LibHttp $(TOPDIR)/Common/eloop
LDFLAGS += -lZQCommon -luv -lHttp -lboost_regex -leloop -lpthread

include $(TOPDIR)/build/common.mk


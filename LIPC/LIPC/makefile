TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

SOLIB	:= LIPC

OBJS	:= LIPC.os Handler.os PipeConnection.os

INCDIR += . .. $(_uv_dir)/include $(_jsoncpp_dir)/include $(TOPDIR)/Common $(TOPDIR)/Common/eloop

LINKDIR += $(_uv_dir)/lib64 $(_jsoncpp_dir)/libs

LDFLAGS += -lZQCommon -luv -ljson -leloop 

include $(TOPDIR)/build/common.mk

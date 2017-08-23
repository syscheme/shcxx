TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

SOLIB	:= LIPC

OBJS	:= LIPC.os Handler.os PipeConnection.os

INCDIR += . .. $(TOPDIR)/Common/eloop/libuv_1.9.1/include $(_jsoncpp_dir)/include $(TOPDIR)/Common $(TOPDIR)/Common/eloop

LINKDIR += $(TOPDIR)/Common/eloop/ $(TOPDIR)/Common/eloop/libuv_1.9.1/out/Release $(_jsoncpp_dir)/libs

LDFLAGS += -lZQCommon -luv -ljson -leloop 

include $(TOPDIR)/build/common.mk

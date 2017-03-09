TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

TARGET := HttpCRG

OBJS := ZQDaemon.o ConfigHelper.o HttpCRGConfig.o HttpCRG.o CRGateway.o

INCDIR += $(TOPDIR)/Common $(TOPDIR)/Common/HttpEngine $(TOPDIR)/Common/eloop/Http/LibHttp $(TOPDIR)/Common/eloop $(TOPDIR)/Common/eloop/libuv_1.9.1/include $(TOPDIR)/Common/eloop/libuv_1.9.1/src $(TOPDIR)/Common/eloop/libuv_1.9.1/src/unix 

CXXFLAGS +=  -DCONFIG_NO_SNMP

LINKDIR += $(TOPDIR)/Common/eloop/libuv_1.9.1/out/Release $(TOPDIR)/Common/eloop/ $(TOPDIR)/Common/eloop/Http/LibHttp
LDFLAGS +=  $(snmplink) $(commonlink) $(icelink) -leloop -lHttp -luv -lServiceShell -lboost_regex -ldl -lpthread -lboost_system-mt

include $(TOPDIR)/build/common.mk  


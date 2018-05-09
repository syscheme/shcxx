TOPDIR := ../..

include $(TOPDIR)/build/defines.mk

TARGET := ServerAgent

OBJS := ServerAgent.o ServerApp.o 
INCDIR += $(_uv_dir)/include $(TOPDIR)/Common $(TOPDIR)/Common/eloop $(TOPDIR)/Common/eloop/Http/LibHttp



LDFLAGS += -lHttp -leloop $(LDOPTS_snmp) $(LDOPTS_zqcomm) -lboost_regex

#LINKDIR += $(TOPDIR)/Common/eloop $(TOPDIR)/Common/eloop/Http/LibHttp 

include $(TOPDIR)/build/common.mk

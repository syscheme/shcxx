TOPDIR := ../..

include $(TOPDIR)/build/defines.mk

SOLIB  := ZQSnmp
 
OBJS   := ModuleMIB.os SnmpVar.os SubAgent.os Agent_netsnmp.os

INCDIR += $(TOPDIR)/Common

LDFLAGS += -lZQCommon

include $(TOPDIR)/build/common.mk

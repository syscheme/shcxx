TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

TARGET  := testSnmp

OBJS   := test.o

INCDIR += ../ $(TOPDIR)/Common $(TOPDIR)

LDFLAGS += -lZQCommon -lZQSnmp -lsnmp
CXXFLAGS += -D_DEBUG

include $(TOPDIR)/build/common.mk

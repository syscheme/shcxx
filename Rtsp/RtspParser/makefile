TOPDIR := ../../..

include $(TOPDIR)/build/defines.mk

LIB := RtspParser

_src_ext := cxx

_dir := ../Utils

INCDIR += $(_dir)
VPATH += $(_dir)

OBJS := $(patsubst %.$(_src_ext), %.o, $(wildcard *.$(_src_ext)) \
		mstring.o StringData.o Data.o cpLog.o VMutex.o)

include $(TOPDIR)/build/common.mk




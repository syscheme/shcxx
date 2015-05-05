TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

TARGET := testForMainFile

OBJS := GetMainFile.o SimpleXMLParser.o IndexFileParser.o IdxFileParserEnvironment.o testMainFile.o

INCDIR += $(TOPDIR)/TianShan/CDNLib/CDNSS $(TOPDIR)/TianShan/CDNLib/CDNSS/C2Streamer $(_vstrm_inc_dir) $(TOPDIR)/Common/libasync

VPATH += $(TOPDIR)/TianShan/CDNLib/CDNSS $(TOPDIR)/TianShan/CDNLib/CDNSS/C2Streamer

CXXFLAGS += -DEXCLUDE_VSTRM_API

LDFLAGS += -lZQCommon -lasync

include $(TOPDIR)/build/common.mk
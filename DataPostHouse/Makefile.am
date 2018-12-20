include  $(top_srcdir)/config/compileflags.mk
CXXFLAGS+=-fPIC -ggdb

AUTOMAKE_OPTIONS = foreign
lib_LIBRARIES = libDataPostHouse.a
libDataPostHouse_a_SOURCES = DataDialogFactoryI.cpp DataPostHouseEnv.cpp DataPostHouseLinux.cpp \
			     DataCommunicator_linux.cpp DataCommunicatorSSL_linux.cpp 
#AM_CPPFLAGS = $(CXXFLAGS)
#AM_LDFLAGS = $(LDFLAGS)
tsflag:="ar"
goal:=libDataPostHouse.a
include  $(top_srcdir)/config/endpros.mk


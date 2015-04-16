include  $(top_srcdir)/config/compileflags.mk

CXXFLAGS  += -fPIC
AUTOMAKE_OPTIONS=foreign 
lib_LIBRARIES=libHttpEngine.a
libHttpEngine_a_SOURCES=ClientRequestGw.cpp HttpEngine.cpp HttpUtil.cpp
CXXFLAGS+=-I../DataPostHouse
tsflag:="ar"
goal:=libHttpEngine.a
include $(top_srcdir)/config/endpros.mk

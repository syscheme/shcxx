include  $(top_srcdir)/config/compileflags.mk
CXXFLAGS+=-fPIC
CXXFLAGS+=-I.. -I../../DataPostHouse
VPATH=:..:@VPATHS@:
AUTOMAKE_OPTIONS=foreign
lib_LIBRARIES=libRtspClient.a
libRtspClient_a_SOURCES=RtspDak.cpp RtspDialogImpl.cpp RtspInterfaceImpl.cpp RtspMsgProcessor.cpp	\
			RtspUtils.cpp RtspClient.cpp RtspClientFactory.cpp
libRtspClient_a_LIBADD=
tsflag:="ar"
goal:=libRtspClient.a
include  $(top_srcdir)/config/endpros.mk
	

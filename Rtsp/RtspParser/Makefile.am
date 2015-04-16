include  $(top_srcdir)/config/compileflags.mk

CXXFLAGS+=-I../Utils
VPATH=:../Utils:@VPATHS@:
SOURCESALL:=CharData.cxx  CharDataParser.cxx  DataException.cxx  RtspAnnounceMsg.cxx  \
	 RtspDescribeMsg.cxx  RtspLocationHdr.cxx  RtspMsg.cxx  RtspMsgParser.cxx \
	 RtspOptionsMsg.cxx  RtspPauseMsg.cxx  RtspPlayMsg.cxx  RtspRangeHdr.cxx  \
	 RtspRecordMsg.cxx  RtspRequest.cxx  RtspResponse.cxx  RtspRtpInfoHdr.cxx  \
	 RtspSetParameterMsg.cxx  RtspSetupMsg.cxx  RtspTeardownMsg.cxx  RtspTransportHdr.cxx  \
	 RtspUtil.cxx  Sptr.cxx  VException.cxx mstring.cxx StringData.cxx Data.cxx cpLog.cxx VMutex.cxx

AUTOMAKE_OPTIONS=foreign
lib_LIBRARIES=libRtspParser.a
libRtspParser_a_SOURCES=$(SOURCESALL)
libRtspParser_a_LIBADD=
AM_LDFLAGS=
tsflag:="ar"
goal:=libRtspParser.a
include $(top_srcdir)/config/endpros.mk


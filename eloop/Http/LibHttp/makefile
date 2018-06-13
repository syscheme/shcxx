TOPDIR := ../../../..

include $(TOPDIR)/build/defines.mk

SOLIB	:= Http

OBJS	:= TCPServer.os RTSPMessage.os RTSPConnection.os RTSPServer.os RTSPClient.os HttpServer.os HttpClient.os HttpConnection.os HttpMessage.os http_parser.os

INCDIR += $(_uv_dir)/include $(TOPDIR)/Common/eloop

LDFLAGS += -leloop -lZQCommon -lboost_regex -lpthread

include $(TOPDIR)/build/common.mk

prebuild:
	if [ -e ../../../RedisClient.os ]; then $(RM) $(RMFLAGS) ../../../RedisClient.os; fi
	if [ -e ../../../HttpClient.os ]; then $(RM) $(RMFLAGS) ../../../HttpClient.os; fi



#ifndef __RTSP_Connection_H__
#define __RTSP_Connection_H__

#include "RTSPMessage.h"
#include <Pointer.h>
#include "TCPServer.h"


namespace ZQ {
namespace eloop {

//-------------------------------------
//	class RTSPParser
//-------------------------------------
class RTSPParser
{
public:
	typedef enum {
		TrackVideo = 0, TrackAudio
	} TrackType;

// 	struct RtspTrack{
// 		uint8_t PT;
// 		uint8_t trackId;
// 		uint8_t interleaved;
// 		TrackType type = (TrackType) -1;
// 		string trackSdp;
// 		string trackStyle;
// 		bool inited;
// 		uint32_t ssrc = 0;
// 		uint16_t seq;
// 		uint32_t timeStamp;
// 	};

	enum ParseState {
		STATE_INIT,
		STATE_HEADERS,
		STATE_BODY,
		STATE_COMPLETE
	};
	ParseState			_ParserState;
	size_t parse( const char* data, size_t len);
//	int parserSDP(const std::string& sdp, RtspTrack Track[2]);

};



//-------------------------------------
//	class RTSPConnection
//-------------------------------------
class RTSPConnection : public TCPConnection
{
private:
	RTSPConnection(const RTSPConnection&);
	RTSPConnection& operator=( const RTSPConnection&);

public:
	virtual ~RTSPConnection(){}

protected:
	RTSPConnection(ZQ::common::Log& log, TCPServer* tcpServer = NULL)
		:TCPConnection(log,tcpServer),_byteSeen(0)
	{}

	virtual void doAllocate(eloop_buf_t* buf, size_t suggested_size);

	virtual void OnRead(ssize_t nread, const char *buf);
	virtual void OnWrote(int status);

	virtual void onError( int error,const char* errorDescription ){}

protected: // impl of RTSPParseSink
	virtual void OnResponses(RTSPMessage::MsgVec& responses){}
	virtual void OnRequests(RTSPMessage::MsgVec& requests){}


private:
	enum ParseState {
		STATE_INIT,
		STATE_HEADERS,
		STATE_BODY,
		STATE_COMPLETE
	};

	typedef struct rtsp_parser_msg
	{
		std::string startLine;
		bool		headerCompleted;
		uint64      contentBodyRead;
		RTSPMessage::Ptr pMsg;

		rtsp_parser_msg() { reset(); }

		void reset()
		{
			headerCompleted = false;
			contentBodyRead = 0;
			pMsg = new RTSPMessage();
		}

	} RTSP_PARSER_MSG;


	void parse(ssize_t bytesRead);

	static std::string trim(char const* str);
	static char* nextLine(char* startOfLine, int maxByte); // this func may change the input chars of startOfLine

private:
	RTSP_PARSER_MSG	  _currentParseMsg;
	int	             _byteSeen;
};







} }//namespace ZQ::eloop
#endif
#ifndef __asynchttp_httpparser_header_file_h__
#define __asynchttp_httpparser_header_file_h__

#include <sstream>
#include <string>
#include <map>
#include <Pointer.h>
#include <Locks.h>
#include "http_parser.h"

namespace LibAsync {
	
	class HttpMessage;
	typedef ZQ::common::Pointer<HttpMessage> HttpMessagePtr;

	class ZQ_COMMON_API HttpMessage : public ZQ::common::SharedObject{
	private:
		ZQ::common::Mutex	mLocker;
	public:
		HttpMessage(http_parser_type type);
		virtual ~HttpMessage();

		typedef HttpMessagePtr	Ptr;

		static const std::string& code2status(int code);
		static std::string httpdate( int deltaInSecond = 0 );

		http_method method() const;
		void		method(http_method mtd);

		const std::string& url() const;
		void		url(const std::string& url);

		void	code( int c);	//set status code
		int		code() const;	//get status cde

		void	status( const std::string& s);	// set status description
		const std::string& status() const;		//get status description

		const std::string& header( const std::string& key) const;

		template<typename T>
		void	header( const std::string& key, const T& value){
			std::ostringstream oss;
			oss<<value;
			ZQ::common::MutexGuard gd(mLocker);
			mHeaders[key] = oss.str();
		}
		void	eraseHeader( const std::string& key );
		
		bool	keepAlive() const;	//check if keepAlive is set
		void	keepAlive( bool b);	//enable/disable keepAlive

		bool	chunked() const;	//check if chunked is set
		void	chunked(bool b);	//enable/disable chunked transfer-encoding,
									//enable chunked also disable Content-Length

		int64	contentLength() const;	//get Content-Length, 0 if chunked is enabled
		void	contentLength(int64 length); //set content-length, this behaviour supress chunked

		bool	hasContentBody( ) const {
			return chunked() || contentLength() > 0;
		}

		std::string toRaw(); // generate raw http message, only start line and headers are generated

		struct caseInsensativeCmp{
			bool operator()( const std::string& lhr, const std::string& rhs) const {
#ifdef ZQ_OS_LINUX
#	define stricmp strcasecmp
#endif
				return stricmp(lhr.c_str(),rhs.c_str()) < 0;
			}
		};
		typedef std::map<std::string,std::string,caseInsensativeCmp> HEADERS;

		inline unsigned int versionMajor() const { return mVerMajor; }
		inline unsigned int versionMinor() const { return mVerMinor; }

		void setVersion( unsigned int major, unsigned int minor) {
			mVerMajor = major;
			mVerMinor = minor;
		}

	private:
		friend class HttpParser;

		
		http_parser_type	mType;//request or response
		http_method			mMethod;
		std::string			mUri;
		std::string			mStatus;
		int					mCode;//status code
		bool				mbChunked;
		bool				mbKeepAlive;
		
		unsigned int		mVerMajor;
		unsigned int		mVerMinor;
		HEADERS				mHeaders;
		std::string			mDummyHeaderValue;
		int64				mBodyLength;//only valid if mbChunked == false
		std::string 		mRawMessage;
	};

	/*
	* Inhereit ParserCallback so that you can get the result of http message parsing
	*/
	class ParserCallback{
	public:
		virtual ~ParserCallback(){}

		/// this method is invoked when http header is completely parsed
		/// You can get uri, method or code, header field and header value in HttpMessage
		/// return true if you don't want to interrupt the parsing procedure
		virtual bool onHttpMessage( const HttpMessagePtr msg) = 0;

		/// this method is invoked when http body data is received and decoded
		/// NOTE: this method may be called many times for one data buffer,
		///		  So don' take this as the finish of the usage of you buffer passed into HttpClient
		/// return true if you don't want to interrupt the parsing procedure
		virtual bool onHttpBody( const char* data, size_t size) = 0;

		/// the whole http message is decoded, that is no more data for current http message
		virtual void onHttpComplete() = 0;

		/// error occured during data receiving or parsing stage
		virtual void onHttpError( int error ) = 0;
	};

	class HttpParser
	{
	public:
		HttpParser(http_parser_type type);
		~HttpParser(void);

		enum ParserState {
			STATE_INIT,
			STATE_HEADERS,
			STATE_BODY,
			STATE_COMPLETE
		};
		
		/// reset parser to initialized stage
		void	reset( ParserCallback* cb );

		///return value == size means successfully parsed,
		/// or else, error occurred
		size_t	parse( const char* data, size_t size);

		/// stop current parsing procedure
		bool	stopParsing() {
			mStopped = true;
			return true;
		}

		/// check if headers are all parsed
		bool	headerComplete() const { 
			return mParserState > STATE_HEADERS;
		}
		bool	httpComplete() const {
			return mParserState >= STATE_COMPLETE;
		}
		ParserState		state() const {
			return mParserState;
		}

		int		lastError() const;

		HttpMessagePtr	currentHttpMessage() {
			return mMessage;
		}
	private:
		static int on_message_begin(http_parser* parser);
		static int on_headers_complete(http_parser* parser);
		static int on_message_complete(http_parser* parser);
		static int on_uri(http_parser* parser, const char* at, size_t size);
		static int on_status(http_parser* parser, const char* at, size_t size);
		static int on_header_field(http_parser* parser, const char* at, size_t size);
		static int on_header_value(http_parser* parser, const char* at, size_t size);
		static int on_body(http_parser* parser, const char* at, size_t size);

		int		onMessageBegin( );
		int		onHeadersComplete();
		int		onMessageComplete();
		int		onUri(const char* at, size_t size);
		int		onStatus(const char* at, size_t size);
		int		onHeaderField(const char* at, size_t size);
		int		onHeaderValue(const char* at, size_t size);
		int		onBody(const char* at, size_t size);

	private:
		http_parser			mParser;
		http_parser_type	mType;
		HttpMessagePtr		mMessage;
		bool				mStopped;
		ParserCallback*		mCallback;

		std::string			mHeaderField;
		std::string*		mHeaderValue;
		http_parser_settings	mParserSettings;
		ParserState			mParserState;
	};

}//namespace LibAsync

#endif//__asynchttp_httpparser_header_file_h__
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312

#ifndef __SERVERAGENT__h__
#define __SERVERAGENT__h__

#include "HttpServer.h"
#include "FileLog.h"
#include "snmp/ZQSnmp.h"

using namespace ZQ::SNMP;
namespace ZQ {
namespace eloop {

	// ---------------------------------------
	// class LoadFile
	// ---------------------------------------
	class LoadFile : public HttpHandler
	{
	public:
		typedef HttpApplication<LoadFile> App;

		LoadFile(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps);
		~LoadFile();

		void Response(){}
		void ResponseIndex(){}

		virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
		virtual bool onBodyData( const char* data, size_t size){return true;}
		virtual void onMessageCompleted(){}
		virtual void onParseError( int error,const char* errorDescription ){}

		//virtual void	onHttpDataSent(bool keepAlive){}

		virtual void	onHttpDataSent();
		virtual void	onHttpDataReceived( size_t size ){}
		virtual void    onError(int error,const char* errorDescription){}
		/*用来设置主页所在路径，相关的js和控制代码的路径，一般设置好后不会修改，除非主页及js控制代码的位置发生变更
		*/
		void setHomeDir(std::string homedir){
			_homedir = homedir;
		}
		/*用来设置额外资源文件所在路径，近期的csv文件所在路径可以通过这个设置
		*/
		void setSourceDir(std::string sourcedir){
			_sourcedir = sourcedir;
		}
	private:
		/*
		这两个路径的优先顺序，先是在_homedir中查找，如果查找不到，则会再在_sourcedir路径查找，否则出错
		*/
		std::string     _homedir;
		std::string     _sourcedir;
		FILE*           _curfile;
	};

	// ---------------------------------------
	// class ServerAgent
	// ---------------------------------------
	class ServerAgent : public HttpHandler 
	{
	public:
		typedef HttpApplication<ServerAgent> App;

		ServerAgent(HttpConnection& conn,ZQ::common::Log& logger,const Properties& dirProps, const Properties& appProps);
		~ServerAgent();

		void Response(){}
		void ResponseIndex(){}

		virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
		virtual bool onBodyData( const char* data, size_t size){return true;}
		virtual void onMessageCompleted(){}
		virtual void onParseError( int error,const char* errorDescription ){}


		virtual void	onHttpDataSent(){}

		virtual void	onHttpDataReceived( size_t size ){}
		virtual void    onError(int error,const char* errorDescription){}
	private:
		SnmpAgent*       _ag;
	};

} }//namespace ZQ::eloop
#endif

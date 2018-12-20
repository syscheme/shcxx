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

		LoadFile(IBaseApplication& app, HttpPassiveConn& conn, const HttpHandler::Properties& dirProps = HttpHandler::Properties());
		~LoadFile();

		void Response(){}
		void ResponseIndex(){}

		virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
		virtual bool onBodyData( const char* data, size_t size){return true;}
		virtual void onMessageCompleted(){}
		virtual void onParseError( int error,const char* errorDescription ){}

		//virtual void	onHttpDataSent(bool keepAlive){}

		virtual void	onHttpDataSent(size_t size);
		virtual void	onHttpDataReceived( size_t size ){}
		virtual void    onError(int error,const char* errorDescription){}
		/*����������ҳ����·������ص�js�Ϳ��ƴ����·����һ�����úú󲻻��޸ģ�������ҳ��js���ƴ����λ�÷������
		*/
		void setHomeDir(std::string homedir){
			_homedir = homedir;
		}
		/*�������ö�����Դ�ļ�����·�������ڵ�csv�ļ�����·������ͨ���������
		*/
		void setSourceDir(std::string sourcedir){
			_sourcedir = sourcedir;
		}
	private:
		/*
		������·��������˳��������_homedir�в��ң�������Ҳ������������_sourcedir·�����ң��������
		*/
		std::string     _homedir;
		std::string     _sourcedir;
		FILE*           _curfile;
		char*			_buf;
	};

	// ---------------------------------------
	// class ServerAgent
	// ---------------------------------------
	class ServerAgent : public HttpHandler 
	{
	public:
		typedef HttpApplication<ServerAgent> App;

		ServerAgent(IBaseApplication& app, HttpPassiveConn& conn, const HttpHandler::Properties& dirProps = HttpHandler::Properties());
		~ServerAgent();

		void Response(){}
		void ResponseIndex(){}

		virtual bool onHeadersEnd( const HttpMessage::Ptr msg);
		virtual bool onBodyData( const char* data, size_t size){return true;}
		virtual void onMessageCompleted(){}
		virtual void onParseError( int error,const char* errorDescription ){}


		virtual void	onHttpDataSent(size_t size){}

		virtual void	onHttpDataReceived( size_t size ){}
		virtual void    onError(int error,const char* errorDescription){}
	private:
		SnmpAgent*       _ag;
	};

} }//namespace ZQ::eloop
#endif

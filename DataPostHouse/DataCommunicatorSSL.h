// File Name: DataCommunicatorSSL.h
// Date: 2009-02
// Description: Definition of SSL server 

#ifndef __DATA_COMMUNICATOR_SSL_H__
#define __DATA_COMMUNICATOR_SSL_H__

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include "DataCommunicatorUnite.h"

namespace ZQ
{
namespace DataPostHouse
{
	/// Load SSL library
	void initSSLLibrary();

	class IDataCommunicatorSSL : public ASocket
	{
	public:
		IDataCommunicatorSSL(DataPostHouseEnv& env, DataPostDak& dak);
		IDataCommunicatorSSL(DataPostHouseEnv& env, DataPostDak& dak, const SOCKET& s, SSL_CTX* ctx,
			const CommunicatorType& type = COMM_TYPE_SSL, SharedObjectPtr userData = NULL);
		virtual ~IDataCommunicatorSSL();
	public:
		virtual	int32 read(int8* buffer, size_t bufSize, int32 timeout = -1);
		virtual	int32 write(const int8* buffer, size_t bufSize, int32 timeout = -1);
		virtual	int32 writeAsync(const int8* buffer, size_t bufSize);

#ifdef ZQ_OS_MSWIN
		virtual	int32 onDataAsyncResult(int32 size, LPOVERLAPPED overlap);
#else
		virtual	int32 onDataAsyncResult(struct ::epoll_event& epollevent);
#endif

	public:
		bool initial(IDataCommunicatorSSL* pSocket, bool bCheckClientCertificae = false);

		/// get peer address
		void getPeerAddress(std::string& strPeerAddress, unsigned short& sPeerPort);
	private:
		/// forbidden copy or assign
		IDataCommunicatorSSL(const IDataCommunicatorSSL& oriSSL);
		IDataCommunicatorSSL& operator=(const IDataCommunicatorSSL& oriSSL);
		
		/// verify client's certificate
		bool checkClientCertificate();

		///create SSL and bind it with TCP socket
		bool initialBeforeAccept();

		///allocate resource to SSL connection and change it's underlying BIO to memory BIO
		bool initialAfterAccept();
	protected:
		SSL_CTX* _ctx;
		bool _bCheckClientCertificate;
	private:
		SSL* _ssl;
		BIO* _readBio;
		BIO* _writeBio;
		char* _encryptDatas;
	};
	typedef ObjectHandle<IDataCommunicatorSSL> IDataCommunicatorSSLPtr;

	class SSLServer : public IDataCommunicatorSSL, public ZQ::common::NativeThread
	{
	public:
		SSLServer(DataPostDak& dak, DataPostHouseEnv& env);
		~SSLServer();
	public:
		bool startServer(const std::string strLocalIp, const std::string strLocalPort, 
				SharedObjectPtr userData = NULL, int32 maxBacklog = 100);
		void stop();

		/// this method should be call before startServer
		void setCertAndKeyFile(const std::string strCertFile, const std::string strKeyFile, const std::string strCertPasswd_);
		void setCheckClientCertificate();

	protected:
		int run(void);
		bool initialize(void);

	private:
		IDataCommunicatorSSL* accept(bool& bExit);
	private:
		DataPostDak& _postDak;
		std::string _strCertificateFile;
		std::string _strPrivatekeyFile;
	};
	typedef ObjectHandle<SSLServer> SSLServerPtr;
}
}

#endif


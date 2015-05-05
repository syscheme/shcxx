
#include "DataCommunicatorSSL.h"
extern "C"
{
#include <arpa/inet.h>
}

namespace ZQ
{
namespace DataPostHouse
{

#define MLOG mEnv.getLogger()
#define COMMFMT(x,y) "COMM[%08llx]TID[%08X][%10s]\t"#y,mId,pthread_self(),#x

std::string strCertificatePasswd = "seaChange";

/// load SSL library

void initSSLLibrary()
{
	static bool inited = false;
	if (inited)
	{
		return;
	}
	SSL_load_error_strings();
	SSL_library_init();
	ERR_load_BIO_strings();
	inited = true;
}

///If need change ceritficate's encrypt password, call this method to change before start server
void setCertificatePasswd(const std::string strCertificatePasswd_)
{
	strCertificatePasswd = strCertificatePasswd_;
}

///set certificate password
/// maybe it isn't work
int password_callback(char* buffer, int nSize, int nrwflag, void* userdata)
{
	strcpy(buffer, strCertificatePasswd.c_str());
	return 1;
}

IDataCommunicatorSSL::IDataCommunicatorSSL(DataPostHouseEnv &env,DataPostDak& dak)
:ASocket(env,dak), _ctx(NULL), _bCheckClientCertificate(false),
_ssl(NULL), _readBio(NULL), _writeBio(NULL), _encryptDatas(NULL)
{
	mType = COMM_TYPE_SSL; 
}

IDataCommunicatorSSL::IDataCommunicatorSSL(DataPostHouseEnv& env, DataPostDak& dak, const SOCKET& s, SSL_CTX* ctx, 
										   const CommunicatorType& type, SharedObjectPtr userData)
:ASocket(env, dak, s, type, userData), _ctx(ctx), _bCheckClientCertificate(false),
_ssl(NULL),_readBio(NULL), _writeBio(NULL), _encryptDatas(NULL)
{
	mType = COMM_TYPE_SSL;
}

IDataCommunicatorSSL::~IDataCommunicatorSSL()
{
	if (_ssl != NULL)
	{
		SSL_free(_ssl);
	}
	delete [] _encryptDatas;
	MLOG(ZQ::common::Log::L_DEBUG, COMMFMT(~IDataCommunicatorSSL, "Close SSL Connection"));
}

bool IDataCommunicatorSSL::initial(IDataCommunicatorSSL* pSocket, bool bCheckClientCertificae)
{
	_bCheckClientCertificate = bCheckClientCertificae; // if need check client's certificate

	if (!initialBeforeAccept())
	{
		return false;
	}
	if (SSL_accept(_ssl) < 1)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(initial, "Failed to hand shake with client"));
		return false;
	}
	if (!initialAfterAccept())
	{
		return false;
	}
	mCompletionKey.dataCommunicator = pSocket;
	mCompletionKey.mStatus = true;
	return true;
}

bool IDataCommunicatorSSL::initialBeforeAccept()
{
	if (NULL == _ctx || -1 == mSock)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(initialBeforeAccept, "Unexcepted Errors : CTX is NULL or Socket is invalide"));
		return false;
	}
	_ssl = SSL_new(_ctx);
	if (NULL == _ssl)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(initialBeforeAccept, "Failed to create SSL"));
		return false;
	}
	SSL_set_mode(_ssl, SSL_MODE_AUTO_RETRY);
	if (0 == SSL_set_fd(_ssl, static_cast<int>(mSock)))
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(initialBeforeAccept, "Failed to bind SSL with socket"));
		return false;
	}
	SSL_in_init(_ssl);  // function?
	return true;
}

bool IDataCommunicatorSSL::checkClientCertificate()
{
	if (NULL == _ssl)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(checkClientCertificate, "SSL connection isn't established"));
		return false;
	}
	char* str = NULL;
	X509* client_cert = SSL_get_peer_certificate(_ssl);
	if (client_cert != NULL)
	{
		if (SSL_get_verify_result(_ssl) != X509_V_OK)
		{
			MLOG(ZQ::common::Log::L_ERROR, COMMFMT(checkClientCertificate, "Fail to verify client's ceritficate"));
			return false;
		}
		str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
		if (NULL == str)
		{
			MLOG(ZQ::common::Log::L_ERROR, COMMFMT(checkClientCertificate, "Fail to get peer certificate's subject name"));
			return false;
		}
		OPENSSL_free(str);
		str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
		if (NULL == str)
		{
			MLOG(ZQ::common::Log::L_ERROR, COMMFMT(checkClientCertificate, "Fail to get peer certificate's issuser name"));
			return false;
		}
		OPENSSL_free(str);
		X509_free(client_cert);
		return true;
	}
	MLOG(ZQ::common::Log::L_ERROR, COMMFMT(checkClientCertificate, "Can't get peer's certificate"));
	return false;
}

bool IDataCommunicatorSSL::initialAfterAccept()
{
	if (_bCheckClientCertificate && (!checkClientCertificate()))
	{
		return false;
	}
	// allocate other resource after ssl accept
	_encryptDatas = new char[mEnv.mEncryptBufferSize];
	if (NULL == _encryptDatas)
	{
		return false;
	}
	_readBio = BIO_new(BIO_s_mem());
	_writeBio = BIO_new(BIO_s_mem());
	if (NULL == _writeBio || NULL == _readBio)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(initialAfterAccept, "Failed to create memory BIO"));
		return false;
	}
	SSL_set_bio(_ssl, _readBio, _writeBio);
	initializeSockName();
	return true;
}

int32 IDataCommunicatorSSL::read(int8* buffer, size_t bufSize, int32 timeout)
{
	if (NULL == _ssl || NULL == _readBio || NULL == buffer || bufSize <= 0)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(read, "Unexcepted Error"));
		return ERROR_CODE_OPERATION_FAIL;
	}
	int nRead = 0;
	int nReceive = 0;
	int nEncryptRead = 0;
	while (nReceive < (int)bufSize)
	{
		nRead = SSL_read(_ssl, buffer + nReceive, bufSize -nReceive);
		if (nRead <= 0)
		{
			nEncryptRead = ASocket::read(_encryptDatas, mEnv.mEncryptBufferSize, timeout);
			
			if (nEncryptRead < 0)
			{
				if (ERROR_CODE_OPERATION_TIMEOUT == nEncryptRead)
				{
					break;
				}
				return nEncryptRead;
			}
			BIO_write(_readBio, _encryptDatas, nEncryptRead);
		}
		else
		{
			nReceive += nRead;
		}
	}
	return static_cast<int32>(nReceive);
}

int32 IDataCommunicatorSSL::write(const int8* buffer, size_t bufSize, int32 timeout)
{
	if (NULL == _ssl || NULL == _writeBio || NULL == buffer || bufSize <= 0)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(write, "Unexcepted Error"));
		return ERROR_CODE_OPERATION_FAIL;
	}
	int nWrite = SSL_write(_ssl, buffer, bufSize);
	if (nWrite <= 0)
	{
		return ERROR_CODE_OPERATION_FAIL;
	}
	char* encryptBuffer = NULL;
	int nEncryptWrite = BIO_get_mem_data(_writeBio, &encryptBuffer);
	int nSend = ASocket::write(encryptBuffer, nEncryptWrite, timeout);
	if (nSend < 0)
	{
		return nSend;
	}
	return nWrite;
}

int32 IDataCommunicatorSSL::writeAsync(const int8* buffer, size_t bufSize)
{
	if (NULL == _ssl || NULL == _writeBio || NULL == buffer || bufSize <= 0)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(writeAsync, "Unexcepted Error"));
		return ERROR_CODE_OPERATION_FAIL;
	}
	int nWrite = SSL_write(_ssl, buffer, bufSize);
	if (nWrite <= 0)
	{
		return ERROR_CODE_OPERATION_FAIL;
	}
	char* encryptBuffer = NULL;
	int nEncryptWrite = BIO_get_mem_data(_writeBio, &encryptBuffer);
	int nSend = ASocket::writeAsync(encryptBuffer, nEncryptWrite);
	if (nSend < 0)
	{
		return nSend;
	}
	return nWrite;
}

int32 IDataCommunicatorSSL::onDataAsyncResult(struct ::epoll_event& epollevent)
{
	assert(mSock != -1);
	assert(mDataDialog != 0);

#if defined _DEBUG || defined DEBUG
	MLOG(ZQ::common::Log::L_DEBUG,COMMFMT(onDataAsyncResult,"Has event [%d]"),epollevent.events);
#endif

	int nRead = 0;
	if( (epollevent.events & EPOLLIN) == EPOLLIN )//read event
	{
		if(!isValid() )
		{//socket is closed
			printf("error here and socket is not valid\n");
			mEnv.getDataDialogFactory()->releaseDataDialog( mDataDialog , this );
			clear();
			return ERROR_CODE_OPERATION_OK;
		}
		do 
		{
			int32 iRet = readAsync();
			if( iRet > 0 )
			{
				try
				{
					BIO_write(_readBio, mReadBuf, iRet);
					while (BIO_ctrl_pending(_readBio) > 0)
					{
						nRead = SSL_read(_ssl, mReadBuf, mReadBufLen);
						if (nRead < 0)
						{
							MLOG(ZQ::common::Log::L_ERROR, COMMFMT(onDataAsyncResult,
									   	"Failed to read datas from ssl. Error code[%d]. Error reason[%s]"), 
								SSL_get_error(_ssl, nRead), ERR_reason_error_string(ERR_get_error()));
							break;
						}
						if (0 == nRead)
						{
							MLOG(ZQ::common::Log::L_ERROR, COMMFMT(onDataAsyncResult, "SSL Connection is closed"));
							break;
						}
						mDataDialog->onRead(mReadBuf, static_cast<size_t>(nRead));
					}
				}
				catch(...)
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,
								"unknown exception when call dialog's onRead()"));			
				}
			}
			else
			{
				if ( iRet == ERROR_CODE_OPERATION_PENDING || iRet == ERROR_CODE_OPERATION_CLOSED )
				{
					return ERROR_CODE_OPERATION_OK;
				}
				else
				{
					MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"Failed to invoke readAsync"));
					mEnv.getDataDialogFactory()->releaseDataDialog( mDataDialog , this );
					clear();					
					return ERROR_CODE_OPERATION_FAIL;
				}
			}

		} while (true);
	}
	else if( (epollevent.events & EPOLLOUT) == EPOLLOUT) //write event
	{
		int32 size = 0;
		if(mWriteBuf.size())
		{
			size = writeAsync(mWriteBuf.c_str(),mWriteBuf.length());
			if(size <= 0)
			{
				MLOG(ZQ::common::Log::L_ERROR,"onDataAsyncResult() writeAsync failed error string[%s]",strerror(errno));
				mEnv.getDataDialogFactory()->releaseDataDialog( mDataDialog , this );
				clear();
				size = 0;
			}
		}
		try
		{
			mDataDialog->onWritten( static_cast<size_t>(size) );
		}
		catch(...)
		{
			MLOG(ZQ::common::Log::L_ERROR,COMMFMT(onDataAsyncResult,"unknown exception when invoke dialog's onWritten"));
			return ERROR_CODE_OPERATION_FAIL;
		}
		return ERROR_CODE_OPERATION_OK;
	}
	else if((epollevent.events & EPOLLHUP) == EPOLLHUP || (epollevent.events & EPOLLERR) == EPOLLERR)
	{
		MLOG(ZQ::common::Log::L_INFO,"onDataAsyncResult() have a hang up or error event");
		mEnv.getDataDialogFactory()->releaseDataDialog( mDataDialog , this );
		clear();
		return ERROR_CODE_OPERATION_FAIL;

	}
	else
	{
		MLOG(ZQ::common::Log::L_ERROR,"onDataSsyncResult() dont known the event type[%d]",epollevent.events);
		mEnv.getDataDialogFactory()->releaseDataDialog( mDataDialog , this );
		clear();
		return ERROR_CODE_OPERATION_FAIL;
	}


}

void IDataCommunicatorSSL::getPeerAddress(std::string& strPeerAddress, unsigned short& sPeerPort)
{
	if (-1 == mSock)
	{
		strPeerAddress = "";
		sPeerPort = 0;
	}
	sockaddr_in peerAddr;
	size_t nAddrLen = sizeof(peerAddr);
	getpeername(mSock, (struct sockaddr*)(&peerAddr), (socklen_t*)(&nAddrLen));
	char* addr = inet_ntoa(peerAddr.sin_addr);
	strPeerAddress = addr;
	sPeerPort = ntohs(peerAddr.sin_port);
}

SSLServer::SSLServer(ZQ::DataPostHouse::DataPostDak &dak, ZQ::DataPostHouse::DataPostHouseEnv &env)
:IDataCommunicatorSSL(env,dak), _postDak(dak), _strCertificateFile(""), _strPrivatekeyFile("")
{
	initSSLLibrary();
}

SSLServer::~SSLServer()
{
	stop();
}

void SSLServer::setCertAndKeyFile(const std::string strCertFile, const std::string strKeyFile, const std::string strCertPasswd_)
{
	_strCertificateFile = strCertFile;
	_strPrivatekeyFile = strKeyFile;
	strCertificatePasswd = strCertPasswd_;
}

bool SSLServer::initialize()
{
	// create ctx
	_ctx = SSL_CTX_new(SSLv23_server_method());
	if (NULL == _ctx)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(init, "Fail to create SSL context"));
		return false;
	}

	// load certificate
	SSL_CTX_set_default_passwd_cb(_ctx, password_callback);
	if (!SSL_CTX_use_certificate_file(_ctx, _strCertificateFile.c_str(), SSL_FILETYPE_PEM))
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(init, "Fail to load certificate"));
		SSL_CTX_free(_ctx);
		_ctx = NULL;
		return false;
	}
	if (!SSL_CTX_use_PrivateKey_file(_ctx, _strPrivatekeyFile.c_str(), SSL_FILETYPE_PEM))
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(init, "Fail to load private key"));
		SSL_CTX_free(_ctx);
		_ctx = NULL;
		return false;
	}
	if (!SSL_CTX_check_private_key(_ctx))
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(init, "Private key does not match the certificate public key"));
		SSL_CTX_free(_ctx);
		_ctx = NULL;
		return false;
	}
	//setCheckClientCertificate();
	MLOG(ZQ::common::Log::L_DEBUG, COMMFMT(init, "Success to load SSL CTX enviroment"));
	return true;
}

bool SSLServer::startServer(const std::string strLocalIp, const std::string strLocalPort, SharedObjectPtr userData, int32 maxBacklog)
{
	int family = (strLocalIp.find(":") != std::string::npos) ? AF_INET6 :AF_INET;
	mAddrInfoHelper.init(family, SOCK_STREAM, IPPROTO_TCP);
	//create socket and bind it
	if( !mAddrInfoHelper.convert(strLocalIp, strLocalPort))
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(startServer, "can't get address information with ip[%s] port[%s] and error string[%s]"),
			strLocalIp.c_str(), strLocalPort.c_str(), strerror(errno));
		return false;
	}
	addrinfo* adInfo = mAddrInfoHelper.getAddrInfo();
	assert(adInfo != NULL);
	if (!createSocket(adInfo->ai_family, SOCK_STREAM, IPPROTO_TCP))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer, "create server socket failed and errorCode [%s]"), 
			strerror(errno));
		return false;
	}

	int breuse = true;
	int re = setsockopt(mSock,SOL_SOCKET,SO_REUSEADDR,(void*)&breuse, sizeof(breuse));
	if(re == -1)
	{
		MLOG(ZQ::common::Log::L_WARNING,COMMFMT(startServer,"set reuse address is failed code[%d] string[%s]"),
		errno,strerror(errno)); 
	}

	if(!bind(strLocalIp, strLocalPort))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer, "failed to bind local address[%s][%s] and errorCode[%s]"),
			strLocalIp.c_str() , strLocalPort.c_str(), strerror(errno) );
		return false;
	}
	if(!listen(maxBacklog))
	{
		MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer, "failed to listen and errorCode[%s]"), strerror(errno));
		return false;
	}
	MLOG(ZQ::common::Log::L_DEBUG, COMMFMT(startServer, "SSL Server is listening at [%s:%s]"), strLocalIp.c_str(), strLocalPort.c_str());

	if (!initialize())
	{
		//MLOG(ZQ::common::Log::L_ERROR,COMMFMT(startServer, "failed to load ssl en"));
		return false;
	}

	mUserData = userData;
	return start();
}

int SSLServer::run()
{
	bool bExit = false;
	while (true)
	{
		IDataCommunicatorSSL* pSocket = accept(bExit);
		if (NULL == pSocket)
		{
			if (bExit)
			{
				MLOG(ZQ::common::Log::L_DEBUG, COMMFMT(run, "SSL server is terminated"));
				break;
			}
			continue;
		}

		IDataCommunicatorExPtr pComm = pSocket;
		pSocket->attachUserData(mUserData);
		IDataDialogPtr dialog = mEnv.getDataDialogFactory()->createDataDialog(pComm);			
		if(dialog)
		{			
			pSocket->attchDialog(dialog);
			dialog->onCommunicatorSetup(pComm);
			if(!_postDak.addnewCommunicator(pComm)) //failed to add communicator to Dak
			{
				pComm->onCommunicatorClosed();
				continue;
			}
/*
			int32 iRet = pSocket->readAsync();
			if( iRet == ERROR_CODE_OPERATION_CLOSED )
			{
				pComm->onCommunicatorClosed();					
			}
			else if (iRet < 0 )
			{
				if ( iRet != ERROR_CODE_OPERATION_PENDING )
				{
					onDataAsyncError();
				}					
			}
*/
		}
		else
		{
			pComm->close();
		}

	}
	return 1;
}

IDataCommunicatorSSL* SSLServer::accept(bool& bExit)
{
	if (-1 == mSock)
	{
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(accept, "Unexcepted Error"));
		return NULL;
	}
	sockaddr_storage addr;
	int addLen = sizeof(addr);
	int s = ::accept(mSock, (sockaddr*)&addr,(socklen_t*) &addLen);
	if(s == -1)
	{
		bExit = true; //listen socket is closed
		MLOG(ZQ::common::Log::L_ERROR, COMMFMT(accept, "accept failed and errorCode[%s]"), strerror(errno));
		return NULL;
	}
	else
	{
		IDataCommunicatorSSL* pSocket = new IDataCommunicatorSSL(mEnv, mDak, s, _ctx);
		if (NULL == pSocket)
		{
			::close(s);
			MLOG(ZQ::common::Log::L_ERROR, COMMFMT(accept, "Fail to create SSL Communicator"));
			return NULL;
		}
		if (!pSocket->initial(pSocket, _bCheckClientCertificate))
		{
			::close(s);
			MLOG(ZQ::common::Log::L_ERROR, COMMFMT(accept, "Fail to initial SSL Communicator"));
			return NULL;
		}
		// get peer address to log 
		std::string strPeerAddress = "";
		unsigned short sPeerPort = 0;
		pSocket->getPeerAddress(strPeerAddress, sPeerPort);
		MLOG(ZQ::common::Log::L_DEBUG, COMMFMT(accept, "Establish SSL connection with [%s:%d]"),
			 strPeerAddress.c_str(), sPeerPort);
		return pSocket;
	}
}

void SSLServer::stop()
{
	close();
	clear();
	if (_ctx != NULL)
	{
		SSL_CTX_free(_ctx);
		_ctx = NULL;
	}
	MLOG(ZQ::common::Log::L_DEBUG, COMMFMT(stop,"Success to unload SSL enviroment"));
}

void SSLServer::setCheckClientCertificate()
{
	SSL_CTX_set_verify(_ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, NULL);
	_bCheckClientCertificate = true;
}


}}


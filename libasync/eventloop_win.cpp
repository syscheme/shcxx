#include "eventloop.h"
#include "Socket.h"
#include <WinSock2.h>
#include <MSWSock.h>

#include "TimeUtil.h"

namespace LibAsync {

	void EventLoop::createLoop()
	{
		mhIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
		if (NULL == mhIOCP)
		{
			//create iocp failure
			abort();
		}
	}

	void EventLoop::destroyLoop()
	{
		if (NULL != mhIOCP)
		{
			CloseHandle(mhIOCP);
			mhIOCP = NULL;
		}
	}

	void EventLoop::wakeupLoop()
	{
		unsigned long key = 0;
		PostQueuedCompletionStatus(mhIOCP, 0, key, NULL);
	}

	bool EventLoop::addSocket(Socket::Ptr sock)
	{
		mhIOCP = CreateIoCompletionPort((HANDLE)sock->mSocket, mhIOCP, (ULONG_PTR)(sock.get()), 0);
		return NULL != mhIOCP;
	}

	void EventLoop::processEvent(int64 expireAt)
	{
		PIOCP_OVERLAPPED	pIO = NULL;
		DWORD dwByteTransfered = 0;
		ULONG_PTR key = -1;

		if (!mhIOCP)
		{
			return;
		}

		int64	timeout = expireAt - ZQ::common::TimeUtil::now();
		timeout = (timeout < 0) ? 0 : timeout;

		bool bRet = GetQueuedCompletionStatus(mhIOCP, &dwByteTransfered, &key, (LPOVERLAPPED*)&pIO, timeout);

		if (!bRet)
		{
			int errCode = WSAGetLastError();

            if (ERROR_INVALID_HANDLE == errCode)
            {
                //IOCP handle被关闭
                return;
            }

			if (WSAENOTCONN == errCode)
			{
				//socket未连接，或未提供远端地址，或远端地址不可用
				return;
			}

			if (ERROR_NETNAME_DELETED == errCode)
			{
				//远端关闭连接
				return;
			}

			if (ERROR_CONNECTION_ABORTED == errCode || ERROR_OPERATION_ABORTED == errCode)
			{
				//本地断开连接
				return;
			}

			if (NULL == pIO)
			{//if there is not timeout, then iocp handle closed
				
				if (WAIT_TIMEOUT == errCode)
				{
					//timeout
					return;
				}
			}else{
				Socket* sock  = (Socket*)key;
				Socket::Ptr sockPtr = sock;
				if (NULL == sock)
				{
					goto err;
				}

				if (INVALID_SOCKET == sockPtr->mSocket)
				{
                    sockPtr->mLastError = LibAsync::ERR_EOF;
					sockPtr->onSocketError(LibAsync::ERR_EOF);

					goto err;
				}
			}		

		err:
			if (pIO)
			{
				if (pIO->buf)
				{
					delete pIO->buf;
				}
				delete pIO;
			}
			return;
		}

		if (0 == key)	//wakeup
			return;

		//GetQueuedCompletionStatus success
		Socket::Ptr sockPtr = (Socket*)key;
		SOCKET sock = sockPtr->mSocket;
		switch(pIO->opType)
		{
		case OP_CONNECT:
			{
                if (sockPtr->isOpened())
                {
                    int seconds;
                    int bytes = sizeof(seconds);

                    int iResult = getsockopt( sock, SOL_SOCKET, SO_CONNECT_TIME, (char *)&seconds, (PINT)&bytes );
                    if ( iResult != NO_ERROR ) {
                        //connect failure
                        sockPtr->mbAlive = false;
                        sockPtr->onSocketError(LibAsync::ERR_CONNREFUSED);
                    }
                    else {
                        if (seconds == 0xFFFFFFFF)	//connect failure
                        {
                            sockPtr->onSocketError(LibAsync::ERR_CONNREFUSED);
                        }
                        else{
                            sockPtr->mbAlive = true;
                            sockPtr->mSendValid = true;
                            sockPtr->onSocketConnected();
                        }
                    }
                }

                pIO->sockRef = NULL;
                delete pIO;
				break;
			}
		case OP_ACCEPT:
			{
                if (sockPtr->isOpened())
                {
                    int seconds;
                    int bytes = sizeof(seconds);
                    SOCKET acceptSock = sockPtr->mAcceptSocket;
                    int iResult = getsockopt( acceptSock, SOL_SOCKET, SO_CONNECT_TIME, (char *)&seconds, (PINT)&bytes );
                    if ( iResult != NO_ERROR ) {
                        //connect failure
                        int err = WSAGetLastError();
                        sockPtr->mbAlive = false;
                        sockPtr->onSocketError(LibAsync::ERR_CONNREFUSED);
                    }
                    else {
                        if (seconds == 0xFFFFFFFF)	//connect failure
                        {
                            sockPtr->onSocketError(LibAsync::ERR_CONNREFUSED);
                        }
                        else{
                            sockPtr->mbAlive = true;
                            sockPtr->mSendValid = true;
                            sockPtr->mRecValid	= true;
                            SocketPtr acceptPtr = sockPtr->onSocketAccepted(acceptSock);
                            acceptPtr->initialServerSocket();
                            if (!sockPtr->innerAccept()){
                                sockPtr->onSocketError(LibAsync::ERR_ERROR);
                            }						
                        }
                    }
                }

                pIO->sockRef = NULL;
				delete pIO->buf;
				delete pIO;
				break;
			}
		case OP_SEND:
            if (sockPtr->isOpened())
            {
                sockPtr->mSendValid = true;
                sockPtr->mRecValid	= true;
                sockPtr->onSocketSent(dwByteTransfered);
            }

            pIO->sockRef = NULL;
			delete pIO->buf;
			delete pIO;
			break;
		case OP_RECV:
            if (sockPtr->isOpened())
            {
                sockPtr->mRecValid = true;
                if (dwByteTransfered == 0)
                {
                    sockPtr->onSocketError(ERR_EOF);
                }
                else{
                    sockPtr->onSocketRecved(dwByteTransfered);
                }
            }		

            pIO->sockRef = NULL;
			delete pIO->buf;
			delete pIO;
			break;
		default:
			break;
		}

		return;
	}
}
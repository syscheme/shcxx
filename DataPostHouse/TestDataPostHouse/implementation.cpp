
#include <stdio.h>
#include <iostream>
#include "implementation.h"

#include <Locks.h>

static ZQ::common::Mutex outputMutex;

using namespace ZQ::DataPostHouse;

DialogImpl::DialogImpl( )
{

}
DialogImpl::~DialogImpl()
{
}
void DialogImpl::onCommunicatorDestroyed( IDataCommunicatorPtr communicator )
{
	assert(communicator == mComm );
	ZQ::common::MutexGuard gd(outputMutex);
	std::cout<<"dialog with communicator "<<communicator->getCommunicatorId()<<" is destroyed"<<std::endl;
	mComm = NULL;
}

void DialogImpl::onCommunicatorSetup( IDataCommunicatorPtr communicator )
{
	mComm = communicator;
	ZQ::common::MutexGuard gd(outputMutex);
	std::cout<<"dialog with communicator "<<communicator->getCommunicatorId()<<" is setup"<<std::endl;
}

void DialogImpl::onWritten( size_t bufSize )
{
	//std::cout<<"written data "<<bufSize<<std::endl;
}

void DialogImpl::onError()
{
}

DWORD WINAPI threadproc(LPVOID lpPara)
{
	IDataCommunicator* mComm = (IDataCommunicator*)(lpPara);
	Sleep(2000);
	mComm->close();
	return 1;
}

bool DialogImpl::onRead( const int8* buffer , size_t bufSize )
{
	assert( mComm != NULL );
	memcpy( szBuf , buffer , bufSize );
	szBuf[bufSize] = 0;
	ZQ::common::MutexGuard gd(outputMutex);
	std::cout<<"communicator "<< mComm->getCommunicatorId() <<" recevied :"<<(char*)szBuf<<std::endl;
	//mComm->writeAsync( szBuf , bufSize );	
	//CloseHandle( CreateThread(NULL,0,threadproc,mComm.get(),0,0) );
	return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
DialogFactoryImpl::DialogFactoryImpl()
{
}
DialogFactoryImpl::~DialogFactoryImpl()
{	
}

IDataDialogPtr DialogFactoryImpl::onCreateDataDialog( IDataCommunicatorPtr communicator )
{
	IDataDialogPtr pDialog = new DialogImpl();
	assert( pDialog != NULL );	
	return pDialog;
}
void DialogFactoryImpl::onReleaseDataDialog( IDataDialogPtr idalog , IDataCommunicatorPtr communicator) 
{

}

void DialogFactoryImpl::onClose( CommunicatorS& comms )
{
	std::cout<<"communicator "<< comms.size()<<" left"<<std::endl;
	CommunicatorS::iterator it = comms.begin();
	for( ; it != comms.end() ; it ++ )
	{
		(*it)->close();
	}
}

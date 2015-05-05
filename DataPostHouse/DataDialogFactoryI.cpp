
#include "DataCommunicator.h"

namespace ZQ
{
namespace DataPostHouse
{

void IDataDialogFactory::close()
{
	CommunicatorS comms;
	{
		ZQ::common::MutexGuard gd(mCommLocker);
		comms = mComms;
	}
	onClose(comms);
	{
		ZQ::common::MutexGuard gd(mCommLocker);
		mComms.clear();
	}
	comms.clear();
}
IDataDialogPtr IDataDialogFactory::createDataDialog( IDataCommunicatorPtr communicator )
{
	IDataDialogPtr pDialog = onCreateDataDialog(communicator);	
	if (pDialog )
	{
		ZQ::common::MutexGuard gd(mCommLocker);
		
//		CommunicatorS::_Pairib ret = mComms.insert(communicator);
		std::pair<CommunicatorS::iterator,bool> ret = mComms.insert(communicator);
		assert( ret.second );
	}
	return pDialog;
}
void IDataDialogFactory::releaseDataDialog(IDataDialogPtr dialog , IDataCommunicatorPtr communicator)
{
	{
		ZQ::common::MutexGuard gd(mCommLocker);
		if( mComms.find(communicator) == mComms.end() )
			return;
	}
	onReleaseDataDialog( dialog , communicator );
	{
		ZQ::common::MutexGuard gd(mCommLocker);
		mComms.erase(communicator);
	}
}



}}//namespace ZQ::DataPostHouse

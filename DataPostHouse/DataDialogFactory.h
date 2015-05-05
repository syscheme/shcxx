#ifndef _zq_data_post_house_dialog_partial_implement_header_file_h__
#define _zq_data_post_house_dialog_partial_implement_header_file_h__

#include "DataCommunicatorEx.h"
#include "DataPostHouseEnv.h"
#include <list>
#include <Locks.h>


namespace ZQ
{
namespace DataPostHouse
{

class DialogFactory : public IDataDialogFactory
{
public:
	DialogFactory( );
	virtual DialogFactory( );

public:	

	virtual void					close( ) ;

	virtual IDataDialogPtr			createDataDialog( IDataCommunicatorPtr communicator ) ;

	virtual void					releaseDataDialog( IDataDialogPtr idalog , IDataCommunicatorPtr communicator) ;

private:
	typedef std::list<IDataCommunicatorExPtr>	CommunicatorList;
	CommunicatorList							mCommList;
	ZQ::common::Mutex							mCommLocker;
};

}}//namespace ZQ::DataPostHouse
#endif //_zq_data_post_house_dialog_partial_implement_header_file_h__

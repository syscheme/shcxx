
#ifndef _zq_data_Post_house_windows_implement_header_file_h__
#define _zq_data_Post_house_windows_implement_header_file_h__

#include <zq_common_conf.h>
#include <vector>
#include "DataPostHouseEnv.h"
#include "DataCommunicatorEx.h"
#include <NativeThread.h>

namespace ZQ
{
namespace DataPostHouse
{



class DataPostDak;
class DataPostMan : public ZQ::common::NativeThread
{
public:	
	DataPostMan( DataPostDak& dak , DataPostHouseEnv& env,  HANDLE completionPort );
	virtual ~DataPostMan( );

	void	stop( );

protected:
	
	bool	init(void);
	int		run(void);
	void	final(void);

private:

	bool				mQuit;

	HANDLE				mCompletionPort;
	
	DataPostDak&		mDak;
	DataPostHouseEnv&	mEnv;
};

class DataPostDak 
{
public:
	DataPostDak( DataPostHouseEnv& env , IDataDialogFactoryPtr factory );
	virtual ~DataPostDak( );

public:
	bool			startDak(  int32 maxPostmen = 10 );
	
	void			stopDak( );

	bool			addnewCommunicator( IDataCommunicatorExPtr comm  );

private:

	typedef std::vector<DataPostMan*>		DataPostMen;
	
	DataPostMen								mPostMen;

	HANDLE									mCompletionPort;

	DataPostHouseEnv&						mEnv;

	IDataDialogFactoryPtr					mDialogFactory;

	bool									mbStart;
	
};

}}//namespace ZQ::DataPostHouse

#endif//_zq_data_Post_house_windows_implement_header_file_h__

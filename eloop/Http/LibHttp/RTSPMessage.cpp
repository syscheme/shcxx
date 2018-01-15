#include "RTSPMessage.h"

namespace ZQ {
namespace eloop {

//-------------------------------------
//	class RTSPMessage
//-------------------------------------
RtspCode2StatusMapInit rtspcode2status;

const std::string& RTSPMessage::header( const std::string& key) const 
{
	ZQ::common::MutexGuard gd(_lockHeaders);
	HEADERS::const_iterator it = _headers.find(key);
	if( it == _headers.end())
		return _dummyVal;
	return it->second;
}

} }//namespace ZQ::eloop
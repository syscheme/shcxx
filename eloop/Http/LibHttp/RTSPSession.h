#ifndef __RTSP_SESSION_H__
#define __RTSP_SESSION_H__

#include <map>
#include "Guid.h"
#include "Pointer.h"

// ---------------------------------------
// class RTSPSession
// ---------------------------------------

class RTSPSession : public virtual ZQ::common::SharedObject
{
public:
	typedef ZQ::common::Pointer<RTSPSession> Ptr;
	typedef std::map<std::string, Ptr> Map;

public:
	RTSPSession()
	{
		char buf[80];
		ZQ::common::Guid guid;
		guid.create();
		guid.toCompactIdstr(buf, sizeof(buf) -2);
		_id = buf;
	}

	const std::string& id() const { return _id; }

	virtual bool	setup() {}
	virtual bool	play() {}
	virtual bool	pause() {}
	virtual bool	teardown() {}

private:
	std::string		_id;
};




#endif
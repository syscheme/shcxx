// ===========================================================================
// Copyright (c) 2015 by
// XOR media, Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of ZQ Interactive, Inc. Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from ZQ Interactive, Inc.
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by ZQ Interactive, Inc.
//
// Ident : $Id: ZQSnmp.h,v 1.8 2014/05/26 09:32:35 hui.shao Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define SNMP exports
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/eloop/eloop_file.h $
// ===========================================================================

#ifndef __ZQ_COMMON_ELOOP_File_H__
#define __ZQ_COMMON_ELOOP_File_H__

#include "eloop.h"
#include "eloop_net.h"
#include <fcntl.h>
namespace ZQ {
namespace eloop {
class ZQ_ELOOP_API FileEvent;
class ZQ_ELOOP_API File;
class ZQ_ELOOP_API Pipe;

// -----------------------------
// class FileEvent
// -----------------------------
// dup of uvpp_fs_event
class FileEvent : public Handle
{
public:
	enum fs_event {
		Rename = UV_RENAME,
		Change = UV_CHANGE
	};
	enum fs_event_flags{
		Event_Default = 0,
		Watch_Entry = UV_FS_EVENT_WATCH_ENTRY,
		Event_Stat = UV_FS_EVENT_STAT,
		Event_Recursive = UV_FS_EVENT_RECURSIVE
	};
	
public:
	FileEvent();

	int init(Loop &loop);
	int start(const char *path, fs_event_flags flags = Event_Default);
	int stop();
	int getpath(char *buffer, size_t *size);

protected:
	virtual void OnFileEvent(const char *filename, fs_event events, ElpeError status) {}

private:
	static void _cbFSevent(uv_fs_event_t *handle, const char *filename, int events, int status);
};


// -----------------------------
// class File
// -----------------------------
class File
{
public:
	enum FileFlags{
		RDONLY = O_RDONLY,
		WRONLY = O_WRONLY,
		RDWR = O_RDWR,
		CREAT = O_CREAT,
		APPEND = O_APPEND,
		CREAT_RDWR = O_CREAT | O_RDWR,
		CREAT_WRONLY = O_CREAT | O_WRONLY
	};
	enum FileMode{
		RD_WR = _S_IREAD | _S_IWRITE,
		READ = _S_IREAD,
		WRITE = _S_IWRITE
	};

public:
	File(Loop& loop);
	File();
	int open(const char* filename,int flags,int mode);
	int read(size_t len);
	int write(const char* data,size_t len);
	int mkdir(const char* dirname,int mode);
	int close();
	void cleanup(uv_fs_t* req);
	char* _buf;


protected:
	virtual void OnOpen(int result){}
	virtual void OnWrite(int result){}
	virtual void OnRead(char* data,int len){}
	virtual void OnClose(int result){}
	virtual void OnMkdir(int result){}

private:
	static void _cbFileOpen(uv_fs_t* req);
	static void _cbFileClose(uv_fs_t* req);
	static void _cbFileWrite(uv_fs_t* req);
	static void _cbFileRead(uv_fs_t* req);
	static void _cbMkdir(uv_fs_t* req);
	void setfb(int fb);

private:
	Loop& _loop;
	int _fb;
};


// -----------------------------
// class Pipe
// -----------------------------
class Pipe : public Stream
{
public:
	Pipe();

	int init(Loop &loop, int ipc = 0);
	int open(uv_file file);
	int bind(const char *name);
	void connect(const char *name);
	int getsockname(char *buffer, size_t *size);
	int getpeername(char *buffer, size_t *size);
	void pending_instances(int count);
	int pending_count();
	

protected:
	//TODO: why wiped uv_connect_t here??
	virtual void OnConnected(ElpeError status) {}

private:
	static void _cbConnected(uv_connect_t *req, int status);
	
};

} } // namespace ZQ::eloop


#endif // __ZQ_COMMON_ELOOP_File_H__

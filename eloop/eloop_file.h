// ===========================================================================
// Copyright (c) 2015 by
// XOR media, Shanghai, PRC.,
// All Rights Reserved.  Unpublished rights reserved under the copyright
// laws of the United States.
// 
// The software contained  on  this media is proprietary to and embodies the
// confidential technology of syscheme Possession, use,
// duplication or dissemination of the software and media is authorized only
// pursuant to a valid written license from syscheme
// 
// This software is furnished under a  license  and  may  be used and copied
// only in accordance with the terms of  such license and with the inclusion
// of the above copyright notice.  This software or any other copies thereof
// may not be provided or otherwise made available to  any other person.  No
// title to and ownership of the software is hereby transferred.
//
// The information in this software is subject to change without notice and
// should not be construed as a commitment by syscheme
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
extern "C"{
	#include <fcntl.h>
};

namespace ZQ {
namespace eloop {
class ZQ_ELOOP_API FSMonitor;
class ZQ_ELOOP_API File;
class ZQ_ELOOP_API Pipe;

#ifndef FLAG
#  define FLAG(_F)  (1<<_F)
#endif // FLAG

#ifdef ZQ_OS_MSWIN

	# ifndef S_IRUSR
	#  define S_IRUSR _S_IREAD
	# endif

	# ifndef S_IWUSR
	#  define S_IWUSR _S_IWRITE
	# endif
#endif

// -----------------------------
// class FSMonitor
// -----------------------------
// to monitors file events occur on file system
class FSMonitor : public Handle
{
public:
	typedef enum _Event {
		fseRenamed = UV_RENAME, // file event when a file or dir has been renamed
		fseChanged = UV_CHANGE  // file event when a file has been modified
	} Event;

	// flags to be passed to start() to watch a file
	typedef enum _StartFlags {
		fsfNone   = 0, // 0 means none flag is turned on
		
		// fefWatchEntry  - instead of watching a directory and all its children file by default,
		//                  this flag specify to only watch the directly entry itself
		fsfWatchEntry     = FLAG(0), // UV_FS_EVENT_WATCH_ENTRY,

		// fsfActiveStat  - for some file systems such as NFS mounts to remote storage, it is recommended
		//                  to actively call stat() on a regular interval for the file event
		fsfActiveStat     = FLAG(1), // UV_FS_EVENT_STAT,

		// fsfRecursive   - to watching the sub-directories
	    fsfRecursive      = FLAG(2), // UV_FS_EVENT_RECURSIVE
	} StartFlags ;
	
public:
	FSMonitor(Loop& loop);

	//@param flags - flag combination of StartFlags
	int monitor(const char *path, uint flags = fsfNone);
	int stop();
	std::string path();

protected:
	//@param events  - combination of flags of Event
	virtual void OnFileEvent(const char *filename, uint events, ElpeError status) {}

//private: // impl of Handle
//	void init();
private:
	static void _cbFSevent(uv_fs_event_t *handle, const char *filename, int events, int status);
};

// -----------------------------
// class File
// -----------------------------
class File
{
public:
	enum FileFlags {
		RDONLY       = O_RDONLY,
		WRONLY       = O_WRONLY,
		RDWR         = O_RDWR,
		CREAT        = O_CREAT,
		APPEND       = O_APPEND,
		CREAT_RDWR   = O_CREAT | O_RDWR,
		CREAT_WRONLY = O_CREAT | O_WRONLY
	};

	enum FileMode {
		RD_WR = S_IRUSR | S_IWUSR,
		READ  = S_IRUSR,
		WRITE = S_IWUSR
	};

public:
	File(Loop& loop);
	File();

	//@param flags - flag combination of FileFlags
	//@param mode  - flag combination of FileMode
	int open(const char* filename, uint flags, uint mode);
	int read(size_t len, int64_t offset);
	int read(char* data,size_t len, int64_t offset);
	int write(const char* data, size_t len, int64_t offset);
	int write(const ZQ::eloop::Handle::eloop_buf_t bufs[],unsigned int nbufs,int64_t offset);

	//@param mode  - flag combination of FileMode
	int mkdir(const char* dirname, uint mode);
	int close();
	void clean();
	char* _buf;

protected:
	virtual void OnOpen(int result) {}
	virtual void OnWrite(int result) {}
	virtual void OnRead(char* data,int len) {}
	virtual void OnRead(int len) {}
	virtual void OnClose(int result) {}
	virtual void OnMkdir(int result) {}

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
	bool _isAlloc;
	size_t _len;
};

// -----------------------------
// class Pipe
// -----------------------------
class Pipe : public AbstractStream
{
public:
	Pipe(Loop& loop, int ipc=1);

	int  open(uv_file file);
	int  bind(const char *name);
	void  connect(const char *name);
	int  getsockname(char *buffer, size_t *size);
	int  getpeername(char *buffer, size_t *size);
	void  pending_instances(int count);
	int  pending_count();
	eloop_handle_type pending_type();
	#ifdef ZQ_OS_LINUX
		int sendfd(const eloop_buf_t bufs[],unsigned int nbufs,int fd);
		int acceptfd();
	#endif

protected:
	//TODO: why wiped uv_connect_t here??
	virtual void OnConnected(ElpeError status) {}

private: // impl of Handle
	void init();
private:
	static void _cbConnected(uv_connect_t *req, int status);
	uv_pipe_t	_fdContainer;
	int _ipc;
	
};

} } // namespace ZQ::eloop

#endif // __ZQ_COMMON_ELOOP_File_H__

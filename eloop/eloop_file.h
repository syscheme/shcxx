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
namespace ZQ {
namespace eloop {

// -----------------------------
// class File
// -----------------------------
// dup of uvpp_fs_event
class File : public Handle
{
public:
	File();
	int init(Loop &loop);
	int start(const char *path, unsigned int flags);
	int stop();
	int getpath(char *buffer, size_t *size);

protected:
	virtual void OnFile_cb(File *self, const char *filename, int events, int status) {}

private:
	static void fs_event_cb(uv_fs_event_t *handle, const char *filename, int events, int status);
};

// -----------------------------
// class Pipe
// -----------------------------
class Pipe : public Stream {

public:
	Pipe();
	int init(Loop &loop, int ipc);
	int open(uv_file file);
	int bind(const char *name);
	void connect(const char *name);
	int getsockname(char *buffer, size_t *size);
	int getpeername(char *buffer, size_t *size);
	void pending_instances(int count);
	int pending_count();

protected:
	virtual void OnPipeConnect_cb(Pipe* self, int status) {}

private:
	static void connect_cb(uv_connect_t *req, int status);
};

} } // namespace ZQ::eloop


#endif // __ZQ_COMMON_ELOOP_File_H__

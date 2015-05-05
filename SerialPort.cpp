// ===========================================================================
// Copyright (c) 2004 by
// ZQ Interactive, Inc., Shanghai, PRC.,
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
// Ident : $SerialPort.cpp 2009/10/26 $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define Base Logger
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/SerialPort.cpp $
// 
// 1     10-11-12 15:56 Admin
// Created.
// 
// 1     10-11-12 15:28 Admin
// Created.
// 
// 2     09-10-26 18:31 Hui.shao
// 
// 1     09-10-26 17:53 Hui.shao
// initially created
// ===========================================================================

#include "SerialPort.h"

extern "C"
{
#include <stdio.h>
}

#define MAP_RCV_FRAME_BFR_LEN	4096
#define MAP_TRX_FRAME_BFR_LEN   4096

namespace ZQ {
namespace common {

// -----------------------------
// class SerialPort
// -----------------------------
SerialPort::SerialPort()
: _hSerialPort(INVALID_HANDLE_VALUE)
{
	memset(&_dcb, 0x00, sizeof(_dcb));
	memset(&_commTimeouts, 0x00, sizeof(_commTimeouts));
	memset(&_overLapped, 0x00, sizeof(_overLapped));
	_commTimeouts.ReadIntervalTimeout =DEFAULT_TIMEOUT_READ;
}

SerialPort::~SerialPort()
{
	close();
}

const char* SerialPort::name() const
{
	if (isOpened())
		return NULL;

	return _portName.c_str();
}

bool SerialPort::open(const char* portName)
{
	if (NULL == portName)
		return false;

	close(); // close the previous open

	// _portName = "\\\\?\\" + _portName;
	_hSerialPort = ::CreateFile(portName, GENERIC_READ | GENERIC_WRITE,	0, 0, OPEN_EXISTING, 0, 0);
	
	if (INVALID_HANDLE_VALUE == _hSerialPort)
		return false;

	_portName = portName;

	if (!::GetCommState(_hSerialPort, &_dcb))
		return false;

	setTimeouts();

	return true;
}

bool SerialPort::close()
{
	if (isOpened())
		::CloseHandle(_hSerialPort);

	_hSerialPort = INVALID_HANDLE_VALUE;
	_portName = "";

	memset(&_dcb, 0x00, sizeof(_dcb));
	memset(&_commTimeouts, 0x00, sizeof(_commTimeouts));
	memset(&_overLapped, 0x00, sizeof(_overLapped));

	return true;
}

bool SerialPort::set(const BaudRate speed, const StopBits stopBits, const Parity parity)
{
	if (!isOpened())
		return false;

	_dcb.BaudRate			= speed;
	_dcb.fParity			= 1;
	_dcb.fBinary			= 1;
	_dcb.Parity				= parity;
	_dcb.ByteSize			= 8;
	_dcb.StopBits			= stopBits;

	if(!::SetCommState(_hSerialPort, &_dcb))
		return false;

	::SetupComm(_hSerialPort, MAP_RCV_FRAME_BFR_LEN, MAP_TRX_FRAME_BFR_LEN);

	return true;
}

bool SerialPort::set(const char* settingDescStr)
{
	if (!isOpened())
		return false;

	DCB dcb;

	if (NULL == settingDescStr || !::BuildCommDCB(settingDescStr, &dcb))
		return false;

	if(!::SetCommState(_hSerialPort, &dcb))
		return false;

	memcpy(&_dcb, &dcb, sizeof(_dcb));

	::SetupComm(_hSerialPort, MAP_RCV_FRAME_BFR_LEN, MAP_TRX_FRAME_BFR_LEN);

	return true;
}

bool SerialPort::setTimeouts(const int readTimeout, const int writeTimeout)
{
	if (!::GetCommTimeouts(_hSerialPort, &_commTimeouts))
		return false;

	_commTimeouts.ReadIntervalTimeout			= readTimeout;
//	_commTimeouts.ReadTotalTimeoutMultiplier	= 0;
//	_commTimeouts.ReadTotalTimeoutConstant		= 0;
	_commTimeouts.WriteTotalTimeoutConstant		= writeTimeout;
//	_commTimeouts.WriteTotalTimeoutMultiplier	= 0;

	if(!::SetCommTimeouts(_hSerialPort, &_commTimeouts))
		return false;

	return true;
}

int SerialPort::read(char* buf, const int maxSize)
{
	if (!isOpened() || NULL == buf)
		return -1;

	DWORD nByteRead=0;
	::ReadFile(_hSerialPort, buf, maxSize, &nByteRead, NULL);

	return (int) nByteRead;
}

int SerialPort::write(const char* buf, const int size)
{
	if (!isOpened() || NULL == buf)
		return -1;

	DWORD nByteWrite=0;
	::WriteFile(_hSerialPort, buf, size, &nByteWrite, NULL);

	return (int) nByteWrite;
}

/// test if the serial post has been connected
///@return true if connected
bool SerialPort::isOpened() const
{
	return (INVALID_HANDLE_VALUE != _hSerialPort);
}

bool SerialPort::writeByte(const uint8 data)
{
	return (1 == write((const char*) &data, 1));
}

bool SerialPort::readByte(uint8& data)
{
	return (1 == read((char*) &data, 1));
}


} // namespace common
} // namespace ZQ

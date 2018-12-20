// ===========================================================================
// Copyright (c) 2004 by
// syscheme, Shanghai,,
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
// Ident : $Id: SerialPort.h,v 1.13 2009/10/26 10:06:56 jshen Exp $
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : Define Base Logger
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/SerialPort.h $
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

#ifndef	__ZQ_COMMON_SerialPort_H__
#define	__ZQ_COMMON_SerialPort_H__

#include "ZQ_common_conf.h"

#include <string>

namespace ZQ {
namespace common {

class ZQ_COMMON_API SerialPort;

#ifdef ZQ_OS_MSWIN
#  define DEFAULT_TIMEOUT_READ  MAXDWORD
#  define DEFAULT_TIMEOUT_WRITE (0)
#endif // ZQ_OS_MSWIN

// -----------------------------
// class SerialPort
// -----------------------------
class SerialPort
{
public:

	typedef enum  //	known baud rates
	{
		BaudRate2400   =CBR_2400,
		BaudRate4800   =CBR_4800,
		BaudRate9600   =CBR_9600,
		BaudRate14400  =CBR_14400,
		BaudRate19200  =CBR_19200,
		BaudRate57600  =CBR_57600,
		BaudRate115200 =CBR_115200,
		BaudRate256000 =CBR_256000
	} BaudRate;

	typedef enum // parity
	{
		NoParity   =NOPARITY,
		OddParity  =ODDPARITY,
		EvenParity =EVENPARITY,
		MarkParity =MARKPARITY,
		SpaceParity=SPACEPARITY
	} Parity;

	typedef enum // parity
	{
		OneStopBit   =0,
		TwoStopBits  =2,
	} StopBits;

public:
	/// Default constructor: create an instance of our serial interface object.
	/// though it could be convenient to have default settings specified here as well as possibly
	/// a default port name.
	SerialPort();

	/// Default deconstructor: delete an instance of out serial interface object.
	virtual ~SerialPort();

	/// get the name of opened serial port
	/// return the NULL-terminated string name of the port, NULL if the serial port is not openned
	const char* SerialPort::name() const;

	/// opens the serial port using port-name provided.
	///@param[in] portName NULL terminated string name of the port to open: eg. COM1, COM2, COM4
	///@return a boolean indicating success or failure
	bool open(const char* portName);

	/// closes the serial port
	///@return a boolean indicating success or failure
	bool close();

	/// apply the settings of the serial port interface. Should be called after the port is opened
	// because otherwise settings could be lost.
	///@param[in] speed the desired speed of the serial port
	///@param[in] stopBits the number of stop bits
	///@param[in] parity parity to be used.
	///@eeturn boolean indicating success or failure
	bool set(const BaudRate speed, const StopBits stopBits, const Parity parity);

	/// apply the settings of the serial port interface. Should be called after the port is opened
	// because otherwise settings could be lost.
	///@param[in] settingDescStr the description string of setting. e.g. "9600,n,8,1"
	///@eeturn boolean indicating success or failure
	bool set(const char* settingDescStr="9600,n,8,1");

	///set the read and write timeouts of the serial port
	///@param[in] readTimeout The amount of time to wait for a single word read to timeout
	///@param[in] writeTimeout The amount of time to wait for a single word write to timeout
	///@return boolean indicating success or failure
	bool setTimeouts(int readTimeout=DEFAULT_TIMEOUT_READ, int writeTimeout=DEFAULT_TIMEOUT_WRITE);

	/// read a buffer of bytes
	///@param[in] buf The buffer to receive the data
	///@param[in] maxSize the max number of bytes that the buffer can receive
	///@return the number of bytes actually read
	int read(char* buf, const int maxSize);

	/// write a buffer of bytes
	///@param[in] buf The buffer to send
	///@param[in] size the number of bytes to send
	///@return the number of bytes actually sent
	int write(const char* buf, const int size);

	/// write a byte thru the serial port
	///@param[in] data the byte to send
	///@return true if succeeded
	bool writeByte(const uint8 data);

	/// read a byte from the serial port
	///@param[in] data the byte to save
	///@return true if succeeded
	bool readByte(uint8& data);

	/// test if the serial post has been connected
	///@return true if connected
	bool isOpened() const;

protected:
	::std::string	_portName;

#ifdef ZQ_OS_MSWIN
	DCB				_dcb;
	COMMTIMEOUTS	_commTimeouts;
	OVERLAPPED		_overLapped;
	HANDLE			_hSerialPort;
#endif // ZQ_OS_MSWIN

};

// usage of the SerialPort:
// SerialPort sp;
// sp.open("COM1"); // open the port
// sp.set(SerialPort::BaudRate9600, SerialPort::OneStopBit, SerialPort::OddParity); // apply the settings
// sp.write(buf, size); or sp.read(buf, size);
// ...
// sp.close();

} // namespace common
} // namespace ZQ

#endif // __ZQ_COMMON_SerialPort_H__


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF 281421,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

#pragma warning(disable:4786)

static const char* const RtspMsg_cxx_Version = 
    "$Id: RtspMsg.cxx,v 1.1.1.1 2006/02/20 10:05:06 Cary.xiao Exp $";

#include "RtspMsg.hxx"
#include "CharDataParser.hxx"
#include "RtspBadDataException.hxx"
#include "cpLog.h"

RtspMsg::RtspMsg()
    : // myTransConnPtr(0),
      myStartLine(),
      myHeaders(),
      myNumHeader(0),
      myContentLength(0),
      myHasBody(false),
      myMsgBody(),
      myCompletionFlag(RTSP_MSG_NEW),
      myIncompHeader(),
      myFirstWordUnknown(false),
      mySessionId(),
      myCSeq(),
      myContentBase(),
      myContentType(),
      myTransportSpec(0),
      myRange(0)
{
    for(u_int32_t i = 0; i < RTSP_UNKNOWN_HDR; i++)
    {
        myHeaderMap[i].offset = -1;
        myHeaderMap[i].len = 0;
    }
}

RtspMsg::RtspMsg(const RtspMsg& src)
    : // myTransConnPtr(src.myTransConnPtr),
      myStartLine(src.myStartLine),
      myHeaders(src.myHeaders),
      myNumHeader(src.myNumHeader),
      myContentLength(src.myContentLength),
      myHasBody(src.myHasBody),
      myMsgBody(src.myMsgBody),
      myCompletionFlag(src.myCompletionFlag),
      myIncompHeader(src.myIncompHeader),
      myFirstWordUnknown(src.myFirstWordUnknown),
      mySessionId(src.mySessionId),
      myCSeq(src.myCSeq),
      myContentBase(src.myContentBase),
      myContentType(src.myContentType),
      myTransportSpec(0),
      myRange(0),
	  myAddedHeaderMap(src.myAddedHeaderMap)
{
    if (src.myTransportSpec != 0)
    {
        myTransportSpec = new RtspTransportSpec(*src.myTransportSpec);
    }
    if (src.myRange != 0)
    {
        myRange = new RtspRangeHdr(*src.myRange);
    }
    for(u_int32_t i = 0; i < RTSP_UNKNOWN_HDR; i++)
    {
        myHeaderMap[i].offset = src.myHeaderMap[i].offset;
        myHeaderMap[i].len = src.myHeaderMap[i].len;
    }
}

RtspMsg&
RtspMsg::operator=(const RtspMsg& src)
{
    if (&src != this)
    {
        // myTransConnPtr = src.myTransConnPtr;
        myStartLine = src.myStartLine;
        myHeaders = src.myHeaders;
        myNumHeader = src.myNumHeader;
        myContentLength = src.myContentLength;
        myHasBody = src.myHasBody;
        myMsgBody = src.myMsgBody;
        myCompletionFlag = src.myCompletionFlag;
        myIncompHeader = src.myIncompHeader;
        myFirstWordUnknown = src.myFirstWordUnknown;
        mySessionId = src.mySessionId;
        myCSeq = src.myCSeq;
        myContentBase = src.myContentBase;
        myContentType = src.myContentType;
        myTransportSpec = 0;
        myRange = 0;
		myAddedHeaderMap = src.myAddedHeaderMap;
        if (src.myTransportSpec != 0)
        {
            myTransportSpec = new RtspTransportSpec(*src.myTransportSpec);
        }
        if (src.myRange != 0)
        {
            myRange = new RtspRangeHdr(*src.myRange);
        }

        for(u_int32_t i = 0; i < RTSP_UNKNOWN_HDR; i++)
        {
            myHeaderMap[i].offset = src.myHeaderMap[i].offset;
            myHeaderMap[i].len = src.myHeaderMap[i].len;
        }
    }
    return *this;
}

const u_int32_t
RtspMsg::getHdrBodyValue(RtspHeadersType header) const
{
    u_int32_t theValue = 0;
	if (myHeaderMap[header].isModified()) {
		CharData numData;

		numData.set(myHeaderMap[header].data.getDataBuf(),
					myHeaderMap[header].data.length());
		CharDataParser numDataParser(&numData);

		u_int32_t value;
		if ( numDataParser.getNextInteger(value) ) {
			theValue = value;
		} else {
			cpLog(LOG_ERR, "error read value of header: %d", header);
		}

	} else {
		if (myHeaderMap[header].offset != -1)
		{
			CharData numData;
	//        cpLog(LOG_DEBUG_STACK, "myHeaders is: %s", myHeaders.getData());

			numData.set(myHeaders.getDataBuf() + 
						myHeaderMap[header].offset,
						myHeaderMap[header].len);
	//        cpLog(LOG_DEBUG_STACK, "The hdr body offset is: %d", 
	//              myHeaderMap[header].offset);
	//        cpLog(LOG_DEBUG_STACK, "The hdr body len is: %d", numData.getLen());

			CharDataParser numDataParser(&numData);

			u_int32_t value;
			if ( numDataParser.getNextInteger(value) ) {
				theValue = value;
			} else {
				cpLog(LOG_ERR, "error read value of header: %d", header);
			}
		}
	}

    return theValue;
}

const Data
RtspMsg::getHdrBodyData(RtspHeadersType header, bool full) const
{
	if (myHeaderMap[header].isModified()) {
		return myHeaderMap[header].data;
	} else {
		Data theData;
		if (myHeaderMap[header].offset != -1)
		{
			CharData bodyData;
			bodyData.set(myHeaders.getDataBuf() + 
						 myHeaderMap[header].offset,
						 myHeaderMap[header].len);

			CharDataParser bodyParser(&bodyData);

			CharData theFirstBody;
			u_int8_t* stop = (full) ? bodyParser.myMaskEol : 
									  bodyParser.myMaskEolSpace;
			if (bodyParser.parseUntil(&theFirstBody, stop))
			{
				Data theFirstData(theFirstBody.getPtr(), theFirstBody.getLen());
				theData = theFirstData;
			}
			else
			{
				cpLog(LOG_ERR, "error read data of header: %d", header);
			}
		}
		return theData;
	}
}


const int
RtspMsg::getContentLength() 
{
    if (myContentLength == 0)
    {
        myContentLength = getHdrBodyValue(RTSP_CONTENT_LENGTH_HDR);
    }
    return myContentLength;
}

bool
RtspMsg::isValid()
{
    if (myHeaderMap[RTSP_CSEQ_HDR].offset != -1)
        return true;
    else
        return false;
}

bool
RtspMsg::isRequest()
{
    CharData strtLine;
    strtLine.set(myStartLine.getDataBuf(), myStartLine.length());
    CharDataParser charParser(&strtLine);

    CharData firstWord;
    charParser.getNextWord(&firstWord);

    return ( !(firstWord.isEqualNoCase("RTSP", 4)) );
}

bool
RtspMsg::isResponse()
{
    return ( !(isRequest()) );
}

const Data& 
RtspMsg::getSessionId()
{
//    if (mySessionId.length() == 0)
//    {
        mySessionId = getHdrBodyData(RTSP_SESSION_HDR, false);
//    }
    return mySessionId;
}

bool 
RtspMsg::setSessionId(const Data& sessionId)
{
	return setHdrBodyData(RTSP_SESSION_HDR, sessionId);
}

const Data&
RtspMsg::getCSeq()
{
    if (myCSeq.length() == 0)
    {
        myCSeq = getHdrBodyData(RTSP_CSEQ_HDR, false);
    }
    return myCSeq;
}

const Data&
RtspMsg::getContentBase()
{
    if (myContentBase.length() == 0)
    {
        myContentBase = getHdrBodyData(RTSP_CONTENT_BASE_HDR, false);
    }
    return myContentBase;
}

const Data&
RtspMsg::getContentType()
{
    if (myContentType.length() == 0)
    {
        myContentType = getHdrBodyData(RTSP_CONTENT_TYPE_HDR, true);
    }
    return myContentType;
}

Sptr< RtspTransportSpec >
RtspMsg::getTransport()
{
	if (myHeaderMap[RTSP_TRANSPORT_HDR].isModified()) {
		CharData transpData;
		transpData.set(myHeaderMap[RTSP_TRANSPORT_HDR].data.getDataBuf(), 
			myHeaderMap[RTSP_TRANSPORT_HDR].data.length());
		CharDataParser transpParser(&transpData);
		RtspTransportHdr transHdr;
		if (myTransportSpec != NULL)
			myTransportSpec = NULL;
		myTransportSpec = transHdr.parseTransportHeader(transpParser);

	} else {
		if (myTransportSpec == 0)
		{
			CharData transpData;
			if (myHeaderMap[RTSP_TRANSPORT_HDR].offset != -1)
			{
				transpData.set(myHeaders.getDataBuf() + 
						   myHeaderMap[RTSP_TRANSPORT_HDR].offset,
						   myHeaderMap[RTSP_TRANSPORT_HDR].len);
				CharDataParser transpParser(&transpData);
				RtspTransportHdr transHdr;
				myTransportSpec = transHdr.parseTransportHeader(transpParser);
			}
		}
	}
	
	return myTransportSpec;
}

Sptr< RtspRangeHdr >
RtspMsg::getRange()
{
	if (myHeaderMap[RTSP_RANGE_HDR].isModified()) {
		CharData rangeData;
		rangeData.set(myHeaderMap[RTSP_RANGE_HDR].data.getDataBuf(), 
					  myHeaderMap[RTSP_RANGE_HDR].data.length());
		CharDataParser rangeParser(&rangeData);

		if (myRange != NULL)
			myRange = 0;

		myRange = new RtspRangeHdr();
		myRange->parseRangeHeader(rangeParser);
		
	} else {
		if (myRange == 0)
		{
			if (myHeaderMap[RTSP_RANGE_HDR].offset != -1)
			{
				CharData rangeData;
				rangeData.set(myHeaders.getDataBuf() + 
							  myHeaderMap[RTSP_RANGE_HDR].offset,
							  myHeaderMap[RTSP_RANGE_HDR].len);
				CharDataParser rangeParser(&rangeData);

				myRange = new RtspRangeHdr();
				myRange->parseRangeHeader(rangeParser);
			}
		}
	}

    return myRange;
}

bool RtspMsg::setHdrBodyData(RtspHeadersType header, const Data& data)
{
	if (header >= RTSP_UNKNOWN_HDR)
		return false;

	myHeaderMap[header].data = data;
	myHeaderMap[header].dirty();
	return true;
}

/*
bool RtspMsg::setHdrBodyValue(RtspHeadersType header, u_int32_t value)
{
	char buf[64];

	if (header >= RTSP_UNKNOWN_HDR)
		return false;

	sprintf(buf, "%lu", value);
	myHeaderMap[RTSP_TRANSPORT_HDR].dirty();
	myHeaderMap[RTSP_TRANSPORT_HDR].data = buf;
	return true;
}
*/

const Data
RtspMsg::getHdrBodyData(const std::string& headerName, bool full) const
{
	HeaderMap::const_iterator itor = myAddedHeaderMap.find(headerName);
	if (itor == myAddedHeaderMap.end()) {
		CharData hdrName(headerName.c_str());
		u_int32_t code = RtspUtil::getHeaderInNumber(hdrName);
		if (code == RTSP_UNKNOWN_HDR) {
			return Data();
		}
		return getHdrBodyData((RtspHeadersType)code, full);
	}

	Data theData;
	Data result;
	if (itor->second.getData(myHeaders.getDataBuf(), theData)) 
	{

		CharData bodyData;
		bodyData.set(theData.getDataBuf(), theData.length());
		CharDataParser bodyParser(&bodyData);

		CharData theFirstBody;
		u_int8_t* stop = (full) ? bodyParser.myMaskEol : 
								  bodyParser.myMaskEolSpace;
		
		if (bodyParser.parseUntil(&theFirstBody, stop))	
		{
			Data theFirstData(theFirstBody.getPtr(), theFirstBody.getLen());
			return theFirstData;
		}
		else 
		{
			cpLog(LOG_ERR, "error read data of header: %s", headerName.c_str());
		}
	}
	
	return result;
}

bool RtspMsg::setHdrBodyData(const std::string& headerName, const Data& data)
{
	HeaderMap::iterator itor = myAddedHeaderMap.find(headerName);
	if (itor == myAddedHeaderMap.end()) {
		CharData hdrName(headerName.c_str());
		u_int32_t code = RtspUtil::getHeaderInNumber(hdrName);
		if (code != RTSP_UNKNOWN_HDR) {
			return setHdrBodyData((RtspHeadersType )code, data);
		} else {
			HeaderValueData hvd;
			hvd.data = data;
			hvd.dirty();
			myAddedHeaderMap.insert(AddedHeaderMapPair(headerName, hvd));
		}
	}

	itor->second.dirty();
	itor->second.data = data;

	return true;

}

const Data
RtspMsg::getAddedHdrBodyData(const std::string& headerName) const
{
	Data result;
	HeaderMap::const_iterator itor = myAddedHeaderMap.find(headerName);
	if (itor == myAddedHeaderMap.end())
		return result;
	return itor->second.data;
}

bool RtspMsg::setAddedHdrBodyData(const std::string& headerName, 
							 const Data& data)
{
	HeaderMap::iterator itor;
	itor = myAddedHeaderMap.find(headerName);
	if (itor == myAddedHeaderMap.end()) {
		HeaderValueData hvd;
		hvd.data = data;
		hvd.dirty();
		myAddedHeaderMap.insert(AddedHeaderMapPair(headerName, hvd));				
	}
	else {
		itor->second.dirty();
		itor->second.data = data;
	}
	return true;
}

/* Local Variables: */
/* c-file-style: "stroustrup" */
/* indent-tabs-mode: nil */
/* c-file-offsets: ((access-label . -) (inclass . ++)) */
/* c-basic-offset: 4 */
/* End: */

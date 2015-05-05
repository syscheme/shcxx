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

static const char* const RtspMsgParser_cxx_Version =
    "$Id: RtspMsgParser.cxx,v 1.1.1.1 2006/02/20 10:05:06 Cary.xiao Exp $";

#include <stdio.h>
#include "RtspMsgParser.hxx"
#include "CharDataParser.hxx"
#include "Data.hxx"
#include "cpLog.h"
#include "RtspDescribeMsg.hxx"
#include "RtspAnnounceMsg.hxx"
#include "RtspPlayMsg.hxx"
#include "RtspRecordMsg.hxx"
#include "RtspSetupMsg.hxx"
#include "RtspPauseMsg.hxx"
#include "RtspTeardownMsg.hxx"
#include "RtspOptionsMsg.hxx"
#include "RtspSetParameterMsg.hxx"
#include "RtspRequest.hxx"
#include "RtspResponse.hxx"


//#define RTSP_PROTOCOL_TRACE



// need to remove all the cpLogs for performance after debuging period
RtspMsg* RtspMsgParser::preParse( const char* data, 
                         int bytesNeedtoDecode,
                         int& bytesDecoded) 
            throw (RtspBadDataException&)
{
    RtspMsg* msg = createRtspMsg(data, bytesNeedtoDecode);
    assert(msg != 0);
        
    CharData stream(data, bytesNeedtoDecode);

    CharDataParser charParser(&stream);
    CharData aLine;
    int result;
   
    int advance = 0;

    if (msg->myCompletionFlag < RTSP_MSG_STRTLINE_COMPLETE)
    {
        result = charParser.getNextLine(&aLine);
    
        Data startline(aLine.getPtr(), aLine.getLen());
        msg->myStartLine += startline;
        
        bytesDecoded += aLine.getLen();
        advance += aLine.getLen();
    
        if (result == 0)
        {
            msg->myCompletionFlag = RTSP_MSG_NEW;
#ifdef RTSP_PROTOCOL_TRACE
            cpLog(LOG_DEBUG_STACK, "The startline is not finished.");
#endif
            return msg;
        }
        else
		{
            msg->myCompletionFlag = RTSP_MSG_STRTLINE_COMPLETE;
#ifdef RTSP_PROTOCOL_TRACE
			cpLog(LOG_DEBUG_STACK, "Startline is:%s", (msg->myStartLine).getData());
#endif
		}
    }

    bool foundEmptyLine = true;
    if (msg->myCompletionFlag < RTSP_MSG_HEADERS_COMPLETE)
        foundEmptyLine = false;

    const char* hdrStart = data + advance;
    int hdrsLen = 0;
    // int hdrIndex = msg->myNumHeader;
    while (!foundEmptyLine && (bytesDecoded < bytesNeedtoDecode))
    {
        result = charParser.getNextLine(&aLine);

        if (result == 0)
        {
            Data incompHdr(aLine.getPtr(), aLine.getLen());
            msg->myIncompHeader += incompHdr;
            bytesDecoded += aLine.getLen();
            advance += aLine.getLen();
            hdrsLen += aLine.getLen();
            // Data hdrs(hdrStart, hdrsLen);
            // msg->myHeaders += hdrs;
#ifdef RTSP_PROTOCOL_TRACE
            cpLog(LOG_DEBUG_STACK, "Incomplete Headers is: %s",
                  (msg->myIncompHeader).getData());
#endif
            // msg->myNumHeader = hdrIndex;
            return msg;
        }
        else
        {
			int newHdrOffset = (msg->myHeaders).length();

#ifdef RTSP_PROTOCOL_TRACE
			cpLog(LOG_DEBUG, "newHdrOffset %d", newHdrOffset);
#endif

            bytesDecoded += aLine.getLen();
            advance += aLine.getLen();
            hdrsLen += aLine.getLen();

            if( (msg->myIncompHeader).length() != 0 )
            {
                Data halfHdr(aLine.getPtr(), aLine.getLen());				
                msg->myIncompHeader += halfHdr;
                aLine.set((msg->myIncompHeader).getDataBuf(),
                          (msg->myIncompHeader).length());
            }
            // else // 到了这里就应该有新的行来了
            // {
            //    hdrIndex++;
            // }

#ifdef RTSP_PROTOCOL_TRACE
			Data hdrLine(aLine.getPtr(), aLine.getLen());
			cpLog(LOG_DEBUG_STACK, "get a line %s", hdrLine.getDataBuf());
#endif

			Data hdrs(aLine.getPtr(), aLine.getLen());
            msg->myHeaders += hdrs;

            CharDataParser lineParser(&aLine);
        
            if (aLine.isEmptyLine())
            {
                foundEmptyLine = true;
                // Data hdrs(hdrStart, hdrsLen);
                // msg->myHeaders += hdrs;
#ifdef RTSP_PROTOCOL_TRACE
                cpLog(LOG_DEBUG_STACK, "Complete Headers is: %s", 
                      (msg->myHeaders).getData());
#endif
                // msg->myNumHeader = hdrIndex;
                msg->myCompletionFlag = RTSP_MSG_HEADERS_COMPLETE;
            }
            else
            {
				msg->myNumHeader ++;

#ifdef RTSP_PROTOCOL_TRACE
                cpLog(LOG_DEBUG_STACK, "The hdrline len is:%d", aLine.getLen());
#endif       
                CharData aHeader;
                lineParser.getNextWord(&aHeader);
#ifdef RTSP_PROTOCOL_TRACE
                cpLog(LOG_DEBUG_STACK, "aHeader len is: %d", aHeader.getLen());
#endif              
                u_int32_t headerNum;
                headerNum = RtspUtil::getHeaderInNumber(aHeader);
#ifdef RTSP_PROTOCOL_TRACE
                cpLog(LOG_DEBUG_STACK, "headerNum is: %d", headerNum);
#endif
                int hdrBdyOffset = newHdrOffset + aHeader.getLen();
#ifdef RTSP_PROTOCOL_TRACE
				cpLog(LOG_DEBUG_STACK, "hdrBdyOffset %d", hdrBdyOffset);
#endif
                CharData tmpData;
                HeaderValueData hv;
  
                if ( lineParser.parseThru(&tmpData, ':') == 0 )
                {
                    hv.offset = -1;
                    hv.len = 0;
                }
                else
                {
                    hdrBdyOffset += tmpData.getLen();
 
                    lineParser.getThruSpaces(&tmpData);
                    hdrBdyOffset += tmpData.getLen();

                    hv.offset = hdrBdyOffset;
                    hv.len = newHdrOffset + aLine.getLen() - hdrBdyOffset;

                }
				
#ifdef RTSP_PROTOCOL_TRACE

				cpLog(LOG_DEBUG_STACK, "** hv.offset %d, hv.len %d",
						hv.offset, hv.len);

				Data hdrName(aHeader.getPtr(), aHeader.getLen());
				Data hdrVal(msg->myHeaders.getData() + hv.offset, hv.len);
				cpLog(LOG_DEBUG_STACK, "hdrNum = %d, hdrName = %s, hdrVal = %s",
					headerNum, hdrName.getDataBuf(), hdrVal.getDataBuf());
				if (headerNum == RTSP_SESSION_HDR) {
					if (hv.len <= 0)
						DebugBreak();
				}
#endif						
				
                if (headerNum < RTSP_UNKNOWN_HDR)
                {
					msg->setHeadersMap(headerNum, hv); 
					
                }
                else
                {
					msg->setAddedHeadersMap(aHeader, hv);
                }
            }
            newHdrOffset += aLine.getLen();
            (msg->myIncompHeader).erase();
        }
    }

	// 下面的代码没有经过严格测试
    if (foundEmptyLine)
    {
		int contentLen = msg->getContentLength();
#ifdef RTSP_PROTOCOL_TRACE
		cpLog(LOG_DEBUG_STACK, "Content-Length is: %d", contentLen);
#endif
        if (contentLen > 0)
        {
            if ( (contentLen - (msg->myMsgBody).length()) > (bytesNeedtoDecode - bytesDecoded) )
            {
                msg->myCompletionFlag = RTSP_MSG_HEADERS_COMPLETE;
#ifdef RTSP_PROTOCOL_TRACE
                cpLog(LOG_DEBUG_STACK, "The msgBody not complete");
#endif
                contentLen = bytesNeedtoDecode - bytesDecoded;
            }
            else
            {
                //if the msg already has some msgBody, contenLen need to 
                // subtract that portion since it has been parsed already
                contentLen -= (msg->myMsgBody).length();
                msg->myCompletionFlag = RTSP_MSG_COMPLETE;
#ifdef RTSP_PROTOCOL_TRACE
                cpLog(LOG_DEBUG_STACK, "The msgBody is complete");
#endif
            }

            Data msgBody(data + advance, contentLen);
            msg->setHasBody(true);
            msg->myMsgBody += msgBody;
            //cpLog(LOG_DEBUG_STACK, "Msgbody is: %s", (msg->myMsgBody).getData());
            bytesDecoded += contentLen;
        }
        else
        {
            msg->myCompletionFlag = RTSP_MSG_COMPLETE;
            msg->setHasBody(false);
#ifdef RTSP_PROTOCOL_TRACE
            cpLog(LOG_DEBUG_STACK, "There is no msgbody.");
#endif
        }
    }

	// 这次没有得到完整的包还有下次嘛, 抛什么异常
	/* 
    else
    {
        // should not come to here
        throw RtspBadDataException( "Bad RTSP message", __FILE__, __LINE__, 0 );
    }
	*/

    return msg;

}

RtspMsg*
RtspMsgParser::createRtspMsg(const char* data, int bytesNeedtoDecode)
{
    RtspMsg* msg = 0;

    CharData strtLine;
    strtLine.set(data, bytesNeedtoDecode);
    CharDataParser charParser(&strtLine);
    CharData firstWord;
    charParser.getNextWord(&firstWord);

    if (firstWord.isEqualNoCase("RTSP",4))
    {
        //msg = new RtspResponse();
		//Yes ,I known this is a response,but I would like to treat it like a request and it'll be changed in future process
		msg = new RtspRequest();
    }
    else
    {
        u_int32_t method;
        method = RtspUtil::getMethodInNumber(firstWord);
        switch (method)
        {
            case RTSP_ANNOUNCE_MTHD:
                msg = new RtspAnnounceMsg();
                break;
            case RTSP_DESCRIBE_MTHD:
                msg = new RtspDescribeMsg();
                break;
            case RTSP_PLAY_MTHD:
                msg = new RtspPlayMsg();
                break;
            case RTSP_RECORD_MTHD:
                msg = new RtspRecordMsg();
                break;
            case RTSP_PAUSE_MTHD:
                msg = new RtspPauseMsg();
                break;
            case RTSP_SETUP_MTHD:
                msg = new RtspSetupMsg();
                break;
            case RTSP_TEARDOWN_MTHD:
                msg = new RtspTeardownMsg();
                break;
            case RTSP_GET_PARAMETER_MTHD:
                msg = new RtspRequest();
                break;
            case RTSP_OPTIONS_MTHD:
                msg = new RtspOptionsMsg();
                break;
            case RTSP_REDIRECT_MTHD:
                msg = new RtspRequest();
                break;
            case RTSP_SET_PARAMETER_MTHD:
                msg = new RtspSetParameterMsg();
                break;
			case RTSP_SET_PING_MTHD:
                msg = new RtspRequest();
                break;
            default :
                msg = new RtspMsg();
                msg->setFirstWordUnknown(true);
                break;
        }
    }
    return msg;
    
}

/* Local Variables: */
/* c-file-style: "stroustrup" */
/* indent-tabs-mode: nil */
/* c-file-offsets: ((access-label . -) (inclass . ++)) */
/* c-basic-offset: 4 */
/* End: */

bool RtspMsgParser::chopping( const char* data, 
							 int bytesNeedtoDecode,
							 int& nBytesDecoded, int& nBytesSkipped) 
{
    CharData stream(data, bytesNeedtoDecode);
    CharDataParser charParser(&stream);
    CharData aLine;
    int result;
	int bytesDecoded =0;
	
	do
	{
		result = charParser.getNextLine(&aLine);
		bytesDecoded += aLine.getLen();
		if (result==0)
			return false;
		
		if (!aLine.isEmptyLine())
		{
			CharDataParser charPar(&aLine);
			CharData firstWord;
			charPar.getNextWord(&firstWord);
			if (firstWord.isEqualNoCase("RTSP",4))
			{
				break;
			}
			else
			{
				u_int32_t method;
				bool bFound;
				method = RtspUtil::getMethodInNumber(firstWord);
				switch (method)
				{
				case RTSP_ANNOUNCE_MTHD:
				case RTSP_DESCRIBE_MTHD:
				case RTSP_PLAY_MTHD:
				case RTSP_RECORD_MTHD:
				case RTSP_PAUSE_MTHD:
				case RTSP_SETUP_MTHD:
				case RTSP_TEARDOWN_MTHD:
				case RTSP_GET_PARAMETER_MTHD:
				case RTSP_OPTIONS_MTHD:
				case RTSP_REDIRECT_MTHD:
				case RTSP_SET_PARAMETER_MTHD:
				case RTSP_SET_PING_MTHD:
				case RTSP_RESPONSE_MTHD:
					bFound = true;
					break;
				default:
					bFound = false;
				};
				if (bFound)
					break;
			}
		}
	}while(bytesDecoded < bytesNeedtoDecode);
	nBytesSkipped = bytesDecoded - aLine.getLen();
	
	int contentLen=0;
    while (bytesDecoded < bytesNeedtoDecode)
    {
        result = charParser.getNextLine(&aLine);
		bytesDecoded += aLine.getLen();
		
        if (result == 0)
        {            
            return false;
        }
		
		if ( aLine.isEmptyLine() )
        {                
			if (contentLen > 0)
			{
				if ( (contentLen) > (bytesNeedtoDecode - bytesDecoded) )
				{
					return false;
				}
				
				bytesDecoded += contentLen;
			}				
			nBytesDecoded = bytesDecoded;
			return true;
        }
		else
		{
			CharDataParser lineParser(&aLine);
            CharData aHeader;
            lineParser.getNextWord(&aHeader);
			if (aHeader.isEqualNoCase("Content-length", 14))
			{	
				char* p = (char*)aHeader.getPtr () + aHeader.getLen ();
				int iLen = aLine.getLen () - aHeader.getLen ();
				while ( *p != ':' && iLen > 0 )
				{
					p++;
					iLen --;
				}
				if (iLen <=0) 
				{
					contentLen = 0;
				}
				else
				{					
					contentLen = atoi(p+1);
				}				
			}
		}
    }
	
	return false;
}

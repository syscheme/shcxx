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

static const char* const RtspRequest_cxx_Version =
    "$Id: RtspRequest.cxx,v 1.1.1.1 2006/02/20 10:05:06 Cary.xiao Exp $";

#include "cpLog.h"
#include "RtspRequest.hxx"

RtspRequest::RtspRequest()
    : RtspMsg(),
      myMethod(RTSP_NULL_MTHD),
      myHost(),
      myFilePath(),
      myBlocksize(0),
      myAccept()
{
}

RtspRequest::RtspRequest(const RtspRequest& src)
    : RtspMsg(src),
      myMethod(src.myMethod),
      myHost(src.myHost),
      myFilePath(src.myFilePath),
      myBlocksize(src.myBlocksize),
      myAccept(src.myAccept)
{
}

RtspRequest&
RtspRequest::operator=(const RtspRequest& src)
{
    if (&src != this)
    {
        RtspMsg::operator=(src);

        myMethod = src.myMethod;
        myHost = src.myHost;
        myFilePath = src.myFilePath;
        myBlocksize= src.myBlocksize;
        myAccept = src.myAccept;
    }
    return (*this);
}

u_int32_t RtspRequest::parse()
{
	if (myStartLine.length() == 0) 
	{
		myMethod = RTSP_NULL_MTHD;
		return RTSP_NULL_MTHD;
	}

	///add to support Response-like request	


    CharData strtLine;
    strtLine.set(myStartLine.getDataBuf(), myStartLine.length());
    CharDataParser charParser(&strtLine);
    CharData firstWord;
    charParser.getNextWord(&firstWord);
	if (firstWord.isEqualNoCase("RTSP",4)) 
	{
		myMethod = RTSP_RESPONSE_MTHD;
		myHost = "";
		myFilePath ="";
		//myProtocol ="";
		CharData proto;
		CharDataParser protoParser(&strtLine);
		protoParser.parseThru(&proto,' ');
		if ( proto.getLen()>0 )
		{
			Data protoData(proto.getPtr(),proto.getLen()-1);
			myProtocol = protoData;
		}
		else
		{
			myProtocol = "";
		}
		
		return myMethod;
	}


    myMethod = RtspUtil::getMethodInNumber(firstWord);
    cpLog(LOG_DEBUG_STACK, "Method= %d", myMethod);

    charParser.getThruSpaces(NULL);

    if (charParser.getCurChar() == '*')//if the host is only a star
    {
        myHost = "*";
        cpLog(LOG_DEBUG_STACK, "Host= %s", myHost.getData());
        cpLog(LOG_DEBUG_STACK, "FilePath= %s", myFilePath.getData());
    }
    else
    {
        //CharData rtsp;
        //charParser.getNextWord(&rtsp); //get thru "rtsp" or "rtspu"
		// CharData spec1;
		if (charParser.getThruSpecial(NULL, "rtspu://") || 
			charParser.getThruSpecial(NULL, "rtsp://")) 
		{		
			// charParser.getThruLength(NULL, 3); //get thru "://"
			CharData theHost;

			//first find the whole uri			
			CharData theURI;
			if( charParser.parseThru(&theURI,' ') )
			{
				//Now ,we get the uri
				
				//step 1,check if there is any

				CharDataParser parseURI(&theURI);
				if( parseURI.parseUntil(&theHost,'/') )
				{
					Data theHostData(theHost.getPtr(),theHost.getLen());
					myHost = theHostData;
					cpLog(LOG_DEBUG_STACK,"Host = %s",myHost.getData());
					charParser=parseURI;
				}
				else
				{
					CharDataParser parseURI2(&theURI);
					if(parseURI2.parseUntil(&theHost,' ' ))
					{
						Data theHostData(theHost.getPtr(),theHost.getLen());
						myHost = theHostData;
						cpLog(LOG_DEBUG_STACK,"Host = %s",myHost.getData());
						charParser=parseURI2;
					}
					else
					{
						//My God!Parse failed
						cpLog(LOG_ERR, "error read host.");
					}
				}
			}
		}
		else 
		{
			if (charParser.getCurChar() != '/') 
			{
				CharData theHost;
				if (charParser.parseUntil(&theHost, '/'))
				{
					Data theHostData(theHost.getPtr(), theHost.getLen());
					myHost = theHostData;
					cpLog(LOG_DEBUG_STACK, "Host= %s", myHost.getData());
				}
				else
				{
					cpLog(LOG_ERR, "error read host.");
				}
			}
		}

		if (charParser.getCurChar() == '/') 
		{
			charParser.parseThru(NULL, '/');
			CharData thePath;
			if (charParser.parseUntil(&thePath, charParser.myMaskEolSpace))
			{
				Data thePathData(thePath.getPtr(), thePath.getLen());
				myFilePath = thePathData;
				cpLog(LOG_DEBUG_STACK, "FilePath= %s", myFilePath.getData());
			}
			else
			{
				cpLog(LOG_ERR, "error read path.");
			}
		}
		else
		{
			
			myFilePath.erase();
		}

		charParser.getThruSpaces(NULL);

		CharData theProto;
		if (charParser.parseUntil(&theProto, charParser.myMaskEolSpace))
		{
			Data theProtoData(theProto.getPtr(), theProto.getLen());
            myProtocol = theProtoData;
            cpLog(LOG_DEBUG_STACK, "Protocol= %s", myProtocol.getData());
		}
    }
	
	return myMethod;
}

const u_int32_t
RtspRequest::getMethod()
{
    if (myMethod == RTSP_NULL_MTHD)
    {
		return parse();
		
		/*
        if (myStartLine.length() == 0)
            return myMethod;

        CharData strtLine;
        strtLine.set(myStartLine.getDataBuf(), myStartLine.length());
        CharDataParser charParser(&strtLine);
        CharData firstWord;
        charParser.getNextWord(&firstWord);
        myMethod = RtspUtil::getMethodInNumber(firstWord);
        cpLog(LOG_DEBUG_STACK, "Method= %d", myMethod);

        charParser.getThruSpaces(NULL);

        if (charParser.getCurChar() == '*')
        {
            myHost = "*";
            cpLog(LOG_DEBUG_STACK, "Host= %s", myHost.getData());
            cpLog(LOG_DEBUG_STACK, "FilePath= %s", myFilePath.getData());
        }
        else
        {
            //CharData rtsp;
            //charParser.getNextWord(&rtsp); //get thru "rtsp" or "rtspu"
			// CharData spec1;
			if (charParser.getThruSpecial(NULL, "rtspu://") || 
				charParser.getThruSpecial(NULL, "rtsp://")) {

				// charParser.getThruLength(NULL, 3); //get thru "://"
				CharData theHost;
				if (charParser.parseUntil(&theHost, '/'))
				{
					Data theHostData(theHost.getPtr(), theHost.getLen());
					myHost = theHostData;
					cpLog(LOG_DEBUG_STACK, "Host= %s", myHost.getData());
				}
				else
				{
					cpLog(LOG_ERR, "error read host.");
				}
			} else {
				if (charParser.getCurChar() != '/') {
					CharData theHost;
					if (charParser.parseUntil(&theHost, '/'))
					{
						Data theHostData(theHost.getPtr(), theHost.getLen());
						myHost = theHostData;
						cpLog(LOG_DEBUG_STACK, "Host= %s", myHost.getData());
					}
					else
					{
						cpLog(LOG_ERR, "error read host.");
					}
				}
			}

            charParser.parseThru(NULL, '/');
            CharData thePath;
            if (charParser.parseUntil(&thePath, charParser.myMaskEolSpace))
            {
                Data thePathData(thePath.getPtr(), thePath.getLen());
                myFilePath = thePathData;
                cpLog(LOG_DEBUG_STACK, "FilePath= %s", myFilePath.getData());
            }
            else
            {
                cpLog(LOG_ERR, "error read path.");
            }

			charParser.getThruSpaces(NULL);

			CharData theProto;
			if (charParser.parseUntil(&theProto, charParser.myMaskEolSpace))
			{
				Data theProtoData(theProto.getPtr(), theProto.getLen());
                myProtocol = theProtoData;
                cpLog(LOG_DEBUG_STACK, "Protocol= %s", myProtocol.getData());
			}
        }
		*/

        //TODO can add check for version here

    }

    return myMethod;
}


const Data&
RtspRequest::getAccept()
{
    if (myAccept.length() == 0)
    {
        myAccept = getHdrBodyData(RTSP_ACCEPT_HDR, true);
    }
    return myAccept;
}


const u_int32_t
RtspRequest::getBlocksize()
{
    if (myBlocksize == 0)
    {
        myBlocksize = getHdrBodyValue(RTSP_BLOCKSIZE_HDR);
    }
    return myBlocksize;
}

Data RtspRequest::encode()
{
	return Data((char*)NULL);
}

/* Local Variables: */
/* c-file-style: "stroustrup" */
/* indent-tabs-mode: nil */
/* c-file-offsets: ((access-label . -) (inclass . ++)) */
/* c-basic-offset: 4 */
/* End: */


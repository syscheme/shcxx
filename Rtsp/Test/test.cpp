// test.cpp : Defines the entry point for the console application.
//

#pragma warning(disable:4786)
#include "StringData.hxx"
#include "RtspMsg.hxx"
#include "RtspRequest.hxx"
#include "RtspResponse.hxx"
#include "RtspMsgParser.hxx"
#include "cpLog.h"

#define CRLF		"\r\n"

int main( int argc, char *argv[] )
{
    // cpLogSetPriority( LOG_DEBUG_STACK);

    char* msg = "DESCRIBE rtsp://www.vovida.com/vovida/abc.wav RTSP/1.0\r\nContent-Base : rtsp://10.0.0.1:554 user\r\nCseq: 1\r\nTransport: RTP/AVP;multicast;ttl=127;mode=\"PLAY\", RTP/AVP;unicast; client_port=3456-3457;mode=\"PLAY\"; destination=vovida.com ; source=10.0.1.1:3030 \r\nCon";
//    char* msg = "RTSP/1.0 200 OK\r\nConnection :  ab\r\nCseq: 1\r\nCon";
    int bytesNeedtoDecode = strlen( msg );
    int bytesDecoded = 0;
    printf( "%s\n(%d)\n", msg, bytesNeedtoDecode );

    Sptr<RtspMsg> savedMsg = 0;
    Sptr<RtspMsg> rtspMsg = 0;
    Sptr<RtspMsg> rtspMsg2 = 0;
    rtspMsg = RtspMsgParser::preParse( msg, bytesNeedtoDecode, bytesDecoded, savedMsg);
	RtspMsgCompletionType flag = rtspMsg->getCompletionFlag();

    assert(msg != 0);
    savedMsg = rtspMsg;

    char* msg2 = "tent-Length: 8\r\nRange: npt=.1-.3;time=\r\nAccept : application/sdp; level=2, application/rtsl \r\nSession:123\r\n\r\n12345678";


    bytesNeedtoDecode = strlen( msg2 );
    bytesDecoded = 0;
    printf( "%s\n(%d)\n", msg2, bytesNeedtoDecode );
    rtspMsg2 = RtspMsgParser::preParse( msg2, bytesNeedtoDecode, bytesDecoded, savedMsg);
	RtspMsgCompletionType flag2 = rtspMsg2->getCompletionFlag();

    assert(rtspMsg2 != 0);

    Sptr<RtspRequest> request = 0;
    Sptr<RtspResponse> response = 0;
    if (rtspMsg2->isRequest()) 
    {
        request.dynamicCast(rtspMsg2);

        printf(" this is a request.\n");
        printf( "the method is: %d\n", request->getMethod());
        printf( "the host is: %s\n", (request->getHost()).getData());
        printf( "the filepath is: %s\n", (request->getFilePath()).getData());
    }
    else 
    {
        response.dynamicCast(rtspMsg);
        printf("this is a response.\n");
        printf( "the statuscode is: %d\n", response->getStatusCode());
    }
    printf( "the cseq is: %s\n", (rtspMsg2->getCSeq()).getData());
    printf( "the sessionId is: %s\n", (rtspMsg2->getSessionId()).getData());
    printf( "the content-base is: %s\n", (rtspMsg2->getContentBase()).getData());
//    printf( "the accept is: %s\n", (rtspMsg->getAccept()).getData());

/*
    Data dummy;
    Data accept = rtspMsg->getAccept();
    if (accept.match("application/sdp", &dummy) != NOT_FOUND)
        printf("accept sdp\n");
    else
        printf("not accept sdp\n");
*/

	/*

    Sptr< RtspTransportSpec > spec = request->getTransport();
    if (spec == NULL)
        printf("cannot find suitable transport spec\n");
    else
    {
        printf("the transport hdr:\n");
        printf("          myIsTcp: %d\n", spec->myIsTcp);
        printf("    myIsMulticast: %d\n", spec->myIsMulticast);
        printf("       myIsRecord: %d\n", spec->myIsRecord);
        printf("       myIsAppend: %d\n", spec->myIsAppend);
        printf("    myDestination: %s\n", spec->myDestination.getData());
        printf("         mySource: %s\n", spec->mySource.getData());
        printf("    myClinetPortA: %d\n", spec->myClientPortA);
        printf("    myClientPortB: %d\n", spec->myClientPortB);
    }
	*/

    Sptr< RtspRangeHdr > range = request->getRange();
    if (range == NULL)
        printf("Range is not there\n");
    else
    {
        printf("the range hdr:\n");
        double x = range->getStartTime();
        double y = range->getStartTime();
        printf("   start time:%2.2f\n", x);
        printf("     end time:%2.2f\n", y);
    }

    if (rtspMsg2->hasBody())
    {
       printf("the msg body is: %s\n", (request->getMsgBody()).getData());
    }
    else
    {
       printf("there is no msgbody.");
    }

	static char msg3[] = 
		"SETUP /SeaChange/ITV?00000000.003805e1 RTSP/1.0"CRLF
		"CSeq:1"CRLF
		"SeaChange-MayNotify:"CRLF
		"SeaChange-Mod-Data:billing-id=0000005000;purchase-time=0000000000;time-remaining=0000000000;home-id=0000050000;smartcard-id=0000005000;purchase-id=0000000000;package-id=0000000000"CRLF
		"SeaChange-Server-Data:node-group-id=0000000001;smartcard-id=0000005000;device-id=01005E010205;supercas-id=0000000100"CRLF
		"SeaChange-Version:1"CRLF
		"Transport: RTP/AVP/TCP;multicast;destination=244.0.0.1;client_port=53764"CRLF CRLF;

    bytesNeedtoDecode = strlen( msg3 );
    bytesDecoded = 0;
    printf( "%s\n(%d)\n", msg3, bytesNeedtoDecode );
	Sptr<RtspMsg> savedMsg2 = 0;
    Sptr<RtspMsg> rtspMsg3 = RtspMsgParser::preParse( msg3, bytesNeedtoDecode, 
		bytesDecoded, savedMsg2);

	request.dynamicCast(rtspMsg3);
    Sptr< RtspTransportSpec > spec3 = request->getTransport();
	const Data& headers = request->getHeaders();
	printf("%s\n", headers.getData());
	
    if (spec3 == NULL)
        printf("cannot find suitable transport spec\n");
    else
    {
        printf("the transport hdr:\n");
        printf("          myIsTcp: %d\n", spec3->myIsTcp);
        printf("    myIsMulticast: %d\n", spec3->myIsMulticast);
        printf("       myIsRecord: %d\n", spec3->myIsRecord);
        printf("       myIsAppend: %d\n", spec3->myIsAppend);
        printf("    myDestination: %s\n", spec3->myDestination.getData());
        printf("         mySource: %s\n", spec3->mySource.getData());
        printf("    myClinetPortA: %d\n", spec3->myClientPortA);
        printf("    myClientPortB: %d\n", spec3->myClientPortB);
    }

    return 0;
}

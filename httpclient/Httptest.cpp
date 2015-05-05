// Httptest.cpp : Defines the entry point for the console application.
//

#include "HttpClient.h"
#include "FileLog.h"
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#endif

using namespace std;
using namespace ZQ::common;
//const char soap_endpoint[] = "http://websrv.cs.fsu.edu/~engelen/calcserver.cgi";
//const char soap_endpoint[] ="http://tech.163.com/04/1103/21/149VQUG60009rt.html";
//const char soap_endpoint[] = "http://192.168.81.112:180";
//const char soap_endpoint[] = "http://www.google.cn/";
const char soap_endpoint[] = "http://10.15.10.33:8080/GetVolumeInfo";

#ifdef WIN32
DWORD WINAPI ThreadProc(LPVOID lpParameter);
typedef struct http_info{
	HttpClient httpC;
	FILE* pfile;
}TH_INFO;
#endif


int main(int argc, char **argv)
{ 
/*
	//thread test	
	TH_INFO info[3];
	char path[256] = {0};
	for(int m = 0; m < 3; m++)
	{
		sprintf(path,"file[%d].log",m);
		info[m].pfile = fopen(path,"ab");
		if(info[m].pfile == NULL)
		{
			printf("open file error\n");
			return -1;
		}
	}
	HANDLE thhandle[3];
	//(SOAP_IO_BUFFER,SOAP_IO_BUFFER)

	for(int n = 0; n < 3; n++)
	{
		DWORD dwid;
		thhandle[n] = CreateThread(NULL,0,ThreadProc,(void*)&info[n],0,&dwid);
		if(thhandle[n] == NULL)
		{
			printf("create thread %d failed\n",n);
			break;
		}
		Sleep(500);
	}

	Sleep(10);
	WaitForMultipleObjects(3,thhandle,true,INFINITE);
	for(n = 0; n < 3; n++)
		CloseHandle(thhandle[n]);

*/
///////////////////////////////////////////////////////////////////////
	ZQ::common::FileLog flog("httptest.log",FileLog::L_DEBUG);

	HttpClient httpC = HttpClient(&flog);
	//init httpclient
	httpC.init(HttpClient::HTTP_IO_KEEPALIVE);
//int ncount = 10000;
//while(ncount--)
//{
//	Sleep(200);
//	char* content = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns=\"urn:calc\"><SOAP-ENV:Body SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><ns:add><a>10</a><b>20</b></ns:add></SOAP-ENV:Body></SOAP-ENV:Envelope>";
		
//	char* content = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<GetTransferStatus providerID=\"comcast.com\" assetID=\"BAAA0000000000018377\" volumeName=\"Philly.Warminster.volume1A\" />";

	char* content = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<GetVolumeInfo volumeName=\"70001\" />";
	//set additional header

	httpC.setHeader(NULL,NULL);
	httpC.setHeader("CSeq","1");

	size_t contentlen = strlen(content);
	//set timeout if want

	httpC.setRecvTimeout(20);
	
	if (httpC.httpConnect(soap_endpoint/*,HttpClient::HTTP_GET*/)
	 || httpC.httpSendContent(content,contentlen)
	 || httpC.httpEndSend()

	 )
	{
		printf("error string: %s\n",httpC.getErrorstr());
		httpC.uninit();
		return -1;
	}
	

	if (httpC.httpBeginRecv())
	{
		printf("errstr: %s\n",httpC.getErrorstr());
		httpC.uninit();
		return -1;
	}

	printf("\nmsg:\n%s\n",httpC.getMsg());
	printf("\nhead:\n");
	std::map<std::string,std::string>  head = httpC.getHeader(); 
	if(head.size())
	{
		std::map<std::string,std::string>::const_iterator it;
		for(it = head.begin();it != head.end(); it++)
			printf("%s: %s\n",it->first.c_str(),it->second.c_str());
	}
	while(!httpC.isEOF())
	{
		if(httpC.httpContinueRecv())
		{
			printf("continue recv error\n");
				return -1;
		}
	}

	if ( httpC.httpEndRecv() )
	{
		printf("httpclient end recv error\n");
		httpC.uninit();
		return -1;
	}
	char* pcontent = NULL;
	size_t slen = httpC.getContentLength();
	pcontent = (char*)malloc(slen+1);
	httpC.getContent(pcontent,slen+1);
	printf("\n%s", pcontent);
	free(pcontent);
	pcontent = NULL;
	printf("\n");

	httpC.setHeader(NULL,NULL);
	httpC.setHeader("CSeq","2");
	
	const char endpoint[] = "http://10.15.10.33:8080/GetContentInfo";
	content = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<GetContentInfo volumeName=\"70001\" />";
	contentlen = strlen(content);

	if (httpC.httpConnect(endpoint)
	 || httpC.httpSendContent(content,contentlen)
	 || httpC.httpEndSend()

	 )
	{
		printf("error num: %d\n",httpC.getErrorcode());
		httpC.uninit();
		return -1;
	}

	if (httpC.httpBeginRecv())
	{
		printf("msg:%s\n",httpC.getMsg());
		printf("error num: %d\n",httpC.getErrorcode());
		printf("errstr: %s\n",httpC.getErrorstr());
		httpC.uninit();
		return -1;
	}

	printf("\nmsg:\n%s\n",httpC.getMsg());
	printf("\nhead:\n");
	std::map<std::string,std::string>  head2 = httpC.getHeader(); 
	if(head2.size())
	{
		std::map<std::string,std::string>::const_iterator it;
		for(it = head2.begin();it != head2.end(); it++)
			printf("%s: %s\n",it->first.c_str(),it->second.c_str());
	}
	while(!httpC.isEOF())
	{
		if(httpC.httpContinueRecv())
		{
			printf("continue recv error\n");
				return -1;
		}
	}
	slen = httpC.getContentLength();
	pcontent = (char*)malloc(slen+1);
	httpC.getContent(pcontent,slen+1);
	printf("\n%s", pcontent);
	free(pcontent);
	pcontent = NULL;
//}
	httpC.uninit();

	flog.flush();
	return 0;
}

#ifdef WIN32
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	TH_INFO* pinfo = (TH_INFO*)lpParameter;
	if(pinfo == NULL)
		return -1;

	FILE* pf = pinfo->pfile;
	if(pf == NULL)
	{
		printf("file handle is null\n");
		return -1;
	}
	
	HttpClient* phttp = &pinfo->httpC;

	char *p = NULL;
	int ncount = 1000;
	char* pcontent = NULL;
	phttp->init(HttpClient::HTTP_IO_KEEPALIVE);
	while(ncount--)
	{
		Sleep(1000);

//		phttp->init(HttpClient::HTTP_IO_KEEPALIVE);

		char* conten = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<GetVolumeInfo volumeName=\"70001\" />";
						
		phttp->setHeader("a", "b");

		if (phttp->httpConnect(soap_endpoint, HttpClient::HTTP_POST)
		 || phttp->httpSendContent(conten,strlen(conten))
		 || phttp->httpEndSend()

		 )
		{
			printf("send content error\n");
			return -1;
		}
	

		if (phttp->httpBeginRecv())
		{
			phttp->uninit();
			return -1;
		}
		p = "msg:\n";
		fwrite(p,1,strlen(p),pf);

		p= phttp->getMsg();
		fwrite(p,1,strlen(p),pf);
		p = "\nhead:\n";
		fwrite(p,1,strlen(p),pf);

		std::map<std::string,std::string>  head = phttp->getHeader(); 
		if(head.size())
		{
			char kv[1024];			
			std::map<std::string,std::string>::const_iterator it;
			for(it = head.begin();it != head.end(); it++)
			{
				memset(kv,0,sizeof(head));
				sprintf(kv,"%s:%s\n",it->first.c_str(),it->second.c_str());
				fwrite(kv,1,strlen(kv),pf);
			}
		}
		p = "\n";
		fwrite(p,1,strlen(p),pf);

		while(!phttp->isEOF())
		{
			if(phttp->httpContinueRecv())
			{
				printf("continue recv error\n");
					return -1;
			}
		}
	
	
		if ( phttp->httpEndRecv() )
		{
			printf("end recv error\n");
			phttp->uninit();
			return -1;
		}
		size_t slen = phttp->getContentLength();		
		pcontent = (char*)malloc(slen+1);
		phttp->getContent(pcontent,slen);
		fwrite(pcontent,1,strlen(pcontent),pf);
		free(pcontent);
		pcontent = NULL;
		

		p = "\n";
		fwrite(p,1,strlen(p),pf);

		fflush(pf);

//		phttp->uninit();
	}
	phttp->uninit();

	return 0;
}
#endif

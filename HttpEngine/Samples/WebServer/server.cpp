#include <HttpEngine.h>
#include <FileLog.h>
#include <sstream>
#include <fstream>
#include <list>

#define Dummy_Page_Content "<html><head><title>dummy</title></head><body><h1>dummy</h1></body></html>"
static std::string int2str(int i)
{
    char buf[12] = {0};
    return itoa(i, buf, 10);
}
class ROFileHandler: public ZQHttp::IRequestHandler
{
private:
    std::string _cachedData;
public:
    explicit ROFileHandler(const char* rootPath)
    {
        _rootPath = rootPath;
    }
    /// on*() return true for continue, false for break.
    virtual bool onRequest(const ZQHttp::IRequest& req, ZQHttp::IResponse& resp)
    {
        if(0 == strcmp(req.uri(), "/_"))
        { // cached data
            // need lock here
            const char* cachedFile = req.queryArgument("c");
            if(cachedFile && '\0' != *cachedFile)
            { // add cache
                std::string cachedFilePath = _rootPath + (*cachedFile == '/' ? "" : "/") + cachedFile;
                _cachedData.clear();
                std::ifstream f;
                f.open(cachedFilePath.c_str(), std::ios::binary);
                char ch;
                while(f.get(ch))
                {
                    _cachedData += ch;
                }
            }
            resp.setHeader("Content-Size", int2str(_cachedData.size()).c_str());
            resp.setHeader("Content-Type", "application/octet-stream");
            resp.headerPrepared();
            if(_cachedData.empty())
            {
                resp.addContent(Dummy_Page_Content, strlen(Dummy_Page_Content ));
            }
            else
            {
                resp.addContent(_cachedData.data(), _cachedData.size());
            }
            resp.complete();
            return true;
        }

        std::string strFile = _rootPath + req.uri();
        //step 1 . open the file
        HANDLE hFile  = CreateFileA(	strFile.c_str(),				// file specification
								        GENERIC_READ,	// access mode 
								        FILE_SHARE_READ,	// share mode
								        0,							// security desc 
								        OPEN_EXISTING,	// how to create
								        0,							// file attributes
								        0);
        if( hFile == INVALID_HANDLE_VALUE)
        { // 404
            resp.sendDefaultErrorPage(404);
	        return false;
        }
        //step 2 .get file size
//        BY_HANDLE_FILE_INFORMATION fileInfo;
//        GetFileInformationByHandle(hFile,&fileInfo);
//        m_CreationTime = fileInfo.ftCreationTime;
        resp.setHeader("Content-Type", getMIMETypeOfFile(strFile).c_str());
        resp.headerPrepared();
        char	szBuf[4096];
        DWORD	dwRead = 0;
        do 
        {
	        ReadFile(hFile,szBuf,4096,&dwRead,NULL);
	        if(dwRead > 0)
	        {
                resp.addContent(szBuf,dwRead);
	        }

        } while(dwRead == 4096);
        CloseHandle(hFile);
        resp.complete();
        return true;
    }
    virtual bool onPostData(const ZQHttp::PostDataFrag& frag)
    {
        return true;
    }
    virtual bool onPostDataEnd()
    {
        return true;
    }
    virtual void onRequestEnd()
    {
    }

    // break the current request processing
    virtual void onBreak()
    {
    }
private:
    static std::string getMIMETypeOfFile(const std::string& filePath)
    { // get the MIME type of the target file base on the file ext
        std::string ext = filePath.substr(filePath.find_last_of('.'));

        static const char* typeTbl[] = {
            ".htm", "text/html",
            ".html", "text/html",
            ".js", "text/javascript",
            ".css", "text/css",
            ".txt", "text/plain",
            ".xml", "text/xml",
            ".gif", "image/gif",
            ".jpg", "image/jpeg",
            ".exe", "application/octet-stream"
        } ;

        for(size_t i = 0; i < sizeof(typeTbl) / sizeof(typeTbl[0]); i += 2)
        {
            if(0 == stricmp(typeTbl[i], ext.c_str()))
                return typeTbl[i + 1];
        }
        return "application/octet-stream";
    }
private:
    std::string _rootPath;
};

class DummyPage: public ZQHttp::IRequestHandler
{
public:
    /// on*() return true for continue, false for break.
    virtual bool onRequest(const ZQHttp::IRequest& req, ZQHttp::IResponse& resp)
    {
        resp.setHeader("Content-Type", "text/html");
        resp.setHeader("Content-Length", int2str(strlen(Dummy_Page_Content)).c_str());
        resp.headerPrepared();
        resp.addContent(Dummy_Page_Content, strlen(Dummy_Page_Content));
        resp.complete();
        return true;
    }
    virtual bool onPostData(const ZQHttp::PostDataFrag& frag)
    {
        return true;
    }
    virtual bool onPostDataEnd()
    {
        return true;
    }
    virtual void onRequestEnd()
    {
    }

    // break the current request processing
    virtual void onBreak()
    {
    }
};

class StaticPageFac: public ZQHttp::IRequestHandlerFactory
{
public:
    StaticPageFac(const char* webroot)
        :_roFileHandler(webroot)
    {
    }
    virtual ZQHttp::IRequestHandler* create(const char* uri)
    {
        if(0 == strcmp(uri, "/"))
            return &_dummyPage;
        else
            return &_roFileHandler;
    }
    virtual void destroy(ZQHttp::IRequestHandler* h) 
    {
    }
private:
    ROFileHandler _roFileHandler;
    DummyPage _dummyPage;
};

namespace Console
{
bool execute(std::ostringstream &output, const char* pExe, const char* pCmdLine)
{
	SECURITY_ATTRIBUTES   sa1,sa2;     
	HANDLE   hInputRead,hInputWrite;     
	HANDLE   hOutputRead,hOutputWrite;     

	sa1.nLength   =   sizeof(SECURITY_ATTRIBUTES);     
	sa1.lpSecurityDescriptor   =   NULL;     
	sa1.bInheritHandle   =   TRUE;     
	if(!CreatePipe(&hOutputRead,&hOutputWrite,&sa1,0))     
	{      
        glog(ZQ::common::Log::L_ERROR, "Create pile 1 error");
		return false;     
	}     

	sa2.nLength   =   sizeof(SECURITY_ATTRIBUTES);     
	sa2.lpSecurityDescriptor   =   NULL;     
	sa2.bInheritHandle   =   TRUE;     
	if(!CreatePipe(&hInputRead,&hInputWrite,&sa2,0))     
	{     
		glog(ZQ::common::Log::L_ERROR,"Create pile 2 error");     
		return false;     
	}     

	STARTUPINFO   si;     
	PROCESS_INFORMATION   pi;     
	si.cb   =   sizeof(STARTUPINFO);     
	GetStartupInfo(&si);     
	si.hStdError   =   hOutputWrite;     
	si.hStdOutput   =   hOutputWrite;     
	si.hStdInput   =   hInputRead;   
	si.wShowWindow   =   SW_HIDE;     
	si.dwFlags   =   STARTF_USESHOWWINDOW   |   STARTF_USESTDHANDLES;     
 
    //////////////////////////////////////////////////////////////////////////
    {
	    if(!CreateProcess(pExe,(LPSTR)pCmdLine,NULL,NULL,TRUE,NULL,NULL,NULL,&si,&pi))   
	    {
            DWORD err = GetLastError();
		    glog(ZQ::common::Log::L_ERROR,"CreateProcess failed.[error code = %u]", err);
		    return false;     
	    }     
    }

    //close unreferred handle of child process
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

	CloseHandle(hInputRead);   
	CloseHandle(hOutputWrite);    

    {
	    char   buffer[4096] = {0};     
	    DWORD   bytesRead;     
	    while(true)   
	    {   
		    memset(buffer,0,sizeof(buffer));
		    if(!ReadFile(hOutputRead,buffer,sizeof(buffer) - 1,&bytesRead,NULL))     
		    {   
			    //glog(Log::L_ERROR,"ReadFile function return false,read output pile error");
			    break;
		    }
		    output << buffer;     
	    }     
    }
	CloseHandle(hInputWrite);   
	CloseHandle(hOutputRead);   

	return true;
}

class CommandHandler: public ZQHttp::IRequestHandler
{
public:
    /// on*() return true for continue, false for break.
    virtual bool onRequest(const ZQHttp::IRequest& req, ZQHttp::IResponse& resp)
    {
        const char* cmd = req.queryArgument("c");
        if(NULL == cmd || '\0' == cmd)
        {
            resp.sendDefaultErrorPage(400);
            return false;
        }

        std::ostringstream buf;
        if(execute(buf, NULL, cmd))
        {
            resp.setHeader("Content-Type", "text/plain");
            resp.headerPrepared();
            std::string content;
            buf.str().swap(content);
            resp.addContent(content.data(), content.size());
            resp.complete();
            return true;
        }
        else
        {
            resp.sendDefaultErrorPage(500);
            return false;
        }
    }
    virtual bool onPostData(const ZQHttp::PostDataFrag& frag)
    {
        return true;
    }
    virtual bool onPostDataEnd()
    {
        return true;
    }
    virtual void onRequestEnd()
    {
    }

    // break the current request processing
    virtual void onBreak()
    {
    }
};
}
class ConsoleCommandFac: public ZQHttp::IRequestHandlerFactory
{
public:
    virtual ZQHttp::IRequestHandler* create(const char* uri)
    {
        return &_cmdHandler;
    }
    virtual void destroy(ZQHttp::IRequestHandler* h) 
    {
    }
private:
    Console::CommandHandler _cmdHandler;
};
class FileUploader: public ZQHttp::IRequestHandler
{
public:
    FileUploader(const std::string& dir)
        :_dir(dir), _resp(NULL)
    {
    }
    virtual bool onRequest(const ZQHttp::IRequest& req, ZQHttp::IResponse& resp)
    {
        _resp = &resp;
        return true;
    }
    virtual bool onPostData(const ZQHttp::PostDataFrag& frag)
    {
        if(frag.name && 0 == strcmp(frag.name, "f"))
        {
            std::string localPath;
            if(frag.fileName)
            {
                _fileNameC = frag.fileName;
                const char* name = strrchr(frag.fileName, '\\');
                if(name)
                {
                    localPath = _dir + name;
                }
                else
                {
                    localPath = _dir + "\\" + frag.fileName;
                }
            }
            if(localPath.empty())
                localPath = _dir + "\\uploaded" + int2str(::GetTickCount());

            _fileNameS = localPath;
            std::ofstream f;
            if(frag.partial)
            {
                f.open(localPath.c_str(), std::ios::app | std::ios::binary);
                f.write(frag.data, frag.len);
                f.close();
            }
            else
            {
                f.open(localPath.c_str(), std::ios::binary);
                f.write(frag.data, frag.len);
                f.close();
            }
        }
        return true;
    }
    virtual bool onPostDataEnd()
    {
        return true;
    }
    virtual void onRequestEnd()
    {
        std::ostringstream buf;
        buf << "<html><head><title>File uploaded</title></head><body>"
            << "<h1>File uploaded</h1>from: "
            << _fileNameC << "<br>to:  "
            << _fileNameS << "</body></html>";
        std::string content;
        content.swap(buf.str());
        _resp->setHeader("Content-Length", int2str(content.size()).c_str());
        _resp->headerPrepared();
        _resp->addContent(content.data(), content.size());
        _resp->complete();
    }

    // break the current request processing
    virtual void onBreak()
    {
    }
private:
    std::string _dir;
    ZQHttp::IResponse* _resp;
    std::string _fileNameC;
    std::string _fileNameS;
};

class FileUploaderFac: public ZQHttp::IRequestHandlerFactory
{
public:
    FileUploaderFac(const std::string& dir)
        :_dir(dir)
    {
    }
    virtual ZQHttp::IRequestHandler* create(const char* uri)
    {
        return new FileUploader(_dir);
    }
    virtual void destroy(ZQHttp::IRequestHandler* h) 
    {
        if(h)
            delete h;
    }
private:
    std::string _dir;
};

class SimpleOpt
{
public:
    void set(int argc, char* argv[])
    {
        _args.clear();
        for(int i = 0; i < argc; ++i)
        {
            _args.push_back(argv[i]);
        }
    }
    const char* get(const char* opt)
    {
        for(Args::const_iterator it = _args.begin(); it != _args.end(); ++it)
        {
            if(it->compare(opt) == 0)
            {
                ++it;
                if(it != _args.end())
                    return it->c_str();
                else
                    return NULL;
            }
        }
        return NULL;
    }
private:
    typedef std::list<std::string> Args;
    Args _args;
};
static void showUsage()
{
    printf("WebServer -r <root> -p <port> -l <log> -c <capacity>\n");
}


int
main(int argc, char* argv[])
{
    SimpleOpt sOpt;
    sOpt.set(argc, argv);
	ZQ::common::Log* plog = NULL;

    const char* root = sOpt.get("-r");
    if(NULL == root)
    {
        showUsage();
        return 0;
    }
    const char* port = sOpt.get("-p");
    if(NULL == port)
        port = "80";
    const char* logpath = sOpt.get("-l");
    if(logpath)
    {
        plog = new ZQ::common::FileLog(logpath, ZQ::common::Log::L_DEBUG);
		ZQ::common::setGlogger(plog);
    }
    const char* capacity = sOpt.get("-c");
    if(NULL == capacity)
        capacity = "1";
    ZQHttp::Engine e(glog);

    ConsoleCommandFac cmdFac;
    e.registerHandler("/exec", &cmdFac);


    FileUploaderFac fuFac(root);
    e.registerHandler("/upload", &fuFac);

    StaticPageFac spFac(root);
    e.registerHandler("/.*", &spFac);

    e.setEndpoint("0.0.0.0", port);
    e.setCapacity(atoi(capacity));
    e.start();

    Sleep(-1);
    e.stop();
	ZQ::common::setGlogger();
	if (plog)
		delete plog;

    return 0;
}

========================================================================
    STATIC LIBRARY : LibHttp Project Overview
========================================================================

AppWizard has created this LibHttp library project for you. 

No source files were created as part of your project.


LibHttp.vcproj
    This is the main project file for VC++ projects generated using an Application Wizard. 
    It contains information about the version of Visual C++ that generated the file, and 
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////

1. HttpServer.cpp:163  Ignore SIGPIPE signal
当服务器close一个连接时，若client端接着发数据。根据TCP协议的规定，会收到一个RST响应，client再往这个服务器发送数据时，系统会发出一个SIGPIPE信号给进程，告诉进程这个连接已经断开了，不要再写了

2.不能一直循环调用write可能造成接收数据乱码，必须要等到write的回调函数来了后再调用下一次write

3.connected_open()是后来添加的,在uv_tcp_open的基础上添加了uv_connection_init函数。

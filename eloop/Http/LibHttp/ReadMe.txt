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
��������closeһ������ʱ����client�˽��ŷ����ݡ�����TCPЭ��Ĺ涨�����յ�һ��RST��Ӧ��client���������������������ʱ��ϵͳ�ᷢ��һ��SIGPIPE�źŸ����̣����߽�����������Ѿ��Ͽ��ˣ���Ҫ��д��

2.����һֱѭ������write������ɽ����������룬����Ҫ�ȵ�write�Ļص��������˺��ٵ�����һ��write

3.connected_open()�Ǻ�����ӵ�,��uv_tcp_open�Ļ����������uv_connection_init������

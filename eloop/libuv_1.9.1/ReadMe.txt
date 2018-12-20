========================================================================
    STATIC LIBRARY : libuv_vs2005 Project Overview
========================================================================

AppWizard has created this libuv_vs2005 library project for you. 

No source files were created as part of your project.


libuv_vs2005.vcproj
    This is the main project file for VC++ projects generated using an Application Wizard. 
    It contains information about the version of Visual C++ that generated the file, and 
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////

libuv_vs2005修改内容

宏设置：
WIN32
_CRT_SECURE_NO_DEPRECATE
_CRT_NONSTDC_NO_DEPRECATE
_WIN32_WINNT=0x0600
_GNU_SOURCE
NDEBUG


1.core.c 
(1)在函数uv_poll_ex中
错误： C2275: 'ULONG' : illegal use of this type as an expression
怀疑比较老的版本变量的定义只能在开头，且用typedef所致
309 ULONG count;
  		》 unsigned long count;
310 ULONG i;			》
 unsigned long count;
312 uint64_t timeout_time;	》 unsigned __int64 timeout_time;

2.winapi.h
4637 添加
  typedef struct _OVERLAPPED_ENTRY {
	  unsigned int* lpCompletionKey;
	  LPOVERLAPPED lpOverlapped;
	  unsigned int* Internal;
	  unsigned long dwNumberOfBytesTransferred;
  } OVERLAPPED_ENTRY, *LPOVERLAPPED_ENTRY;

可以解决一堆相关类型的错误，可是不知道为什么在新建的文件中添加这个不起作用，在winapi.h中添加就可以

3.fs.c
fs__realpath_handle函数中VOLUME_NAME_DOS未定义
#define VOLUME_NAME_DOS  0x0 

4.tty.c
fs__realpath_handle函数中MAPVK_VK_TO_VSC未定义
#define MAPVK_VK_TO_VSC (0) 

5.util.c
uv_interface_addresses函数中IP_ADAPTER_UNICAST_ADDRESS_LH未定义的类型
783行加上如下定义
typedef struct _IP_ADAPTER_UNICAST_ADDRESS_LH {

	union {
		ULONGLONG Alignment;
		struct {
			ULONG Length;
			DWORD Flags;
		};
	};
	struct _IP_ADAPTER_UNICAST_ADDRESS_LH *Next;
	SOCKET_ADDRESS Address;
	IP_PREFIX_ORIGIN PrefixOrigin;
	IP_SUFFIX_ORIGIN SuffixOrigin;
	IP_DAD_STATE DadState;
	ULONG ValidLifetime;
	ULONG PreferredLifetime;
	ULONG LeaseLifetime;
	UINT8 OnLinkPrefixLength;

} IP_ADAPTER_UNICAST_ADDRESS_LH,*PIP_ADAPTER_UNICAST_ADDRESS_LH;

这些类型单独放到uv_newtype.h中定义会报错，把一些定义都放到了需要使用的位置，就正常了。猜测是头文件包含顺序的问题。


6.atomicops-inl.h
增加如下代码
static char __declspec(inline) uv__atomic_exchange_set(char volatile* target) {
//  return _InterlockedOr8(target, 1);
	__asm mov ecx, target
	__asm mov al, byte ptr [ecx]
	
	__asm lock or byte ptr [ecx], 1

}


//tty.c 1038

static long my_1_InterlockedOr(long volatile* target) {
	__asm mov ecx, target
	__asm mov al, byte ptr [ecx]
	
	__asm lock or byte ptr [ecx], 1

}


//tty.c 470

static long my_0_InterlockedOr(long volatile* target) {

	__asm mov ecx, target
	__asm mov al, byte ptr [ecx]
	
	__asm lock or byte ptr [ecx], 0

}
在async.c和tty.c两个文件中需要用到

7.编译Linux版本库的时候需要修改libuv.target.mk，否则编译eloop库时会提示could not read symbols: Bad value

Release版本在CFLAGS_Release变量中添加-shared和-fPIC


8.在atomicops-inl.h里的相关函数修改成汇编代码，编译64位不对,后来查资料 _InterlockedOr8函数需要头文件：#include <intrin.h>所以之前的修改都可以去掉，只需要该加上头文件
然后将tty.c中的InterlockedOr 改为_InterlockedOr

_InterlockedOr8介绍：https://msdn.microsoft.com/ar-sa/library/b11125ze(v=vs.100).aspx
https://msdn.microsoft.com/zh-cn/library/b11125ze.aspx
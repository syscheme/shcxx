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

libuv_vs2005�޸�����

�����ã�
WIN32
_CRT_SECURE_NO_DEPRECATE
_CRT_NONSTDC_NO_DEPRECATE
_WIN32_WINNT=0x0600
_GNU_SOURCE
NDEBUG


1.core.c 
(1)�ں���uv_poll_ex��
���� C2275: 'ULONG' : illegal use of this type as an expression
���ɱȽ��ϵİ汾�����Ķ���ֻ���ڿ�ͷ������typedef����
309 ULONG count;
  		�� unsigned long count;
310 ULONG i;			��
 unsigned long count;
312 uint64_t timeout_time;	�� unsigned __int64 timeout_time;

2.winapi.h
4637 ���
  typedef struct _OVERLAPPED_ENTRY {
	  unsigned int* lpCompletionKey;
	  LPOVERLAPPED lpOverlapped;
	  unsigned int* Internal;
	  unsigned long dwNumberOfBytesTransferred;
  } OVERLAPPED_ENTRY, *LPOVERLAPPED_ENTRY;

���Խ��һ��������͵Ĵ��󣬿��ǲ�֪��Ϊʲô���½����ļ����������������ã���winapi.h����ӾͿ���

3.fs.c
fs__realpath_handle������VOLUME_NAME_DOSδ����
#define VOLUME_NAME_DOS  0x0 

4.tty.c
fs__realpath_handle������MAPVK_VK_TO_VSCδ����
#define MAPVK_VK_TO_VSC (0) 

5.util.c
uv_interface_addresses������IP_ADAPTER_UNICAST_ADDRESS_LHδ���������
783�м������¶���
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

��Щ���͵����ŵ�uv_newtype.h�ж���ᱨ����һЩ���嶼�ŵ�����Ҫʹ�õ�λ�ã��������ˡ��²���ͷ�ļ�����˳������⡣


6.atomicops-inl.h
�������´���
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
��async.c��tty.c�����ļ�����Ҫ�õ�

7.����Linux�汾���ʱ����Ҫ�޸�libuv.target.mk���������eloop��ʱ����ʾcould not read symbols: Bad value

Release�汾��CFLAGS_Release���������-shared��-fPIC


8.��atomicops-inl.h�����غ����޸ĳɻ����룬����64λ����,���������� _InterlockedOr8������Ҫͷ�ļ���#include <intrin.h>����֮ǰ���޸Ķ�����ȥ����ֻ��Ҫ�ü���ͷ�ļ�
Ȼ��tty.c�е�InterlockedOr ��Ϊ_InterlockedOr

_InterlockedOr8���ܣ�https://msdn.microsoft.com/ar-sa/library/b11125ze(v=vs.100).aspx
https://msdn.microsoft.com/zh-cn/library/b11125ze.aspx
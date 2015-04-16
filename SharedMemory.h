/********************************************************************************************
* MOD-NAME      : SharedMemory.h
* LONG-NAME     : 
*
* AUTHOR        : Tony
* DEPARTMENT    : 
* TELEPHONE     : 
* CREATION-DATE : 
* SP-NO         : 
* FUNCTION      : 
* 
*********************************************************************************************/
#ifndef __MEMMAPFILE_H__
#define __MEMMAPFILE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SharedMemory.h : header file
#include <winnt.h>
// FOR THE SHARE MEMORY
#define SERVICENAME_LEN     80 //60
#define DEF_SHARED_SIZE		1024*3
#define VAR_NAME_LENGTH		SERVICENAME_LEN
#define VAR_DATA_LENGTH     150 // 120    

// FOR THE XMLFILEOPER
#define FILENAME_LEN        256
#define SERVICEOID_LEN      10

//structure creates header for a variable
typedef struct _tagValueHeader {
	TCHAR wszValueName[VAR_NAME_LENGTH];		//name of the variable
	TCHAR wszValueData[VAR_DATA_LENGTH];
} ValueHeader;

// SNMP Config Params
typedef struct _tagSNMPCONFIGDATA
{
	TCHAR szCompanyOID[VAR_DATA_LENGTH];
	TCHAR szDirectory[FILENAME_LEN];
	DWORD dwServiceOID;
}SNMPCONFIGDATA,*PSNMPCONFIGDATA;


class CSharedMemory;

typedef void (*PINITMEMORY)(CSharedMemory *);


// CSharedMemory class
class CSharedMemory 
{
public:
	CSharedMemory(const TCHAR *szName =_T("Global\\SharedData"), DWORD dwSize = DEF_SHARED_SIZE, PINITMEMORY InitMemoryProcedure = NULL,LPSECURITY_ATTRIBUTES lpsaAttributes = NULL);
	~CSharedMemory();

	
	BOOL SetSdMem(SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor);
	BOOL GetSdMem(SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded);
	BOOL SetSdSem(SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor);
	BOOL GetSdSem(SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded);
	BOOL SetSa(LPSECURITY_ATTRIBUTES lpsaAttributes);

	BOOL AddValue(const TCHAR *szName, void * pVarData = NULL);
	BOOL AddDwordValue(const TCHAR *szName, DWORD dVarData = 0);
	BOOL DeleteValue(const TCHAR *szName);

	BOOL ExistValue(const TCHAR *szName);
	BOOL IsCreated(void);

	DWORD GetVariablesCount(void);
	BOOL  GetValueInfo(DWORD dwIndex, ValueHeader *pVarInfo);

	BOOL  SetValue(const TCHAR *szName, void *bData,DWORD dwLength);
	BOOL  GetValue(const TCHAR *szName, void *bData, LPDWORD dwLength);

	BOOL  SetDwordValue(const TCHAR *szName, DWORD dwVal);
	DWORD GetDwordValue(const TCHAR *szName, DWORD dwDefVal = -1);

	void *GetPointer(const TCHAR *szName);

	BOOL InterlockedIncrement(const TCHAR *szName, LPLONG plNewVal = NULL);
	BOOL InterlockedDecrement(const TCHAR *szName, LPLONG plNewVal = NULL);
	BOOL InterlockedExchange(const TCHAR *szTargetName, LONG lNewVal, LPLONG plPrevValue = NULL);
	BOOL InterlockedTestExchange(const TCHAR *szTargetName, LONG lOldValue, LONG lNewValue, LPLONG plPrevValue = NULL);
	BOOL InterlockedCompareExchange(const TCHAR *szTargetName, LONG lExchange, LONG lComperand, LPLONG plIntiVal = NULL);
	BOOL InterlockedExchangeAdd(const TCHAR *szTargetName, LONG lIncrement, LPLONG plIntiVal = NULL);

	BOOL Write(BYTE *pbData, DWORD dwLength, DWORD dwOffset = 0);
	BOOL Read(BYTE *pbData, DWORD dwLength, DWORD dwOffset = 0);

	DWORD GetLastError(void);
	DWORD GetMemSize(void);
	BOOL  AmIFirst(void);

protected:
	void  SetLastError(DWORD dwErrCode);
	BOOL  CreateSa(void);
	BOOL  FindValue(const TCHAR *wszName,LPBYTE *pData, ValueHeader **pTmp);
	WCHAR *AllocateUnicodeStr(const char *szStr);
	TCHAR  m_hSharedName[MAX_PATH];
		
	DWORD m_dwLastError;		//last error issued from system by method of class

	CRITICAL_SECTION   m_csect;
	BOOL m_bFirst;				//TRUE, if this is first reference to sm

	BOOL m_bCreated;			//TRUE, if shared memory was created

	DWORD m_dwMemSize;			//size of allocated shared memory

	LPBYTE m_lpView;			//pointer to the begining of shared memory
	HANDLE m_hFileMap;			//handle of shared memory (mapped file)

	BOOL m_bSecPres;			//TRUE, if OS support security

	UCHAR m_SdEvent[SECURITY_DESCRIPTOR_MIN_LENGTH];
	SECURITY_ATTRIBUTES m_SecAtr;
	SID *m_pLogSid;
	SID *m_pSysSid;
	ACL *m_pNewAcl;
	SID *m_pEvrSid;
};

#endif //__MEMMAPFILE_H__

/********************************************************************************************
* MOD-NAME      : SharedMemory.cpp
* LONG-NAME     : 
*
* AUTHOR        : dony
* DEPARTMENT    : 
* TELEPHONE     : 
* CREATION-DATE : 
* SP-NO         : 
* FUNCTION      : 
* 
*********************************************************************************************/
#include "stdafx.h"
#include "SharedMemory.h"

/////////////////////////////////////////////////////////////////////////////
// CSharedMemory


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::CSharedMemory
 * DESCRIPTION          : 
 * RETURN TYPE          : 
 * ARGUMENT             : const TCHAR *szName
 * ARGUMENT             : DWORD dwSize
 * ARGUMENT             : PINITMEMORY InitMemoryProcedure
 * ARGUMENT             : LPSECURITY_ATTRIBUTES lpsaAttributes
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
CSharedMemory::CSharedMemory(const TCHAR *szName, DWORD dwSize, PINITMEMORY InitMemoryProcedure,LPSECURITY_ATTRIBUTES lpsaAttributes )
	:	m_dwLastError(0),
		m_pNewAcl(NULL),
		m_pLogSid(NULL),
		m_pSysSid(NULL),
		m_pEvrSid(NULL),
		m_bCreated(FALSE),
		m_bFirst(TRUE),
		m_bSecPres(FALSE)
{
	::InitializeCriticalSection(&m_csect);
	::EnterCriticalSection(&m_csect);
	try
	{
		do
		{
			OSVERSIONINFO osvi;
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			if (GetVersionEx(&osvi))
			{

				if (osvi.dwMajorVersion >= 3)
				{
					if ((osvi.dwMinorVersion == 51) ||
						(osvi.dwMinorVersion == 0)  ||
						(osvi.dwMinorVersion == 1)) 
					{
						m_bSecPres = TRUE;
					}
				}
			} 
			else 
			{
				SetLastError(::GetLastError());
				break;
			}

			//copy or create security descriptor for objects
			if (m_bSecPres)
			{
				if (lpsaAttributes) 
				{
					if (!SetSa(lpsaAttributes)) 
					{
						break;
					}
				} 
				else 
				{
					if (!CreateSa())
					{
						break;
					}
				}
			}

			_tcscpy(m_hSharedName,szName);
			*(m_hSharedName+_tcslen(szName)) = 0;

			
			
			// atempt to create file mapping in th page file ...
			m_hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE,m_bSecPres ? &m_SecAtr : NULL,PAGE_READWRITE,0,dwSize,szName);

			// ... it can already exist (it is not error) ...
			if (::GetLastError() == ERROR_ALREADY_EXISTS) 
			{
				m_bFirst = FALSE;		//remember, we are not first
			}

			// ... it can fail ...
			if (m_hFileMap == NULL)
			{
				SetLastError(::GetLastError());
				break;
			}

			// ... but if it did not, map a view
			m_lpView = (LPBYTE) MapViewOfFile(m_hFileMap,FILE_MAP_READ | FILE_MAP_WRITE,0,0,0);

			if (!m_lpView) {
				SetLastError(::GetLastError());
				break;
			}

			//allow usage only of already existing size, could be less then requested
			if (m_bFirst) 
			{
				::ZeroMemory(m_lpView,dwSize);
				m_dwMemSize = dwSize;
				CopyMemory(m_lpView+sizeof(DWORD),&m_dwMemSize,sizeof(DWORD));
			} 
			else
			{
				CopyMemory(&m_dwMemSize,m_lpView+sizeof(DWORD),sizeof(DWORD));
			}

			//if everything is OK up to now, the we have shared memory
			m_bCreated = TRUE;
			
			//and it can be initialized in one place
			if (InitMemoryProcedure)
			{
				
				InitMemoryProcedure(this);
				
			}
			
		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
}
/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::SetLastError
 * DESCRIPTION          : 
 * RETURN TYPE          : void 
 * ARGUMENT             : DWORD dwErrCode
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
void CSharedMemory::SetLastError(DWORD dwErrCode)			//this method and following macro will
{															//provide trace output of error description
	m_dwLastError = dwErrCode;								//with file name and line number where
	::SetLastError(dwErrCode);								//error occures - it works correctly
}															//only in this order


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::~CSharedMemory
 * DESCRIPTION          : 
 * RETURN TYPE          : 
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
CSharedMemory::~CSharedMemory()
{
	if (m_lpView) UnmapViewOfFile(m_lpView);
	if (m_hFileMap) CloseHandle(m_hFileMap);
	if (m_pNewAcl) HeapFree(GetProcessHeap(), 0, m_pNewAcl);
	if (m_pLogSid) FreeSid(m_pLogSid);
	if (m_pSysSid) FreeSid(m_pSysSid);
	if (m_pEvrSid) FreeSid(m_pEvrSid);
	::DeleteCriticalSection(&m_csect);
}

/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::GetLastError
 * DESCRIPTION          : 
 * RETURN TYPE          : DWORD 
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
DWORD CSharedMemory::GetLastError()
{
	return m_dwLastError;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::GetMemSize
 * DESCRIPTION          : 
 * RETURN TYPE          : DWORD 
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
DWORD CSharedMemory::GetMemSize()
{
	if (m_bCreated)
	{
		return m_dwMemSize;
	}
	else 
	{
		SetLastError(ERROR_OBJECT_NOT_FOUND);
		return 0;
	}
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::AmIFirst
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : void
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::AmIFirst(void)
{
	return m_bFirst;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::SetSdMem
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : SECURITY_INFORMATION SecurityInformation
 * ARGUMENT             : PSECURITY_DESCRIPTOR SecurityDescriptor
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::SetSdMem(SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor)
{
	BOOL rc = FALSE;

	if (m_bCreated)
	{

		rc = ::SetKernelObjectSecurity(m_hFileMap,SecurityInformation,SecurityDescriptor);

		if (!rc) 
		{
			SetLastError(::GetLastError());
		}
	}
	else 
	{
		SetLastError(ERROR_OBJECT_NOT_FOUND);
	}

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::GetSdMem
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : SECURITY_INFORMATION RequestedInformation
 * ARGUMENT             : PSECURITY_DESCRIPTOR pSecurityDescriptor
 * ARGUMENT             : DWORD nLength
 * ARGUMENT             : LPDWORD lpnLengthNeeded
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::GetSdMem(SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded)
{
	BOOL rc = FALSE;

	if (m_bCreated)
	{

		rc = ::GetKernelObjectSecurity(m_hFileMap,RequestedInformation,pSecurityDescriptor,nLength,lpnLengthNeeded);

		if (!rc) 
		{
			SetLastError(::GetLastError());
		}
	} 
	else
	{
		SetLastError(ERROR_OBJECT_NOT_FOUND);
	}

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::SetSdSem
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : SECURITY_INFORMATION SecurityInformation
 * ARGUMENT             : PSECURITY_DESCRIPTOR SecurityDescriptor
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::SetSdSem(SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor)
{
	BOOL rc = FALSE;

	if (m_bCreated) 
	{


		rc = ::SetKernelObjectSecurity((HANDLE)NULL,SecurityInformation,SecurityDescriptor);

		if (!rc) 
		{
			SetLastError(::GetLastError());
		}
	}
	else 
	{
		SetLastError(ERROR_OBJECT_NOT_FOUND);
	}

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::GetSdSem
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : SECURITY_INFORMATION RequestedInformation
 * ARGUMENT             : PSECURITY_DESCRIPTOR pSecurityDescriptor
 * ARGUMENT             : DWORD nLength
 * ARGUMENT             : LPDWORD lpnLengthNeeded
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::GetSdSem(SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded)
{
	BOOL rc = FALSE;

	if (m_bCreated) 
	{

		
		rc = ::GetKernelObjectSecurity(HANDLE(NULL),RequestedInformation,pSecurityDescriptor,nLength,lpnLengthNeeded);

		if (!rc)
		{
			SetLastError(::GetLastError());
		}
	}
	else
	{
		SetLastError(ERROR_OBJECT_NOT_FOUND);
	}

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::CreateSa
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::CreateSa()
{
	BOOL rc = FALSE;

	SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;

	DWORD cbAcl;

	PSECURITY_DESCRIPTOR psdNewSD=(PSECURITY_DESCRIPTOR)m_SdEvent;

	do {
		// allow access to user logged on interactivelly
		if (!AllocateAndInitializeSid(&sidAuth,1,SECURITY_INTERACTIVE_RID,0,0,0,0,0,0,0,(void **)&m_pLogSid)) 
		{
			SetLastError(::GetLastError());
			break;
		}
		// allow access to system
		if (!AllocateAndInitializeSid(&sidAuth,1,SECURITY_LOCAL_SYSTEM_RID,0,0,0,0,0,0,0,(void **)&m_pSysSid))
		{
			SetLastError(::GetLastError());
			break;
		}
		// allow access to services
		if (!AllocateAndInitializeSid(&sidAuth,1,SECURITY_SERVICE_RID,0,0,0,0,0,0,0,(void **)&m_pEvrSid)) 
		{
			SetLastError(::GetLastError());
			break;
		}

		cbAcl = GetLengthSid (m_pLogSid) + GetLengthSid (m_pSysSid) + GetLengthSid (m_pEvrSid) + 
				sizeof(ACL) + (3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
		m_pNewAcl = (PACL) HeapAlloc(GetProcessHeap(), 0, cbAcl);

		if (!m_pNewAcl) 
		{
			SetLastError(::GetLastError());
			break;
		}

		if (!InitializeAcl(m_pNewAcl, cbAcl, ACL_REVISION))
		{
			SetLastError(::GetLastError());
			break;
		}
		// allow everything to interractive user, system and services
		if (!AddAccessAllowedAce(	m_pNewAcl,
									ACL_REVISION,
									STANDARD_RIGHTS_ALL | 
									SPECIFIC_RIGHTS_ALL | 
									EVENT_ALL_ACCESS,
									m_pLogSid)) 
		{
			SetLastError(::GetLastError());
			break;
		}
		if (!AddAccessAllowedAce(	m_pNewAcl,
									ACL_REVISION,
									STANDARD_RIGHTS_ALL | 
									SPECIFIC_RIGHTS_ALL | 
									EVENT_ALL_ACCESS,
									m_pSysSid)) 
		{
			SetLastError(::GetLastError());
			break;
		}
		if (!AddAccessAllowedAce(	m_pNewAcl,
									ACL_REVISION,
									STANDARD_RIGHTS_ALL | 
									SPECIFIC_RIGHTS_ALL | 
									EVENT_ALL_ACCESS,
									m_pEvrSid))
		{
			SetLastError(::GetLastError());
			break;
		}

		if (!InitializeSecurityDescriptor(psdNewSD,SECURITY_DESCRIPTOR_REVISION)) 
		{
			SetLastError(::GetLastError());
			break;
		}

		if (!SetSecurityDescriptorDacl(psdNewSD,TRUE,m_pNewAcl,FALSE))
		{
			SetLastError(::GetLastError());
			break;
		}

		m_SecAtr.nLength = sizeof(m_SecAtr);
		m_SecAtr.lpSecurityDescriptor = psdNewSD;
		m_SecAtr.bInheritHandle = TRUE;

		rc = TRUE;

	} while (FALSE);

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::SetSa
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : LPSECURITY_ATTRIBUTES lpsaAttributes
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::SetSa(LPSECURITY_ATTRIBUTES lpsaAttributes)
{
	BOOL rc = FALSE;

	if (IsValidSecurityDescriptor(lpsaAttributes->lpSecurityDescriptor)) 
	{

		m_SecAtr.bInheritHandle = lpsaAttributes->bInheritHandle;
		m_SecAtr.nLength = lpsaAttributes->nLength;

		m_SecAtr.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)m_SdEvent;
		ZeroMemory(m_SecAtr.lpSecurityDescriptor,sizeof(m_SdEvent));

		InitializeSecurityDescriptor(m_SecAtr.lpSecurityDescriptor,SECURITY_DESCRIPTOR_REVISION);

		SID *pOwner;
		BOOL bOwnerDefaulted;

		GetSecurityDescriptorOwner(
			((SECURITY_DESCRIPTOR *)(lpsaAttributes->lpSecurityDescriptor)),
			(void**)&pOwner,
			&bOwnerDefaulted);

		SetSecurityDescriptorOwner(
			((SECURITY_DESCRIPTOR *)(m_SecAtr.lpSecurityDescriptor)),
			pOwner,
			bOwnerDefaulted);

		SID *pGroup;
		BOOL bGroupDefaulted;

		GetSecurityDescriptorGroup(
			((SECURITY_DESCRIPTOR *)(lpsaAttributes->lpSecurityDescriptor)),
			(void**)&pGroup,
			&bGroupDefaulted);

		SetSecurityDescriptorGroup(
			((SECURITY_DESCRIPTOR *)(m_SecAtr.lpSecurityDescriptor)),
			pGroup,
			bGroupDefaulted);

		BOOL bPresent = FALSE;
		ACL *pSacl;
		BOOL bSaclDefaulted;

		GetSecurityDescriptorSacl(
			((SECURITY_DESCRIPTOR *)(lpsaAttributes->lpSecurityDescriptor)),
			&bPresent,
			&pSacl,
			&bSaclDefaulted);

		SetSecurityDescriptorSacl(
			((SECURITY_DESCRIPTOR *)(m_SecAtr.lpSecurityDescriptor)),
			bPresent,
			pSacl,
			bSaclDefaulted);

		bPresent = FALSE;
		ACL *pDacl;
		BOOL bDaclDefaulted;

		GetSecurityDescriptorDacl(
			((SECURITY_DESCRIPTOR *)(lpsaAttributes->lpSecurityDescriptor)),
			&bPresent,
			&pDacl,
			&bDaclDefaulted);

		SetSecurityDescriptorDacl(
			((SECURITY_DESCRIPTOR *)(m_SecAtr.lpSecurityDescriptor)),
			bPresent,
			pDacl,
			bDaclDefaulted);

		rc = TRUE;

	} 
	else 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
	}

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::AddValue
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
  * AUTHOR               :  
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::AddValue(const TCHAR *szName, void * pVarData)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;

	::EnterCriticalSection(&m_csect);
	try
	{
	
		do
		{
			if (!m_bCreated)
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}

			if (sizeof(pVarData) == 0 ) 
			{
				SetLastError(ERROR_BAD_LENGTH);
				break;
			}

			if (_tcsclen(szName)+1 > VAR_NAME_LENGTH)
			{
				SetLastError(RPC_S_STRING_TOO_LONG);
				break;
			}

			wszName = (TCHAR *)szName;


			DWORD dwCurrentUssage = 2*sizeof(DWORD);
			DWORD dwCount;
			DWORD dwNewCount;
			BOOL bExist = FALSE;

			CopyMemory(&dwCount,m_lpView,sizeof(DWORD));
			dwNewCount = dwCount + 1;

			LPBYTE pData = m_lpView;
			pData += 2*sizeof(DWORD);
			ValueHeader *pTmp = NULL;

			if (dwCount == 0)
			{		// zeroize memory, if this is first variable in the memory
				ZeroMemory(pData,m_dwMemSize-(2*sizeof(DWORD)));
			}

			//find index and count current memory usage
			for (DWORD i = 0; i < dwCount; i++) 
			{
				pTmp = (ValueHeader *)pData;
				dwCurrentUssage += sizeof(ValueHeader);
				
				pData += sizeof(ValueHeader);

				if (_tcscmp(pTmp->wszValueName,wszName) == 0) 
				{ 
					bExist = TRUE;
					break;
				}
			}

			if (bExist)
			{
				SetLastError(ERROR_ALREADY_EXISTS);
				rc = TRUE; // 如果以存在，则不需要再添加变量
				break;
			}

			int iTest = sizeof(ValueHeader);

			if ((dwCurrentUssage + sizeof(ValueHeader) ) > m_dwMemSize)
			{
				SetLastError(ERROR_OUTOFMEMORY);
				break;
			}

			CopyMemory(m_lpView,&dwNewCount,sizeof(DWORD));

			ValueHeader newVal;
			ZeroMemory(&newVal,sizeof(ValueHeader));

			_tcscpy(newVal.wszValueName,wszName);
			_tcscpy(newVal.wszValueData,(TCHAR*)pVarData);

			if ( pVarData )
			{
				CopyMemory(pData,&newVal,sizeof(ValueHeader));
			}
			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
	return rc;
}

/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::AddDwordValue
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
 * ARGUMENT             : DWORD dwDefault
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
 BOOL CSharedMemory::AddDwordValue(const TCHAR *szName, DWORD dVarData)
{
    TCHAR szTmp[VAR_DATA_LENGTH];
	_stprintf(szTmp,_T("%d"),dVarData);
	return AddValue(szName,szTmp);
}
/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::DeleteValue
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::DeleteValue(const TCHAR *szName)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;

	::EnterCriticalSection(&m_csect);
	try
	{
	
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}

			wszName = (TCHAR *)szName;
			DWORD dwCount;
			BOOL bFound = FALSE;

			CopyMemory(&dwCount,m_lpView,sizeof(DWORD));

			LPBYTE pData = m_lpView;
			pData += 2*sizeof(DWORD);
			ValueHeader *pTmp = NULL;

			DWORD dwLengthToZero = 0;
			DWORD dwMoveLength = 0;
			DWORD dwNewCount = dwCount - 1;

			DWORD dwDeleted = 0;

			BYTE *memToZero = NULL;
			BYTE *moveFrom = NULL;
			BYTE *moveTo = NULL;

			for (DWORD i = 0; i < dwCount; i++)
			{

				pTmp = (ValueHeader *)pData;
				pData += sizeof(ValueHeader);


				dwMoveLength += sizeof(ValueHeader);



				if (_tcscmp(pTmp->wszValueName,wszName) == 0) 
				{ 

					bFound = TRUE;
					moveFrom = pData;
					moveTo = pData - ( sizeof(ValueHeader));

					dwMoveLength = sizeof(ValueHeader);

					dwDeleted = dwMoveLength;

					dwLengthToZero = sizeof(ValueHeader);
				}
			}

			if (!bFound) 
			{
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}

			memToZero = pData - dwDeleted;

			MoveMemory(moveTo,moveFrom,dwMoveLength);
			CopyMemory(m_lpView,&dwNewCount,sizeof(DWORD));
			ZeroMemory(memToZero,dwLengthToZero);

			
			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);


	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::GetVariablesCount
 * DESCRIPTION          : 
 * RETURN TYPE          : DWORD 
 * ARGUMENT             : void
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
DWORD CSharedMemory::GetVariablesCount(void)
{
	DWORD dwCount = 0;

	::EnterCriticalSection(&m_csect);
	try
	{
	
		do
		{
			if (!m_bCreated)
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}
			
			CopyMemory(&dwCount,m_lpView,sizeof(DWORD));
			
			m_dwLastError = 0;

		} while (FALSE);

	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);

	return dwCount;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::GetValueInfo
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : DWORD dwIndex
 * ARGUMENT             : ValueHeader *pVarInfo
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::GetValueInfo(DWORD dwIndex, ValueHeader *pVarInfo)
{
	BOOL rc = FALSE;

	::EnterCriticalSection(&m_csect);
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}

			DWORD dwCount;
			CopyMemory(&dwCount,m_lpView,sizeof(DWORD));

			LPBYTE pData = m_lpView;
			pData += 2*sizeof(DWORD);
			ValueHeader *pTmp = NULL;

			if (dwIndex >= dwCount)
			{
				SetLastError(ERROR_NOT_FOUND);
				break;
			}

			for (DWORD i = 0; i < dwIndex; i++)
			{
				pTmp = (ValueHeader *)pData;
				pData += sizeof(ValueHeader);
			}
			*pVarInfo = *((ValueHeader *)pData);

			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::FindValue
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *wszName
 * ARGUMENT             : LPBYTE *pData
 * ARGUMENT             : ValueHeader **pTmp
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::FindValue(const TCHAR *wszName,LPBYTE *pData, ValueHeader **pTmp)
{
	BOOL rc = FALSE;

	*pData += 2*sizeof(DWORD);

	DWORD dwCount;
	CopyMemory(&dwCount,m_lpView,sizeof(DWORD));

	*pData = m_lpView;
	*pData += 2*sizeof(DWORD);
	*pTmp = NULL;

	for (DWORD i = 0; i < dwCount; i++) 
	{
		*pTmp = (ValueHeader *)*pData;
		*pData += sizeof(ValueHeader);
		if (_tcscmp((*pTmp)->wszValueName,wszName) == 0) { 
			rc = TRUE;
			break;
		}
	}
	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : *CSharedMemory::AllocateUnicodeStr
 * DESCRIPTION          : 
 * RETURN TYPE          : TCHAR 
 * ARGUMENT             : const char *szStr
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
WCHAR *CSharedMemory::AllocateUnicodeStr(const char *szStr)
{
	WCHAR *wszStr = NULL;

	DWORD dwLn = strlen(szStr)+2;
	wszStr = new WCHAR[dwLn];
	if (!MultiByteToWideChar (CP_ACP, 0, szStr, -1, wszStr, dwLn)) 
	{
		SetLastError(::GetLastError());
		if (wszStr) delete [] wszStr;
		wszStr = NULL;
	}

	return wszStr;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::SetValue
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
 * ARGUMENT             : void *bData
  * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::SetValue(const TCHAR *szName, void *bData,DWORD dwLength)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;


	::EnterCriticalSection(&m_csect);
	try
	{
	
		do 
		{
			if (!m_bCreated)
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}

			if (dwLength == 0) 
			{
				SetLastError(ERROR_BAD_LENGTH);
				break;
			}

									
			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;

			wszName = (TCHAR *)szName;

			if (!FindValue(wszName,&pData,&pTmp))
			{
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}

			ZeroMemory(pData - sizeof(pTmp->wszValueData),sizeof(pTmp->wszValueData));
			CopyMemory(pData - sizeof(pTmp->wszValueData),bData,sizeof(pTmp->wszValueData));
						
			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
		
	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::GetValue
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
 * ARGUMENT             : void *bData
 * ARGUMENT             : LPDWORD dwLength
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::GetValue(const TCHAR *szName, void *bData, LPDWORD dwLength)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;

	::EnterCriticalSection(&m_csect);
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}

			
			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;

			wszName = (TCHAR *)szName;

			if (!FindValue(wszName,&pData,&pTmp)) 
			{
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}

			if (bData == NULL) 
			{
				*dwLength = sizeof(pTmp->wszValueData); 
				rc = TRUE;
				break;
			} 


			*dwLength = sizeof(pTmp->wszValueData); 
			CopyMemory(bData,pTmp->wszValueData,sizeof(pTmp->wszValueData));
			
			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::SetDwordValue
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
 * ARGUMENT             : DWORD dwVal
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::SetDwordValue(const TCHAR *szName, DWORD dwVal)
{
	TCHAR szTmp[VAR_DATA_LENGTH];
	_stprintf(szTmp,_T("%d"),dwVal);
	return SetValue(szName,szTmp,sizeof(szTmp));
}
/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::GetDwordValue
 * DESCRIPTION          : 
 * RETURN TYPE          : DWORD 
 * ARGUMENT             : const TCHAR *szName
 * ARGUMENT             : DWORD dwDefVal
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
DWORD CSharedMemory::GetDwordValue(const TCHAR *szName, DWORD dwDefVal)
{
	
	DWORD dwVal = 0;
	TCHAR szTmp[VAR_DATA_LENGTH];
	BOOL bReturn = GetValue(szName,szTmp,&dwVal);
	DWORD rc;
	rc = _ttol(szTmp);
	
	if (rc) 
	{
		return rc;
	}
	else 
	{
		return dwDefVal;
	}
}

/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : *CSharedMemory::GetPointer
 * DESCRIPTION          : 
 * RETURN TYPE          : void 
 * ARGUMENT             : const TCHAR *szName
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
void *CSharedMemory::GetPointer(const TCHAR *szName)
{
	void *rc = NULL;
	TCHAR *wszName = NULL;


	::EnterCriticalSection(&m_csect);
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}


			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;

			wszName = (TCHAR *)szName;


			if (!FindValue(wszName,&pData,&pTmp)) 
			{
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}

			rc = pData - sizeof(pTmp);

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::ExistValue
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::ExistValue(const TCHAR *szName)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;
	::EnterCriticalSection(&m_csect);
	try
	{
	
		do
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}
			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;

			wszName = (TCHAR *)szName;


			
			rc = FindValue(wszName,&pData,&pTmp);
			
			m_dwLastError = 0;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::IsCreated
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : void
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::IsCreated(void)
{
	return m_bCreated;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::InterlockedIncrement
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
 * ARGUMENT             : LPLONG plNewVal
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::InterlockedIncrement(const TCHAR *szName, LPLONG plNewVal)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;


	::EnterCriticalSection(&m_csect);
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}

			
			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;



			wszName = (TCHAR *)szName;


			if (!FindValue(wszName,&pData,&pTmp)) {
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}

			/*
			LONG lValue;
			if (pTmp->dwLength != sizeof(LONG)) {
				SetLastError(ERROR_INVALID_DATA);
				break;
			} 
					
			CopyMemory(&lValue,pData - pTmp->dwLength,pTmp->dwLength);
			
			CopyMemory(&lValue,pData -  pTmp->dwLength,pTmp->dwLength);

			lValue++;
			if (plNewVal) *plNewVal = lValue;
			CopyMemory(pData - pTmp->dwLength,&lValue,pTmp->dwLength);
			*/

			

			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
	

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::InterlockedDecrement
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szName
 * ARGUMENT             : LPLONG plNewVal
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::InterlockedDecrement(const TCHAR *szName, LPLONG plNewVal)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;


	::EnterCriticalSection(&m_csect);			
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}


			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;


			wszName = (TCHAR *)szName;


			if (!FindValue(wszName,&pData,&pTmp)) {
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}

			/*
			LONG lValue;
			if (pTmp->dwLength != sizeof(LONG)) {
				SetLastError(ERROR_INVALID_DATA);
				break;
			}

			CopyMemory(&lValue,pData - pTmp->dwLength,pTmp->dwLength);
			lValue--;
			if (plNewVal) *plNewVal = lValue;
			CopyMemory(pData - pTmp->dwLength,&lValue,pTmp->dwLength);
			*/
		
			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
	

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::InterlockedExchange
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szTargetName
 * ARGUMENT             : LONG lNewVal
 * ARGUMENT             : LPLONG plPrevValue
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::InterlockedExchange(const TCHAR *szTargetName, LONG lNewVal, LPLONG plPrevValue)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;


	::EnterCriticalSection(&m_csect);	
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}


			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;

			wszName = (TCHAR *)szTargetName;


			if (!FindValue(wszName,&pData,&pTmp)) {
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}

			/*
			//		LONG lValue;
			if (pTmp->dwLength != sizeof(LONG)) {
				SetLastError(ERROR_INVALID_DATA);
				break;
			}

			CopyMemory(&lValue,pData - pTmp->dwLength,pTmp->dwLength);
			if (plPrevValue) *plPrevValue = lValue;
			CopyMemory(pData - pTmp->dwLength,&lNewVal,pTmp->dwLength);
			*/
			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::InterlockedTestExchange
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szTargetName
 * ARGUMENT             : LONG lOldValue
 * ARGUMENT             : LONG lNewValue
 * ARGUMENT             : LPLONG plPrevValue
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::InterlockedTestExchange(const TCHAR *szTargetName,LONG lOldValue,LONG lNewValue, LPLONG plPrevValue)
{
	return InterlockedCompareExchange(szTargetName,lNewValue,lOldValue,plPrevValue);
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::InterlockedCompareExchange
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szTargetName
 * ARGUMENT             : LONG lExchange
 * ARGUMENT             : LONG lComperand
 * ARGUMENT             : LPLONG plIntiVal
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::InterlockedCompareExchange(const TCHAR *szTargetName, LONG lExchange, LONG lComperand, LPLONG plIntiVal)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;


	::EnterCriticalSection(&m_csect);
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}
		

			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;

	//		LONG lValue;

			wszName = (TCHAR *)szTargetName;


			if (!FindValue(wszName,&pData,&pTmp)) {
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}
			/*
			if (pTmp->dwLength != sizeof(LONG)) {
				SetLastError(ERROR_INVALID_DATA);
				break;
			}
			
			CopyMemory(&lValue,pData - pTmp->dwLength,pTmp->dwLength);

			if (lValue == lComperand) {
				CopyMemory(pData - pTmp->dwLength,&lExchange,pTmp->dwLength);
			}

			if (plIntiVal) *plIntiVal = lValue;
			*/

			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
	
	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::InterlockedExchangeAdd
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : const TCHAR *szTargetName
 * ARGUMENT             : LONG lIncrement
 * ARGUMENT             : LPLONG plIntiVal
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::InterlockedExchangeAdd(const TCHAR *szTargetName, LONG lIncrement, LPLONG plIntiVal)
{
	BOOL rc = FALSE;
	TCHAR *wszName = NULL;


	::EnterCriticalSection(&m_csect);
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}
		

			LPBYTE pData = NULL;
			ValueHeader *pTmp = NULL;

	//		LONG lValue;

			wszName = (TCHAR *)szTargetName;

			if (!FindValue(wszName,&pData,&pTmp)) {
				SetLastError(ERROR_FILE_NOT_FOUND);
				break;
			}

			/*
			if (pTmp->dwLength != sizeof(LONG)) {
				SetLastError(ERROR_INVALID_DATA);
				break;
			}

			CopyMemory(&lValue,pData - pTmp->dwLength,pTmp->dwLength);

			LONG lNew = lValue + lIncrement;
			CopyMemory(pData - pTmp->dwLength,&lNew,pTmp->dwLength);

			if (plIntiVal) *plIntiVal = lValue;
			*/

			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);
	
#ifndef _UNICODE
	if (wszName) delete [] wszName;
#endif

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::Write
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : BYTE *pbData
 * ARGUMENT             : DWORD dwLength
 * ARGUMENT             : DWORD dwOffset
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::Write(BYTE *pbData, DWORD dwLength, DWORD dwOffset)
{
	BOOL rc = FALSE;


	::EnterCriticalSection(&m_csect);
	try
	{
	
		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}
				
			DWORD dw;


			CopyMemory(&dw,m_lpView,sizeof(DWORD));

			//test - no variables
	//		if (dw != 0) { // modify by dony 
			if (dw == 0) 
			{
				SetLastError(ERROR_INVALID_DATA);
				break;
			}

			CopyMemory(&dw,m_lpView+sizeof(DWORD),sizeof(DWORD));

			if ((dwLength + dwOffset) > (dw-2*sizeof(DWORD)))
			{
				SetLastError(ERROR_BAD_LENGTH);
				break;
			}

			CopyMemory(m_lpView+((2*sizeof(DWORD))+dwOffset),pbData,dwLength);
		
			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);

	return rc;
}


/**************************************************************************************
 * PROG-NAME            : 
 * LONG-NAME            : 
 * FUNCTION             : CSharedMemory::Read
 * DESCRIPTION          : 
 * RETURN TYPE          : BOOL 
 * ARGUMENT             : BYTE *pbData
 * ARGUMENT             : DWORD dwLength
 * ARGUMENT             : DWORD dwOffset
 * AUTHOR               : 
 * DATE OF LAST UPDATE  : 
 * GLOBAL READ          :
 * GLOBAL WRITE         :
 * FUNCTION CALLS       :
 * ERROR-EXITS          :
 * SIDE-EFFECTS         :
 **************************************************************************************/
BOOL CSharedMemory::Read(BYTE *pbData, DWORD dwLength, DWORD dwOffset)
{
	BOOL rc = FALSE;

	::EnterCriticalSection(&m_csect);
	try
	{
	

		do 
		{
			if (!m_bCreated) 
			{
				SetLastError(ERROR_OBJECT_NOT_FOUND);
				break;
			}
		

			
			DWORD dw;

			CopyMemory(&dw,m_lpView,sizeof(DWORD));

			//test - no variables
	//		if (dw != 0) { // moidyfy by dony
			if (dw == 0) { 
				SetLastError(ERROR_INVALID_DATA);
				break;
			}

			CopyMemory(&dw,m_lpView+sizeof(DWORD),sizeof(DWORD));

			if ((dwLength + dwOffset) > (dw-2*sizeof(DWORD))) 
			{
				SetLastError(ERROR_BAD_LENGTH);
				break;
			}

			CopyMemory(pbData,m_lpView+((2*sizeof(DWORD))+dwOffset),dwLength);

			rc = TRUE;

		} while (FALSE);
	}
	catch(...)
	{
	}
	::LeaveCriticalSection(&m_csect);

	return rc;
}

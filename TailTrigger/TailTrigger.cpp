
#include <regex.h>
#include <assert.h>

#include "TailTrigger.h"
#include "formatstr.h"
#include "safemalloc.h"

#define DEF_NUM_BYTES_TO_READ 1024
#define MAX_LINE_LEN 1024
#define SIZE_OF_CIRC_POS 10

namespace ZQ
{
	namespace common
	{
		bool TailTriggerCheck(const char* const strSyntax, const char* strSource, std::vector<std::string>& arrParaments)
		{
			if (!(strSyntax&&strSource))
				return false;
			
			regex reg(strSyntax);
			cmatch cm;
			if (!query_match(strSource, strSource+strlen(strSource), cm, reg))
			{
				return false;
			}
			std::string strParament;
			arrParaments.clear();
			for (int i = 1; i < cm.size(); ++i)
			{
				strParament.assign(cm[i].first, cm[i].second);
				arrParaments.push_back(strParament);
			}
			return true;
		}
		
		void FormatCopy(const char* strSource, char*& strDest, int nMaxLen, const std::vector<std::string>& arrLineParaments)
		{
			if (NULL != strSource)
			{
				std::string strFormat = FormatString(strSource, arrLineParaments);
				safe_new(strDest, nMaxLen);
				strncpy(strDest, strFormat.c_str(), nMaxLen-1);
			}
		}
		
		int TailTrigger::ReadLine(HANDLE hFile, char* strLine, size_t zLen)
		{
			DWORD dwReadCount = 0;
			if (FALSE == ReadFile(hFile, strLine, zLen-1, &dwReadCount, NULL))
			{
				return 0;
			}
			if (0 == dwReadCount)
			{
				return 0;
			}
			for (DWORD i = 0; i < dwReadCount-1; ++i)
			{
				if (strLine[i] == char(13)&& strLine[i+1] == char(10))
				{
					strLine[i] = char(0);
					SetFilePointer(hFile, int(i)+2-int(dwReadCount), 0, FILE_CURRENT);
					break;
				}
			}
			return int(i-1);
		}
		
		int TailTrigger::ReadLineRange(HANDLE hFile, char* strLine, size_t zLen, size_t zTopRange)
		{
			int nRead = ReadLine(hFile, strLine, zLen);
			if (0 == zTopRange)
			{
				return nRead;
			}
			DWORD dwPos = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
			if (dwPos > zTopRange)
			{
				return 0;
			}
			return nRead;
		}

		TailTrigger::TailTrigger()
			:m_hMoniter(NULL), m_bRun(false)
		{
		}
		
		TailTrigger::TailTrigger(const char* strLogFile)
			:m_hMoniter(NULL), m_bRun(false)
		{
			strncpy(m_strLogFile, strLogFile, MAX_PATH);
		}
		
		TailTrigger::~TailTrigger()
		{
			stop();
		}
		
		void TailTrigger::setLogFile(const char* strLogFile)
		{ 
			strncpy(m_strLogFile, strLogFile, MAX_PATH);
		}
		
		const char* TailTrigger::getLogFile()
		{
			return m_strLogFile; 
		}
		
		DWORD __stdcall LogParserProc(void* pVoid)
		{
			assert(pVoid);
			
			TailTrigger* pLogReg = static_cast<TailTrigger*>(pVoid);
			pLogReg->ParserProc();
			
			return 0;
		}
		
		void TailTrigger::ParserProc(void)
		{
			HANDLE	hLogFile = NULL;
			char	szBuf[DEF_NUM_BYTES_TO_READ] = {0};
			DWORD	dwNumBytesRead;
			DWORD	dwCircpos;
			char	strLine[MAX_LINE_LEN] = {0};
			
			int	nLogFilePosition = -1;
			
			while (m_bRun)
			{
				hLogFile = CreateFile(m_strLogFile,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
				
				if (hLogFile == INVALID_HANDLE_VALUE)
				{	//can not moniter this file
					//printf("error file handle, cannot read log file: %s\n", m_cfgFile.Section[i].LogLocation);
					Sleep(500);
					continue;
				}
				if (ReadFile(hLogFile,&szBuf,SIZE_OF_CIRC_POS,&dwNumBytesRead,NULL))
				{
					sscanf(szBuf, "%ul",&dwCircpos);
					//printf("new position: %d\n", dwCircpos);
					if (nLogFilePosition == dwCircpos)
					{	//
						Sleep(500);
					}
					else if (nLogFilePosition < 0)
					{
						nLogFilePosition = dwCircpos;
					}
					else
					{
						if (dwCircpos > nLogFilePosition)
						{
							SetFilePointer(hLogFile, nLogFilePosition, 0, FILE_BEGIN);
							while (0 != ReadLineRange(hLogFile, strLine, MAX_LINE_LEN, dwCircpos))
							{
								std::vector<std::string> arrParaments;
								for (int i = 0; i < size(); ++i)
								{
									if (TailTriggerCheck(this->operator[](i).c_str(), strLine, arrParaments))
									{
										OnAction(m_strLogFile, this->operator[](i).c_str(), strLine, arrParaments);
									}
								}
							}
							nLogFilePosition = dwCircpos;
						}
						else if (dwCircpos < nLogFilePosition)
						{
							SetFilePointer(hLogFile, nLogFilePosition, 0, FILE_BEGIN);
							while (0 != ReadLineRange(hLogFile, strLine, MAX_LINE_LEN, 0))
							{
								std::vector<std::string> arrParaments;
								for (int i = 0; i < size(); ++i)
								{
									if (TailTriggerCheck(this->operator[](i).c_str(), strLine, arrParaments))
									{
										OnAction(m_strLogFile, this->operator[](i).c_str(), strLine, arrParaments);
									}
								}
							}
							SetFilePointer(hLogFile, 0, 0, FILE_BEGIN);
							while (0 != ReadLineRange(hLogFile, strLine, MAX_LINE_LEN, dwCircpos))
							{
								std::vector<std::string> arrParaments;
								for (int i = 0; i < size(); ++i)
								{
									if (TailTriggerCheck(this->operator[](i).c_str(), strLine, arrParaments))
									{
										OnAction(m_strLogFile, this->operator[](i).c_str(), strLine, arrParaments);
									}
								}
							}
							nLogFilePosition = dwCircpos;
						}
					}
				}
				CloseHandle(hLogFile);
				hLogFile = NULL;
			}
		}
		
		
		bool TailTrigger::run()
		{
			if (isRun())
			{
				return false;
			}
			m_bRun = true;
			
			m_hMoniter = CreateThread(NULL, 0, LogParserProc, static_cast<void*>(this), 
				0, NULL);
			return (NULL != m_hMoniter);
		}
		
		bool TailTrigger::stop()
		{
			m_bRun = false;
			Sleep(250);
			if (m_hMoniter)
			{
				Sleep(2000);	//wait for the thread closed by itself
			}
			
			//close the thread if it can not close by itself
			if (m_hMoniter)
			{
				if (FALSE == TerminateThread(m_hMoniter, 1))
				{
					return false;
				}
				CloseHandle(m_hMoniter);
				m_hMoniter = NULL;
			}
			return true;
		}
		
		bool TailTrigger::isRun()
		{
			return m_bRun;
		}
		
	}
}

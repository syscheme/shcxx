
//TailTrigger.h
//Daniel Wang
#ifndef _D_LIB_TAIL_TRIGGER_H_
#define _D_LIB_TAIL_TRIGGER_H_

#include <vector>
#include <string>
#include <windows.h>

namespace ZQ
{
	namespace common
	{
		DWORD __stdcall LogParserProc(void* pVoid);

		//TailTrigger
		class TailTrigger: public std::vector<std::string>
		{
		private:
			char	m_strLogFile[MAX_PATH];
			HANDLE	m_hMoniter;
			bool	m_bRun;

		protected:
			int ReadLine(HANDLE hFile, char* strLine, size_t zLen);

			int ReadLineRange(HANDLE hFile, char* strLine, size_t zLen, size_t zTopRange);

		public:
			TailTrigger();

			TailTrigger(const char* strLogFile);

			virtual ~TailTrigger();

			void setLogFile(const char* strLogFile);

			const char* getLogFile();

			virtual void OnAction(const char* strFileName, const char* strSyntax, const char* strLine, const std::vector<std::string>& arrParam) = 0;

			void ParserProc(void);


			bool run();

			bool stop();

			bool isRun();
		};
	}
}

#endif//_D_LIB_TAIL_TRIGGER_H_

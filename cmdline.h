//cmdline.h
//daniel wang

#ifndef _D_LIB_OPTPARSER_H_
#define _D_LIB_OPTPARSER_H_

#include <tchar.h>
#include <vector>
#include <string>

namespace ZQ
{
	namespace common
	{
		
#define MAX_PREFIX_COUNT 16;
		
		
		const int MAX_OPT_LEN = 256;
		
		class CmdLine
		{
		public:
			
			enum Arguments
			{
				no_argument=0,
					required_argument=1,
					optional_argument=2,
			};
			
			enum Order
			{
				REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
			};
			
			struct option 
			{
				const char *name;
				int  has_arg;
				int *flag;
				int val;
			};
			private:
				int		m_nErr;      /* if error message should be printed */
				int		m_nInd;      /* index into parent argv vector */
				int		m_nOpt;      /* character checked for validity */
				int		m_nReset;    /* reset getopt */
				char*	m_strArg;      /* argument associated with option */
				
				int		m_nFirst_nonopt;
				int		m_nLast_nonopt;
				char*	m_strNextchar;
				Order	m_ordering;
				char*	m_strPosixly_correct;
				int		m_n__getopt_initialized;
				
				
			private:
				const char *_getopt_initialize (int argc, char *const *argv, const char *optstring);
				int _getopt_internal (int argc,char *const *argv,const char *optstring,const struct option *longopts, int *longind,int long_only);
				void exchange (char **argv);
				
				void clear(void);
				int getopt (int argc,char *const *argv,const char *optstring);
				int getopt_long (int argc,char *const *argv,const char *options,const struct option *long_options,int *opt_index);
				const char* getarg(void);
				
			private:
				char	m_strOpt[MAX_OPT_LEN];
			public:
				CmdLine(const char *strOpt = NULL);
				~CmdLine();
				
				const char* GetOptString(void);
				void SetOptString(const char* strOpt);
				
				int ReadOpt(int argc,char *const *argv);
				const char* ReadArg(void);
		};
		
	}
}

#endif//_D_LIB_OPTPARSER_H_

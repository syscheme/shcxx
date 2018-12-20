#ifndef __ZQ_GETOPT_H__
#define __ZQ_GETOPT_H__

#include <string>
#include <vector>

namespace ZQ {
namespace common {

struct option 
{
	const char *name;
	int  has_arg;
	int *flag;
	int val;
};

#define POSOK(x)	(x!=std::string::npos)

class GetOpt
{
public:
	GetOpt(int argc, char **argv, const char *optstring, int long_only = 0);
	GetOpt(char *argv, const char *optstring, int long_only = 0);
	GetOpt(std::vector<std::string>& argv, const char *optstring, int long_only = 0);
	GetOpt(int argc, char **argv, const char *options, const struct option *long_options, int *opt_index, int long_only = 0);
	~GetOpt(void);

public:
	int hasNext();
	char opt(){ return _opt; };
	char * optarg(){ return _optarg; };

private:
	/** Callers store zero here to inhibit the error message `getopt' prints
	for unrecognized options.  */
	int   opterr;      /* if error message should be printed */

	/** Index in ARGV of the next element to be scanned.
	This is used for communication to and from the caller
	and for communication between successive calls to `getopt'.

	On entry to `getopt', zero means this is the first call; initialize.

	When `getopt' returns -1, this is the index of the first of the
	non-option elements that the caller should itself scan.

	Otherwise, `optind' communicates from one call to the next
	how much of ARGV has been scanned so far.  */
	int   optind;      /* index into parent argv vector */

	/** Set to an option character which was unrecognized.  */
	int   optopt;      /* character checked for validity */


	int   optreset;    /* reset getopt */

	/** For communication from `getopt' to the caller.
	When `getopt' finds an option that takes an argument,
	the argument value is returned here.
	Also, when `ordering' is RETURN_IN_ORDER,
	each non-option ARGV-element is returned here.  */
	char *_optarg;      /* argument associated with option */

	enum
	{
		no_argument=0,
		required_argument=1,
		optional_argument=2,
	};

	int first_nonopt;
	int last_nonopt;
	char *nextchar;
	enum
	{
		REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
	} ordering;
	char *posixly_correct;
	int __getopt_initialized;

private:
	int argc;
	char **argv;
	const char *optstring;
	const struct option *long_options;
	int *opt_index;
	int long_only;

	int _opt;

	const char *_getopt_initialize (int argc, char *const *argv, const char *optstring);
	void exchange (char **);
	int _getopt_internal();
	bool ParseCmdLine(const std::string& str ,std::vector<std::string>& result);
};
}//common
}//ZQ 

#endif //__ZQ_GETOPT_H__
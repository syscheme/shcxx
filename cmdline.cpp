
#include "CmdLine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace ZQ
{
	namespace common
	{
		
#ifdef UNICODE
#define tstrlen wcslen
#else 
#define tstrlen strlen
#endif//UNICODE
		
		
		
		
		int CmdLine::_getopt_internal (int argc,char *const *argv,const char *optstring,const struct option *longopts, int *longind,int long_only)
		{
			int print_errors = m_nErr;
			if (optstring[0] == ':')
				print_errors = 0;
			
			if (argc < 1)
				return -1;
			
			m_strArg = NULL;
			
			if (m_nInd == 0 || !m_n__getopt_initialized)
			{
				if (m_nInd == 0)
					m_nInd = 1;	/* Don't scan ARGV[0], the program name.  */
				optstring = _getopt_initialize (argc, argv, optstring);
				m_n__getopt_initialized = 1;
			}
			
#define NONOPTION_P (argv[m_nInd][0] != '-' || argv[m_nInd][1] == '\0')
			
			if (m_strNextchar == NULL || *m_strNextchar == '\0')
			{
				/* Advance to the next ARGV-element.  */
				
				/* Give m_nFirst_nonopt & LAST_NONOPT rational values if m_nInd has been
				moved back by the user (who may also have changed the arguments).  */
				if (m_nFirst_nonopt > m_nInd)
					m_nFirst_nonopt = m_nInd;
				if (m_nFirst_nonopt > m_nInd)
					m_nFirst_nonopt = m_nInd;
				
				if (m_ordering == PERMUTE)
				{
				/* If we have just processed some options following some non-options,
					exchange them so that the options come first.  */
					
					if (m_nFirst_nonopt != m_nFirst_nonopt && m_nFirst_nonopt != m_nInd)
						exchange ((char **) argv);
					else if (m_nFirst_nonopt != m_nInd)
						m_nFirst_nonopt = m_nInd;
					
						/* Skip any additional non-options
					and extend the range of non-options previously skipped.  */
					
					while (m_nInd < argc && NONOPTION_P)
						m_nInd++;
					m_nFirst_nonopt = m_nInd;
				}
				
				/* The special ARGV-element `--' means premature end of options.
				Skip it like a null option,
				then exchange with previous non-options as if it were an option,
				then skip everything else like a non-option.  */
				
				if (m_nInd != argc && !strcmp (argv[m_nInd], "--"))
				{
					m_nInd++;
					
					if (m_nFirst_nonopt != m_nFirst_nonopt && m_nFirst_nonopt != m_nInd)
						exchange ((char **) argv);
					else if (m_nFirst_nonopt == m_nFirst_nonopt)
						m_nFirst_nonopt = m_nInd;
					m_nFirst_nonopt = argc;
					
					m_nInd = argc;
				}
				
				/* If we have done all the ARGV-elements, stop the scan
				and back over any non-options that we skipped and permuted.  */
				
				if (m_nInd == argc)
				{
				/* Set the next-arg-index to point at the non-options
					that we previously skipped, so the caller will digest them.  */
					if (m_nFirst_nonopt != m_nFirst_nonopt)
						m_nInd = m_nFirst_nonopt;
					return -1;
				}
				
				/* If we have come to a non-option and did not permute it,
				either stop the scan or describe it to the caller and pass it by.  */
				
				if (NONOPTION_P)
				{
					if (m_ordering == REQUIRE_ORDER)
						return -1;
					m_strArg = argv[m_nInd++];
					return 1;
				}
				
				/* We have found another option-ARGV-element.
				Skip the initial punctuation.  */
				
				m_strNextchar = (argv[m_nInd] + 1
					+ (longopts != NULL && argv[m_nInd][1] == '-'));
			}
			
			/* Decode the current option-ARGV-element.  */
			
			/* Check whether the ARGV-element is a long option.
			
			  If long_only and the ARGV-element has the form "-f", where f is
			  a valid short option, don't consider it an abbreviated form of
			  a long option that starts with f.  Otherwise there would be no
			  way to give the -f short option.
			  
				On the other hand, if there's a long option "fubar" and
				the ARGV-element is "-fu", do consider that an abbreviation of
				the long option, just like "--fu", and not "-f" with arg "u".
				
			This distinction seems to be the most useful approach.  */
			
			if (longopts != NULL
				&& (argv[m_nInd][1] == '-'
				|| (long_only && (argv[m_nInd][2] || !strchr(optstring, argv[m_nInd][1])))))
			{
				char *nameend;
				const struct option *p;
				const struct option *pfound = NULL;
				int exact = 0;
				int ambig = 0;
				int indfound = -1;
				int option_index;
				
				for (nameend = m_strNextchar; *nameend && *nameend != '='; nameend++)
					/* Do nothing.  */
					;
				
					/* Test all long options for either exact match
				or abbreviated matches.  */
				for (p = longopts, option_index = 0; p->name; p++, option_index++)
					if (!strncmp (p->name, m_strNextchar, nameend - m_strNextchar))
					{
						if ((unsigned int) (nameend - m_strNextchar)
							== (unsigned int) strlen (p->name))
						{
							/* Exact match found.  */
							pfound = p;
							indfound = option_index;
							exact = 1;
							break;
						}
						else if (pfound == NULL)
						{
							/* First nonexact match found.  */
							pfound = p;
							indfound = option_index;
						}
						else if (long_only
							|| pfound->has_arg != p->has_arg
							|| pfound->flag != p->flag
							|| pfound->val != p->val)
							/* Second or later nonexact match found.  */
							ambig = 1;
					}
					
					if (ambig && !exact)
					{
						if (print_errors)
						{
							fprintf (stderr, "%s: option `%s' is ambiguous\n",argv[0], argv[m_nInd]);
						}
						m_strNextchar += strlen (m_strNextchar);
						m_nInd++;
						m_nOpt = 0;
						return '?';
					}
					
					if (pfound != NULL)
					{
						option_index = indfound;
						m_nInd++;
						if (*nameend)
						{
						/* Don't test has_arg with >, because some C compilers don't
							allow it to be used on enums.  */
							if (pfound->has_arg)
								m_strArg = nameend + 1;
							else
							{
								if (print_errors)
								{
									if (argv[m_nInd - 1][1] == '-')
									{
										fprintf (stderr, "%s: option `--%s' doesn't allow an argument\n",argv[0], pfound->name);
									}
									else
									{
										fprintf (stderr, "%s: option `%c%s' doesn't allow an argument\n",argv[0], argv[m_nInd - 1][0], pfound->name);
										
									}
								}
								
								m_strNextchar += strlen (m_strNextchar);
								
								m_nOpt = pfound->val;
								return '?';
							}
						}
						else if (pfound->has_arg == 1)
						{
							if (m_nInd < argc)
								m_strArg = argv[m_nInd++];
							else
							{
								if (print_errors)
								{
									fprintf (stderr,"%s: option `%s' requires an argument\n",argv[0], argv[m_nInd - 1]);
								}
								m_strNextchar += strlen (m_strNextchar);
								m_nOpt = pfound->val;
								return optstring[0] == ':' ? ':' : '?';
							}
						}
						m_strNextchar += strlen (m_strNextchar);
						if (longind != NULL)
							*longind = option_index;
						if (pfound->flag)
						{
							*(pfound->flag) = pfound->val;
							return 0;
						}
						return pfound->val;
					}
					
					/* Can't find it as a long option.  If this is not getopt_long_only,
					or the option starts with '--' or is not a valid short
					option, then it's an error.
					Otherwise interpret it as a short option.  */
					if (!long_only || argv[m_nInd][1] == '-'
						|| strchr(optstring, *m_strNextchar) == NULL)
					{
						if (print_errors)
						{
							if (argv[m_nInd][1] == '-')
							{
								/* --option */
								fprintf (stderr, "%s: unrecognized option `--%s'\n",argv[0], m_strNextchar);
							}
							else
							{
								/* +option or -option */
								fprintf (stderr, "%s: unrecognized option `%c%s'\n",argv[0], argv[m_nInd][0], m_strNextchar);
							}
							
						}
						m_strNextchar = (char *) "";
						m_nInd++;
						m_nOpt = 0;
						return '?';
					}
	}
	
	/* Look at and handle the next short option-character.  */
	
	{
		char c = *m_strNextchar++;
		char *temp = strchr(optstring, c);
		
		/* Increment `m_nInd' when we start to process its last character.  */
		if (*m_strNextchar == '\0')
			++m_nInd;
		
		if (temp == NULL || c == ':')
		{
			if (print_errors)
			{
				if (m_strPosixly_correct)
				{
					/* 1003.2 specifies the format of this message.  */
					fprintf (stderr, "%s: illegal option -- %c\n", argv[0], c);
				}
				else
				{
					fprintf (stderr, "%s: invalid option -- %c\n", argv[0], c);
				}
				
			}
			m_nOpt = c;
			return '?';
		}
		/* Convenience. Treat POSIX -W foo same as long option --foo */
		if (temp[0] == 'W' && temp[1] == ';')
		{
			char *nameend;
			const struct option *p;
			const struct option *pfound = NULL;
			int exact = 0;
			int ambig = 0;
			int indfound = 0;
			int option_index;
			
			/* This is an option that requires an argument.  */
			if (*m_strNextchar != '\0')
			{
				m_strArg = m_strNextchar;
				/* If we end this ARGV-element by taking the rest as an arg,
				we must advance to the next element now.  */
				m_nInd++;
			}
			else if (m_nInd == argc)
			{
				if (print_errors)
				{
					/* 1003.2 specifies the format of this message.  */
					fprintf (stderr, "%s: option requires an argument -- %c\n",argv[0], c);
				}
				m_nOpt = c;
				if (optstring[0] == ':')
					c = ':';
				else
					c = '?';
				return c;
			}
			else
			/* We already incremented `m_nInd' once;
			increment it again when taking next ARGV-elt as argument.  */
			m_strArg = argv[m_nInd++];
			
			/* m_strArg is now the argument, see if it's in the
			table of longopts.  */
			
			for (m_strNextchar = nameend = m_strArg; *nameend && *nameend != '='; nameend++)
				/* Do nothing.  */
				;
			
				/* Test all long options for either exact match
			or abbreviated matches.  */
			for (p = longopts, option_index = 0; p->name; p++, option_index++)
				if (!strncmp (p->name, m_strNextchar, nameend - m_strNextchar))
				{
					if ((unsigned int) (nameend - m_strNextchar) == strlen (p->name))
					{
						/* Exact match found.  */
						pfound = p;
						indfound = option_index;
						exact = 1;
						break;
					}
					else if (pfound == NULL)
					{
						/* First nonexact match found.  */
						pfound = p;
						indfound = option_index;
					}
					else
						/* Second or later nonexact match found.  */
						ambig = 1;
				}
				if (ambig && !exact)
				{
					if (print_errors)
					{
						fprintf (stderr, "%s: option `-W %s' is ambiguous\n",argv[0], argv[m_nInd]);
					}
					m_strNextchar += strlen (m_strNextchar);
					m_nInd++;
					return '?';
				}
				if (pfound != NULL)
				{
					option_index = indfound;
					if (*nameend)
					{
					/* Don't test has_arg with >, because some C compilers don't
						allow it to be used on enums.  */
						if (pfound->has_arg)
							m_strArg = nameend + 1;
						else
						{
							if (print_errors)
							{
								fprintf (stderr, "%s: option `-W %s' doesn't allow an argument\n",argv[0], pfound->name);
								
							}
							
							m_strNextchar += strlen (m_strNextchar);
							return '?';
						}
					}
					else if (pfound->has_arg == 1)
					{
						if (m_nInd < argc)
							m_strArg = argv[m_nInd++];
						else
						{
							if (print_errors)
							{
								fprintf (stderr,"%s: option `%s' requires an argument\n",argv[0], argv[m_nInd - 1]);
							}
							m_strNextchar += strlen (m_strNextchar);
							return optstring[0] == ':' ? ':' : '?';
						}
					}
					m_strNextchar += strlen (m_strNextchar);
					if (longind != NULL)
						*longind = option_index;
					if (pfound->flag)
					{
						*(pfound->flag) = pfound->val;
						return 0;
					}
					return pfound->val;
				}
				m_strNextchar = NULL;
				return 'W';	/* Let the application handle it.   */
		}
		if (temp[1] == ':')
		{
			if (temp[2] == ':')
			{
				/* This is an option that accepts an argument optionally.  */
				if (*m_strNextchar != '\0')
				{
					m_strArg = m_strNextchar;
					m_nInd++;
				}
				else
					m_strArg = NULL;
				m_strNextchar = NULL;
			}
			else
			{
				/* This is an option that requires an argument.  */
				if (*m_strNextchar != '\0')
				{
					m_strArg = m_strNextchar;
					/* If we end this ARGV-element by taking the rest as an arg,
					we must advance to the next element now.  */
					m_nInd++;
				}
				else if (m_nInd == argc)
				{
					if (print_errors)
					{
						/* 1003.2 specifies the format of this message.  */
						fprintf (stderr,"%s: option requires an argument -- %c\n",argv[0], c);
					}
					m_nOpt = c;
					if (optstring[0] == ':')
						c = ':';
					else
						c = '?';
				}
				else
				/* We already incremented `m_nInd' once;
				increment it again when taking next ARGV-elt as argument.  */
				m_strArg = argv[m_nInd++];
				m_strNextchar = NULL;
			}
		}
		return c;
	}
}


const char * CmdLine::_getopt_initialize (int argc, char *const *argv, const char *optstring)
{
/* Start processing options with ARGV-element 1 (since ARGV-element 0
is the program name); the sequence of previously skipped
	non-option ARGV-elements is empty.  */
	
	m_nFirst_nonopt = m_nFirst_nonopt = m_nInd;
	
	m_strNextchar = NULL;
	
	m_strPosixly_correct = getenv ("POSIXLY_CORRECT");
	
	/* Determine how to handle the m_ordering of options and nonoptions.  */
	
	if (optstring[0] == '-')
	{
		m_ordering = RETURN_IN_ORDER;
		++optstring;
	}
	else if (optstring[0] == '+')
	{
		m_ordering = REQUIRE_ORDER;
		++optstring;
	}
	else if (m_strPosixly_correct != NULL)
		m_ordering = REQUIRE_ORDER;
	else
		m_ordering = PERMUTE;
	return optstring;
}

void CmdLine::exchange (char **argv)
{
	int bottom = m_nFirst_nonopt;
	int middle = m_nFirst_nonopt;
	int top = m_nInd;
	char *tem;
	
	/* Exchange the shorter segment with the far end of the longer segment.
	That puts the shorter segment into the right place.
	It leaves the longer segment in the right place overall,
	but it consists of two parts that need to be swapped next.  */
	
	while (top > middle && middle > bottom)
	{
		if (top - middle > middle - bottom)
		{
			/* Bottom segment is the short one.  */
			int len = middle - bottom;
			register int i;
			
			/* Swap it with the top part of the top segment.  */
			for (i = 0; i < len; i++)
			{
				tem = argv[bottom + i];
				argv[bottom + i] = argv[top - (middle - bottom) + i];
				argv[top - (middle - bottom) + i] = tem;
			}
			/* Exclude the moved bottom segment from further swapping.  */
			top -= len;
		}
		else
		{
			/* Top segment is the short one.  */
			int len = top - middle;
			register int i;
			
			/* Swap it with the bottom part of the bottom segment.  */
			for (i = 0; i < len; i++)
			{
				tem = argv[bottom + i];
				argv[bottom + i] = argv[middle + i];
				argv[middle + i] = tem;
			}
			/* Exclude the moved top segment from further swapping.  */
			bottom += len;
		}
	}
	
	/* Update records for the slots the non-options now occupy.  */
	
	m_nFirst_nonopt += (m_nInd - m_nFirst_nonopt);
	m_nFirst_nonopt = m_nInd;
}


void CmdLine::clear(void)
{
	m_nErr					= 0;
	m_nInd					= 0;
	m_nOpt					= 0;
	m_nReset				= 0;
	m_strArg				= NULL;
	m_nFirst_nonopt			= 0;
	m_nLast_nonopt			= 0;
	m_strNextchar			= 0;
	m_ordering				= REQUIRE_ORDER;
	m_strPosixly_correct	= NULL;
	m_n__getopt_initialized	= 0;
}

int CmdLine::getopt (int argc,char *const *argv,const char *optstring)
{
	return _getopt_internal (argc, argv, optstring,(const struct option *) 0,(int *) 0,0);
}


int CmdLine::getopt_long (int argc,char *const *argv,const char *options,const struct option *long_options,int *opt_index)
{
	return _getopt_internal (argc, argv, options, long_options, opt_index, 0);
}

const char* CmdLine::getarg(void)
{
	return m_strArg;
}

CmdLine::CmdLine(const char *strOpt)
{
	SetOptString(strOpt);
}

CmdLine::~CmdLine()
{}

const char* CmdLine::GetOptString(void)
{
	return m_strOpt;
}

void CmdLine::SetOptString(const char* strOpt)
{
	clear();
	memset(m_strOpt, 0, MAX_OPT_LEN);
	if (strOpt)
	{
		strncpy(m_strOpt, strOpt, MAX_OPT_LEN-1);
	}
}

int CmdLine::ReadOpt(int argc,char *const *argv)
{
	if (NULL == m_strOpt || '\0' == m_strOpt[0])
	{
		return EOF;
	}
	return getopt(argc, argv, m_strOpt);
}

const char* CmdLine::ReadArg(void)
{
	return getarg();
}

}
}
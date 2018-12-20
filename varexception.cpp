
#include "varexception.h"
#include <string.h>

VarException::VarException(const char* strException)
{
	strncpy(m_strError, strException, MAX_EXCEPTION_LEN-1);
}

const char *VarException::what() const
{
	return m_strError;
}

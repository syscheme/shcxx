//varexception.h
//daniel wang

#include <exception>

#ifndef _D_LIB_VAREXCEPTION_H_
#define _D_LIB_VAREXCEPTION_H_

const int MAX_EXCEPTION_LEN = 256;

class VarException : public std::exception
{
private:
	char m_strError[MAX_EXCEPTION_LEN];
public:
	VarException(const char* strException) throw();
	virtual const char *what() const throw();
};

#endif//_D_LIB_VAREXCEPTION_H_

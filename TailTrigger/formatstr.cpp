
#include "formatstr.h"
#include <stdio.h>
#include <vector>
#include <ctype.h>
#include <stdarg.h>


const int MAX_BUFFER_SIZE	= 4096;
const int MAX_PARAMENT		= 256;

const int MAX_INT_STR_LEN	= 10;

std::string FormatString(const char* strFmtStr, ...)
{
	va_list arg;
	va_start(arg, strFmtStr);

	char strTemp[MAX_BUFFER_SIZE] = {0};

	std::vector<const char*> arrParament;

	char strDigit[MAX_INT_STR_LEN+1] = {0};

	int nHadRead = 0;
	
	int nDigitPose, nTemp;

	int nDestPose = 0;

	for (int i = 0; '\0' != strFmtStr[i]; ++i)
	{
		if ('%' == strFmtStr[i])
		{
			if (0 != isdigit(strFmtStr[i+1]))
			{
				++i;
				nDigitPose = i;

				memset(strDigit, 0, MAX_INT_STR_LEN);
				for (int j = 0; j < MAX_INT_STR_LEN; ++j)
				{
					if (0 != isdigit(strFmtStr[i+j]))
					{
						strDigit[j] = strFmtStr[i+j];
					}
					else
					{
						break;
					}
				}

				nTemp = atoi(strDigit);
				while (nTemp > nHadRead)
				{
					arrParament.push_back(va_arg(arg, const char*));
					++nHadRead;
				}

				strcat(strTemp+nDestPose-1, arrParament[nTemp-1]);
				nDestPose += strlen(arrParament[nTemp-1]);
				i += strlen(strDigit)-1;
			}
		}
		else
		{
			strTemp[nDestPose++] = strFmtStr[i];
		}
	}


	va_end(arg);


	return strTemp;
}

std::string FormatString(const char* strPre, const std::vector<std::string>& arrParament)
{
	FormatProc fp;

	std::string strDesStr = PreFormat(strPre, fp);
	std::string strFmtStr;

	char strUTCTime[20] = {0};

	/*
	switch (fp)
	{
	case FP_LOC2UTC:
		LocalTimeToUTCTime(strDesStr.c_str(), strUTCTime, 20);
		strFmtStr = strUTCTime;
	default:
		strFmtStr = strDesStr;
	}*/
	strFmtStr = strDesStr;

	char strTemp[MAX_BUFFER_SIZE] = {0};

	char strDigit[MAX_INT_STR_LEN+1] = {0};

	int nHadRead = 0;
	
	int nDigitPose, nTemp;

	int nDestPose = 0;

	for (int i = 0; '\0' != strFmtStr[i]; ++i)
	{
		if ('%' == strFmtStr[i])
		{
			if (0 != isdigit(strFmtStr[i+1]))
			{
				++i;
				nDigitPose = i;

				memset(strDigit, 0, MAX_INT_STR_LEN);
				for (int j = 0; j < MAX_INT_STR_LEN; ++j)
				{
					if (0 != isdigit(strFmtStr[i+j]))
					{
						strDigit[j] = strFmtStr[i+j];
					}
					else
					{
						break;
					}
				}

				nTemp = atoi(strDigit);

				if (0 == nDestPose)
				{
					strcpy(strTemp, arrParament[nTemp-1].c_str());
				}
				else
				{
					strcat(strTemp+nDestPose-1, arrParament[nTemp-1].c_str());
				}
				nDestPose += strlen(arrParament[nTemp-1].c_str());
				i += strlen(strDigit)-1;
			}
		}
		else
		{
			strTemp[nDestPose++] = strFmtStr[i];
		}
	}

	/*
	FILE* hFile = fopen("c:\\isa.test", "a");
	fprintf(hFile, "\n\n");
	for (int p = 0; p < arrParament.size(); ++p)
	{
		fprintf(hFile, "Patam: %s\n", arrParament[p].c_str());
	}
	fprintf(hFile, "\tFmt: %s\n\tStr: %s\n", strPre, strTemp);
	fclose(hFile);
	*/

	return strTemp;
}

std::string PreFormat(const char* strFmtStr, FormatProc& fp)
{
	char strProc[7] = {0};
	if (0 == strncmp(strFmtStr, LOC2UTC, strlen(LOC2UTC)))
	{
		fp = FP_LOC2UTC;
		return strFmtStr+strlen(LOC2UTC);
	}
	fp = FP_NONE;
	return strFmtStr;
}


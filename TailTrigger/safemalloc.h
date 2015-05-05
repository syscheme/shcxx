
#ifndef _D_LIB_SAFE_MALLOC_H_
#define _D_LIB_SAFE_MALLOC_H_

template <typename _DATA>
_DATA* safe_new(_DATA*& pBuf, size_t zCount)
{
	if (NULL != pBuf)
	{
		delete[] pBuf;
	}
	pBuf = new _DATA[zCount];
	memset(pBuf, 0, zCount);
	return pBuf;
}

template <typename _DATA>
bool safe_delete(_DATA*& pBuf)
{
	if (NULL != pBuf)
	{
		delete[] pBuf;
		return true;
	}
	return false;
}

#endif//_D_LIB_SAFE_MALLOC_H_

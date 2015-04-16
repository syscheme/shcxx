#ifndef _XMAP_H_
#define _XMAP_H_

#include <assert.h>
#include <xdef.h>

#pragma warning(disable:4786)
#include <map>
// #pragma warning(default:4786)

#ifndef _NO_NAMESPACE_
namespace ZQ {
#endif

#ifndef _xassert
#define _xassert(e)		assert((e))
#endif

struct XPlex     // warning variable length structure
{
	XPlex* pNext;
#if (_AFX_PACKING >= 8)
	DWORD dwReserved[1];    // align on 8 byte boundary
#endif
	// BYTE data[maxNum*elementSize];

	void* data() { return this+1; }

	// like 'calloc' but no zero fill
	// may throw memory exceptions
	static XPlex* _STDCALL_ Create(XPlex*& pHead, unsigned int nMax, 
		unsigned int cbElement)
	{
		_xassert(nMax > 0 && cbElement > 0);
		XPlex* p = (XPlex*) new unsigned char[sizeof(XPlex) + nMax * cbElement];
				// may throw exception
		p->pNext = pHead;
		pHead = p;  // change head (adds in reverse order for simplicity)
		return p;
	}

	void FreeDataChain()       // free this one and links
	{
		XPlex* p = this;
		while (p != NULL)
		{
			unsigned char * bytes = (unsigned char *) p;
			XPlex* pNext = p->pNext;
			delete[] bytes;
			p = pNext;
		}
	}
};

template<class TYPE>
inline void DestructElements(TYPE* pElements, int nCount)
{
	_xassert(nCount == 0 || pElements != NULL);

	// call the destructor(s)
	for (; nCount--; pElements++)
		pElements->~TYPE();
}

template<typename TKey, typename TVal>
class XMapT : protected std::map<TKey, TVal> {
protected:
	// Association
	struct XAssoc
	{
		XAssoc* pNext;
		unsigned int nHashTVal;  // needed for efficient iteration
		TKey key;
		TVal value;
	};

public:
	typedef void* POSITION;
// Construction
	XMapT(int nBlockSize = 10);

// Attributes
	// number of elements
	int GetCount() const;
	bool IsEmpty() const;

	// Lookup
	bool Lookup(TKey& TKey, TVal& rTVal) const;

// Operations
	// Lookup and add if not there
	TVal& operator[](TKey& TKey);

	// add a new (TKey, TVal) pair
	void SetAt(TKey& TKey, TVal& newTVal);

	// removing existing (TKey, ?) pair
	bool RemoveKey(TKey& TKey);
	void RemoveAll();

	// iterating all (TKey, TVal) pairs
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, TKey& rTKey, TVal& rTVal) const;

	// advanced features for derived classes
	unsigned int GetHashTableSize() const;
	void InitHashTable(unsigned int hashSize, bool bAllocNow = TRUE);

// Implementation
protected:
	XAssoc** m_pHashTable;
	unsigned int m_nHashTableSize;
	int m_nCount;
	XAssoc* m_pFreeList;
	struct XPlex* m_pBlocks;
	int m_nBlockSize;

	XAssoc* NewAssoc();
	void FreeAssoc(XAssoc*);
	XAssoc* GetAssocAt(TKey&, unsigned int&) const;

public:
	~XMapT();
};

template<typename TKey, typename TVal>
inline int XMapT<TKey, TVal>::GetCount() const
	{ return m_nCount; }
template<typename TKey, typename TVal>
inline bool XMapT<TKey, TVal>::IsEmpty() const
	{ return m_nCount == 0; }
template<typename TKey, typename TVal>
inline void XMapT<TKey, TVal>::SetAt(TKey& key, TVal& newValue)
	{ (*this)[key] = newValue; }
template<typename TKey, typename TVal>
inline XMapT<TKey, TVal>::POSITION XMapT<TKey, TVal>::GetStartPosition() const
	{ return (m_nCount == 0) ? NULL : BEFORE_START_XMapT<TKey, TVal>::POSITION; }
template<typename TKey, typename TVal>
inline unsigned int XMapT<TKey, TVal>::GetHashTableSize() const
	{ return m_nHashTableSize; }

/////////////////////////////////////////////////////////////////////////////
// XMapT<TKey, TVal> out-of-line functions

template<typename TKey, typename TVal>
XMapT<TKey, TVal>::XMapT(int nBlockSize)
{
	_xassert(nBlockSize > 0);

	m_pHashTable = NULL;
	m_nHashTableSize = 17;  // default size
	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
}

template<typename TKey, typename TVal>
void XMapT<TKey, TVal>::InitHashTable(
	unsigned int nHashSize, bool bAllocNow)
//
// Used to force allocation of a hash table or to override the default
//   hash table size of (which is fairly small)
{
	_xassert_VALID(this);
	_xassert(m_nCount == 0);
	_xassert(nHashSize > 0);

	if (m_pHashTable != NULL)
	{
		// free hash table
		delete[] m_pHashTable;
		m_pHashTable = NULL;
	}

	if (bAllocNow)
	{
		m_pHashTable = new XAssoc* [nHashSize];
		memset(m_pHashTable, 0, sizeof(XAssoc*) * nHashSize);
	}
	m_nHashTableSize = nHashSize;
}

template<typename TKey, typename TVal>
void XMapT<TKey, TVal>::RemoveAll()
{
	if (m_pHashTable != NULL)
	{
		// destroy elements (values and keys)
		for (unsigned int nHash = 0; nHash < m_nHashTableSize; nHash++)
		{
			XAssoc* pAssoc;
			for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
			  pAssoc = pAssoc->pNext)
			{
				DestructElements<TVal>(&pAssoc->value, 1);
				DestructElements<TKey>(&pAssoc->key, 1);
			}
		}
	}

	// free hash table
	delete[] m_pHashTable;
	m_pHashTable = NULL;

	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;
}

template<typename TKey, typename TVal>
XMapT<TKey, TVal>::~XMapT()
{
	RemoveAll();
	_xassert(m_nCount == 0);
}

template<typename TKey, typename TVal>
XMapT<TKey, TVal>::XAssoc*
XMapT<TKey, TVal>::NewAssoc()
{
	if (m_pFreeList == NULL)
	{
		// add another block
		XPlex* newBlock = XPlex::Create(m_pBlocks, m_nBlockSize, sizeof(XMapT::XAssoc));
		// chain them into free list
		XMapT::XAssoc* pAssoc = (XMapT::XAssoc*) newBlock->data();
		// free in reverse order to make it easier to debug
		pAssoc += m_nBlockSize - 1;
		for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--)
		{
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc;
		}
	}
	_xassert(m_pFreeList != NULL);  // we must have something

	XMapT::XAssoc* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	_xassert(m_nCount > 0);  // make sure we don't overflow
	ConstructElements<TKey>(&pAssoc->key, 1);
	ConstructElements<TVal>(&pAssoc->value, 1);   // special construct values
	return pAssoc;
}

template<typename TKey, typename TVal>
void XMapT<TKey, TVal>::FreeAssoc(XMapT::XAssoc* pAssoc)
{
	DestructElements<TVal>(&pAssoc->value, 1);
	DestructElements<TKey>(&pAssoc->key, 1);
	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	_xassert(m_nCount >= 0);  // make sure we don't underflow

	// if no more elements, cleanup completely
	if (m_nCount == 0)
		RemoveAll();
}

template<typename TKey, typename TVal>
XMapT<TKey, TVal>::XAssoc* XMapT<TKey, TVal>::GetAssocAt(TKey& key, unsigned int& nHash) const
// find association (or return NULL)
{
	nHash = HashKey<TKey&>(key) % m_nHashTableSize;

	if (m_pHashTable == NULL)
		return NULL;

	// see if it exists
	XAssoc* pAssoc;
	for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (CompareElements(&pAssoc->key, &key))
			return pAssoc;
	}
	return NULL;
}

template<typename TKey, typename TVal>
bool XMapT<TKey, TVal>::Lookup(TKey& key, TVal& rValue) const
{
	_xassert_VALID(this);

	unsigned int nHash;
	XAssoc* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL)
		return FALSE;  // not in map

	rValue = pAssoc->value;
	return TRUE;
}

template<typename TKey, typename TVal>
TVal& XMapT<TKey, TVal>::operator[](TKey& key)
{
	_xassert_VALID(this);

	unsigned int nHash;
	XAssoc* pAssoc;
	if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
	{
		if (m_pHashTable == NULL)
			InitHashTable(m_nHashTableSize);

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc();
		pAssoc->nHashValue = nHash;
		pAssoc->key = key;
		// 'pAssoc->value' is a constructed object, nothing more

		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	}
	return pAssoc->value;  // return new reference
}

template<typename TKey, typename TVal>
bool XMapT<TKey, TVal>::RemoveKey(TKey& key)
// remove key - return TRUE if removed
{
	_xassert_VALID(this);

	if (m_pHashTable == NULL)
		return FALSE;  // nothing in the table

	XAssoc** ppAssocPrev;
	ppAssocPrev = &m_pHashTable[HashKey<TKey&>(key) % m_nHashTableSize];

	XAssoc* pAssoc;
	for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (CompareElements(&pAssoc->key, &key))
		{
			// remove it
			*ppAssocPrev = pAssoc->pNext;  // remove from list
			FreeAssoc(pAssoc);
			return TRUE;
		}
		ppAssocPrev = &pAssoc->pNext;
	}
	return FALSE;  // not found
}

template<typename TKey, typename TVal>
void XMapT<TKey, TVal>::GetNextAssoc(XMapT<TKey, TVal>::POSITION& rNextPosition,
	TKey& rKey, TVal& rValue) const
{
	_xassert_VALID(this);
	_xassert(m_pHashTable != NULL);  // never call on empty map

	XAssoc* pAssocRet = (XAssoc*)rNextPosition;
	_xassert(pAssocRet != NULL);

	if (pAssocRet == (XAssoc*) BEFORE_START_XMapT<TKey, TVal>::POSITION)
	{
		// find the first association
		for (unsigned int nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
			if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
				break;
		_xassert(pAssocRet != NULL);  // must find something
	}

	// find next association
	_xassert(AfxIsValidAddress(pAssocRet, sizeof(XAssoc)));
	XAssoc* pAssocNext;
	if ((pAssocNext = pAssocRet->pNext) == NULL)
	{
		// go to next bucket
		for (unsigned int nBucket = pAssocRet->nHashValue + 1;
		  nBucket < m_nHashTableSize; nBucket++)
			if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
				break;
	}

	rNextPosition = (XMapT<TKey, TVal>::POSITION) pAssocNext;

	// fill in return data
	rKey = pAssocRet->key;
	rValue = pAssocRet->value;
}


#ifndef _NO_NAMESPACE_
} // End of namespace ZQ
#endif

#endif // #ifndef _XMAP_H_

#include "BufferPool.h"
#include "TimeUtil.h"
namespace ZQ {
namespace common {

class FreeListHeap {
public:
    FreeListHeap (size_t hintSize, size_t maxLen);
    ~FreeListHeap ();
    void* malloc();
    bool free(void* p);

    size_t freeSize() const;
    size_t freeSizeMax() const;
private:
    size_t hintSize_;
    size_t maxLen_; // list limitation
    union FreeObject {
        FreeObject* next;
        void* sig;
    } *freeList_;
    size_t listLen_; // trace the list length
};

FreeListHeap::FreeListHeap (size_t hintSize, size_t maxLen)
    :hintSize_(hintSize), maxLen_(maxLen), freeList_(NULL), listLen_(0) {
}

FreeListHeap::~FreeListHeap () {
    FreeObject *obj = freeList_;
    while (obj) {
        void* p = obj;
        obj = obj->next; // iterate
        ::free(p);
    }
}

#define ObjectOfBuffer(p) (FreeObject*)((char*)p - sizeof(FreeObject))
#define BufferOfObject(o) (void*)((char*)o + sizeof(FreeObject))

void* FreeListHeap::malloc() {
    if (freeList_) {
        FreeObject* obj = freeList_;
        freeList_ = freeList_->next; // pop
        --listLen_;
        obj->sig = this;
        return BufferOfObject(obj);
    } else {
        FreeObject* obj = (FreeObject*)::malloc(sizeof(FreeObject) + hintSize_);
        if(obj) {
            obj->sig = this;
            return BufferOfObject(obj);
        } else {
            return NULL;
        }
    }
}
bool FreeListHeap::free(void* p) {
    if (p) {
        FreeObject* obj = ObjectOfBuffer(p);
        if(obj->sig == this) { // validate buffer
            if (listLen_ < maxLen_) {
                obj->next = freeList_; // push
                freeList_ = obj;
                ++listLen_;
            } else {
                ::free(obj);
            }
            return true;
        } else { // invalid
            return false;
        }
    } else {
        return false;
    }
}

size_t FreeListHeap::freeSize() const {
    return listLen_;
}
size_t FreeListHeap::freeSizeMax() const {
    return maxLen_;
}

BufferPool::BufferPool ()
    :flist_(NULL), bufferSize_(0), poolSizeMax_(0), alignTo_(0) {
}
BufferPool::~BufferPool () {
    ZQ::common::MutexGuard guard(lock_);
    if(flist_) {
        delete flist_;
        flist_ = NULL;
    }
}

bool BufferPool::initialize (size_t bufSize, size_t maxBuffers, size_t maxFreeBuffers, size_t alignTo ) {
    if (bufSize == 0 || maxBuffers == 0 || maxFreeBuffers > maxBuffers) {
        return false;
    }
    ZQ::common::MutexGuard guard(lock_);
    if (NULL == flist_) {
        alignTo_ = alignTo;
        bufSize += alignTo_; // reserve for the alignment
        flist_ = new FreeListHeap(bufSize, maxFreeBuffers);
        if (flist_) {
            bufferSize_ = bufSize;
            poolSizeMax_ = maxBuffers;
            return true;
        }
    }

    return false;
}

size_t BufferPool::reserve(size_t n) {
    ZQ::common::MutexGuard guard(lock_);
    if(flist_) {
        if (n > flist_->freeSizeMax()) {
            n = flist_->freeSizeMax();
        }
        if (n > poolSizeMax_ - allocated_.size()) {
            n = poolSizeMax_ - allocated_.size();
        }

        if (n > 0 && n > flist_->freeSize()) {
            std::set<void*> tmp;
            for (size_t i = 0; i < n; ++i) {
                void* p = flist_->malloc(); // may fail here
                if (p) {
                    tmp.insert(p);
                }
            }
            for (std::set<void*>::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                flist_->free(*it);
            }
        }
        return flist_->freeSize();
    } else {
        return 0;
    }
}
bool BufferPool::allocate(Buffer& buf, ErrorCode& err) {
    ZQ::common::MutexGuard guard(lock_);
    if (flist_) {
        if(allocated_.size() >= poolSizeMax_) {
            buf.ptr = NULL;
            buf.len = 0;
            err = allInUse;
            return false;
        }

        char* ptr = (char*)flist_->malloc();
        if(ptr) {
            // calc the alignment
            size_t padding = alignTo_ == 0 ? 0 : (alignTo_ - (ptr - (char*)NULL) % alignTo_);
            if(padding == alignTo_) {
                padding = 0; // just fit the alignment requirement
            }
            buf.ptr = ptr + padding;
            buf.len = bufferSize_ - alignTo_;
            err = noError;
            allocated_[buf.ptr] = ptr;
        } else {
            buf.len = 0;
            err = badAlloc;
        }
    } else {
        buf.ptr = NULL;
        buf.len = 0;
        err = notInited;
    }
    return err == noError;
}

bool BufferPool::release(const Buffer& buf, ErrorCode& err) {
    if(buf.ptr == NULL) {
        err = badPtr;
        return false;
    }
    ZQ::common::MutexGuard guard(lock_);
    if (flist_) {
        std::map<void*, void*>::iterator it = allocated_.find(buf.ptr);
        if (it != allocated_.end()) {
            if (flist_->free(it->second)) {
                allocated_.erase(it);
                err = noError;
            } else {
                err = badPtr;
            }
        } else {
            err = badPtr;
        }
    } else {
        err = notInited;
    }
    return err == noError;
}

/// get the status of the buffer use.
/// @param nActive  - the count of buffers that are in use.
/// @param nFree    - the count of buffers that are free in the pool.
/// @param nMax     - the count of max buffers that the pool can supply.
void BufferPool::getStatus(size_t& nActive, size_t& nFree, size_t& nMax) const {
    ZQ::common::MutexGuard guard(lock_);
    nActive = allocated_.size();
    nFree = flist_ ? flist_->freeSize() : 0;
    nMax = poolSizeMax_;
}

/// list the buffers that in use
/// @return     - the pointers of the buffers that are in use
std::set<void*> BufferPool::listActiveBuffers() const {
    ZQ::common::MutexGuard guard(lock_);
    // copy the pointers
    std::set<void*> bufs;
    for(std::map<void*, void*>::const_iterator it = allocated_.begin(); it != allocated_.end(); ++it) {
        bufs.insert(it->first);
    }
    return bufs;
}

const char* BufferPool::showError(ErrorCode err) {
    static const char* errmsgTbl[] = {
        "No error", // idx:0
        "The pool is not initialized yet",// idx:1
        "Insufficient memory",// idx:2
        "All buffers are in use",// idx:3
        "The buffer is not allocated from this pool",// idx:4
        "Unknown"// idx:5
    };

    switch(err) {
    case noError:
        return errmsgTbl[0];
    case notInited:
        return errmsgTbl[1];
    case badAlloc:
        return errmsgTbl[2];
    case allInUse:
        return errmsgTbl[3];
    case badPtr:
        return errmsgTbl[4];
    default:
        return errmsgTbl[5];
    }
}

// -----------------------------
// class BufferList
// -----------------------------
BufferList::BufferList(bool bReference)
		: _bReference(bReference), _totalSize(0)
	{
	}

BufferList::~BufferList()
	{
		if (_bReference)
			return;

		// else free buffers in _bdlist that have been allocated locally
		for (BDList::iterator itBD = _bdlist.begin(); itBD < _bdlist.end(); itBD++)
		{
			if (NULL == itBD->data)
				continue;
			delete [] itBD->data;
			itBD->data=NULL;
		}

		_bdlist.clear();
	}

size_t BufferList::length() const
	{
		size_t len =0;
		for (BDList::const_iterator itBD = _bdlist.begin(); itBD < _bdlist.end(); itBD++)
		{
			if (NULL == itBD->data)
				continue;
			len += itBD->len;
		}

		return len;
	}

size_t BufferList::join(uint8* buf, size_t size, int payloadLen)
	{
		if (!_bReference || NULL == buf || size <=0)
			return _totalSize;

		if (payloadLen<0)
			payloadLen = size;

		return doJoin(buf, size, payloadLen);
	}

uint mcopy(void* dest, void* src, uint size)
{
	uint n = size / sizeof(uint);
	uint* idest = (uint*)dest, * isrc = (uint*)src;
	while(n--)
		*idest++ = *isrc++;
	n = size % sizeof(uint);
	while(n--)
		*(((uint8*)idest) +n) = *(((uint8*)isrc) +n);
}


size_t BufferList::fill_2(uint8* buf, size_t offset, size_t len, std::vector<int>& useTime)
	{
		// step 1. seek to the right bd per offset
		int64 temp1 = now();
		size_t offsetInBuffer =0, offsetInBD =0;
		BDList::iterator itBD = _bdlist.begin();
		for (; itBD < _bdlist.end(); itBD++)
		{
			if ((offsetInBuffer+itBD->size) < offset)
			{
				offsetInBuffer += itBD->size;
				continue;
			}

			offsetInBD = offset - offsetInBuffer;
			break;
		}
		int64  temp2 = now();
		int a = (int)(temp2 - temp1);
		useTime.push_back(a);
		size_t nLeft = len;	
		int count = 0;
		for (; nLeft >0 && itBD < _bdlist.end(); itBD++, offsetInBD=0)
		{
			size_t nBytes = itBD->size - offsetInBD;
			if (nBytes > nLeft)
				nBytes = nLeft;
			count ++;
			if (nBytes >0)
			{
				mcopy(itBD->data + offsetInBD, buf + len -nLeft, nBytes);
				nLeft -= nBytes;
				//count ++;
				itBD->len = offsetInBD + nBytes;
			}
		}
	  	int64 temp3 = now();
		int b = (int)(temp3 - temp2);
		useTime.push_back(b);

		while (!_bReference && nLeft >0) // reached the BDList end but not yet filled completed
		{
			uint8* pNewBuf = new uint8[BUFFLIST_PAGE_SIZE];
			if (NULL == pNewBuf)
				break;

			size_t nBytes = (nLeft > BUFFLIST_PAGE_SIZE) ? BUFFLIST_PAGE_SIZE : nLeft;
			doJoin(pNewBuf, BUFFLIST_PAGE_SIZE, nBytes);

			mcopy(pNewBuf, buf + len -nLeft, nBytes);
			nLeft -= nBytes;
		}
		int64 temp4 = now();
		int c = (int)(temp4 - temp3);
		useTime.push_back(c);
		useTime.push_back(count);
		return (len -nLeft);
	}

size_t BufferList::fill(uint8* buf, size_t offset, size_t len)
	{
		// step 1. seek to the right bd per offset
		size_t offsetInBuffer =0, offsetInBD =0;
		BDList::iterator itBD = _bdlist.begin();
		for (; itBD < _bdlist.end(); itBD++)
		{
			if ((offsetInBuffer+itBD->size) < offset)
			{
				offsetInBuffer += itBD->size;
				continue;
			}

			offsetInBD = offset - offsetInBuffer;
			break;
		}

		size_t nLeft = len;
		for (; nLeft >0 && itBD < _bdlist.end(); itBD++, offsetInBD=0)
		{
			size_t nBytes = itBD->size - offsetInBD;
			if (nBytes > nLeft)
				nBytes = nLeft;

			if (nBytes >0)
			{
				memcpy(itBD->data + offsetInBD, buf + len -nLeft, nBytes);
				nLeft -= nBytes;
				itBD->len = offsetInBD + nBytes;
			}
		}

		while (!_bReference && nLeft >0) // reached the BDList end but not yet filled completed
		{
			uint8* pNewBuf = new uint8[BUFFLIST_PAGE_SIZE];
			if (NULL == pNewBuf)
				break;

			size_t nBytes = (nLeft > BUFFLIST_PAGE_SIZE) ? BUFFLIST_PAGE_SIZE : nLeft;
			doJoin(pNewBuf, BUFFLIST_PAGE_SIZE, nBytes);

			memcpy(pNewBuf, buf + len -nLeft, nBytes);
			nLeft -= nBytes;
		}

		return (len -nLeft);
	}

size_t BufferList::read(uint8* buf, size_t offset, size_t len)
	{
		// step 1. seek to the right bd per offset
		size_t offsetInBuffer =0, offsetInBD =0;
		BDList::iterator itBD = _bdlist.begin();
		for (; itBD < _bdlist.end(); itBD++)
		{
			if ((offsetInBuffer+itBD->len) < offset)
			{
				offsetInBuffer += itBD->len;
				continue;
			}

			offsetInBD = offset - offsetInBuffer;
			break;
		}

		// step 2. read the data from this BufferList to the output buf space
		size_t nLeft = len;
		for (; nLeft >0 && itBD < _bdlist.end(); itBD++, offsetInBD=0)
		{
			size_t nBytes = itBD->len - offsetInBD;
			if (nBytes > nLeft)
				nBytes = nLeft;

			if (nBytes >0)
			{
				memcpy(buf + len -nLeft, itBD->data + offsetInBD, nBytes);
				nLeft -= nBytes;
			}
		}

		return (len -nLeft);
	}

	// read data out from this buffer into a string 
	// return the bytes has been read from this BufferList
	size_t BufferList::readToString(std::string& str, size_t offset, size_t len)
	{
		// step 1. seek to the right bd per offset
		size_t offsetInBuffer =0, offsetInBD =0;
		BDList::iterator itBD = _bdlist.begin();
		for (; itBD < _bdlist.end(); itBD++)
		{
			if ((offsetInBuffer+itBD->len) < offset)
			{
				offsetInBuffer += itBD->len;
				continue;
			}

			offsetInBD = offset - offsetInBuffer;
			break;
		}

		// step 2. read the data from this BufferList to the output buf space
		size_t nLeft = len;
		for (; (len <0 || nLeft >0) && itBD < _bdlist.end(); itBD++, offsetInBD=0)
		{
			size_t nBytes = itBD->len - offsetInBD;
			if (nBytes > nLeft)
				nBytes = nLeft;

			if (nBytes >0)
			{
				str.append((char*)(itBD->data + offsetInBD), nBytes);
				nLeft -= nBytes;
			}
		}

		return (len -nLeft);
	}

size_t BufferList::doJoin(uint8* buf, size_t size, size_t len)
	{
		BD bd;
		bd.data = buf;
		bd.size = size;
		bd.len =len;
		_bdlist.push_back(bd);
		_totalSize += size;
		return _totalSize;
	}


}} // namespace ZQ::common

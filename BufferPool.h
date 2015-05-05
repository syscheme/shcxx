#ifndef __ZQ_Common_BufferPool_H__
#define __ZQ_Common_BufferPool_H__
#include "ZQ_common_conf.h"
#include "Locks.h"
#include "Pointer.h"
#include <set>
#include <map>
#include <vector>

namespace ZQ {
namespace common {

class ZQ_COMMON_API BufferPool;
class ZQ_COMMON_API BufferList;

struct Buffer {
    void* ptr; // the memory pointer
    size_t len; // the length of the buffer
    Buffer():ptr(NULL),len(0){}
};

class FreeListHeap; // helper class

///
/// the BufferPool class
///
class BufferPool {
public:
    enum ErrorCode {
        noError = 0,
        notInited, // the pool is not initialized yet
        badAlloc, // memory allocation fault
        allInUse, // all buffers is in use
        badPtr // the buffer is not allocated from this pool
    };
    BufferPool ();
    ~BufferPool ();

    /// initialize the pool
    /// @param bufSize          - the buffer's memory size in bytes.
    /// @param maxBuffers       - the limitation of the pool's capacity.
    /// @param maxFreeBuffers   - the max free buffers count when the pool is idle.
    /// @param alignTo          - the address alignment of the buffer. 0 for no alignment.
    bool initialize (size_t bufSize, size_t maxBuffers, size_t maxFreeBuffers, size_t alignTo = 0);

    /// reserve buffers for the future allocation.
    /// @param n    - the count of the buffers that need to be reserved.
    /// @return     - the free buffers count after the reservation.
    size_t reserve(size_t n);

    /// allocate memory buffer from the pool.
    /// @param buf  - the buffer that will be allocated from the pool.
    /// @param err  - the error code in the call.
    ///               possible value: noError, notInited, badAlloc, allInUse.
    /// @return     - true for success. false for failure and
    ///               the err parameter will indicate the error.
    bool allocate(Buffer& buf, ErrorCode& err);

    /// release the buffer.
    /// @param buf  - the buffer that was allocated from the pool.
    /// @param err  - the error code in the call.
    ///               possible value: noError, notInited, badPtr.
    /// @return     - true for success. false for failure and
    ///               the err parameter will indicate the error.
    bool release(const Buffer& buf, ErrorCode& err);

    /// get the status of the buffer use.
    /// @param nActive  - the count of buffers that are in use.
    /// @param nFree    - the count of buffers that are free in the pool.
    /// @param nMax     - the count of max buffers that the pool can supply.
    void getStatus(size_t& nActive, size_t& nFree, size_t& nMax) const;

    /// list the buffers that in use
    /// @return     - the pointers of the buffers that are in use
    std::set<void*> listActiveBuffers() const;

    /// show the error message
    /// @param err  - the error code
    /// @return     - the string description of the error code
    static const char* showError(ErrorCode err);
private:
    ZQ::common::Mutex lock_;
    FreeListHeap* flist_;
    std::map<void*, void*> allocated_;
    size_t bufferSize_;
    size_t poolSizeMax_;
    size_t alignTo_;
};

// -----------------------------
// class BufferList
// -----------------------------
#define BUFFLIST_PAGE_SIZE (4*1024) // 4KB

class BufferList : public SharedObject
{
public:
	typedef Pointer <BufferList> Ptr;

	BufferList(bool bReference = false);
	virtual ~BufferList();

	size_t size() const	{ return _totalSize; }
	size_t length() const;

	// join a new piece of buffer in, only allowed when this buffer is reference-only
	//@return the total size of buffer after join
	virtual size_t join(uint8* buf, size_t size);

	// fill data into this buffer. 
	//  - if this buffer is not refrerence only, it may grow if space is not enough to complete the filling
	//  - otherwise, the filling stops when the space in buffer is run out
	// return the bytes has been filled into this BufferList
	virtual size_t fill(uint8* buf, size_t offset, size_t len);

	// read data out from this buffer. 
	// return the bytes has been read from this BufferList
	virtual size_t read(uint8* buf, size_t offset, size_t maxlen);

	// read data out from this buffer into a string 
	// return the bytes has been read from this BufferList
	virtual size_t readToString(std::string& str, size_t offset=0, size_t len =-1);


protected:

	typedef struct _BD
	{
		uint8* data;
		size_t len, size;
	} BD;

	typedef std::vector < BD > BDList;

	BDList _bdlist;
	bool   _bReference;
	size_t _totalSize;

	// join a new piece of buffer in
	//@return the total size of buffer after join
	virtual size_t doJoin(uint8* buf, size_t size, size_t len);
};

}} // namespace ZQ::common
#endif

#ifndef __libasync_diskfile_header_file_h__
#define __libasync_diskfile_header_file_h__

#include "eventloop.h"

namespace LibAsync {

	class DiskFile: virtual public ZQ::common::SharedObject {
	protected:
		DiskFile(EventLoop& loop);
	private:
		DiskFile(const DiskFile& );
		DiskFile& operator=( const DiskFile&);
	public:
		virtual ~DiskFile(void);

		typedef ZQ::common::Pointer<DiskFile>	Ptr;

		EventLoop&		getLoop() const { return mLoop; }

		static DiskFile::Ptr	create( EventLoop& loop );

		bool			open( const std::string& filepath, const std::string& mode );

		void			close( );

	protected:
		bool			read( AsyncBuffer& buf );
		bool			read( AsyncBufferS& bufs);

		bool			write( AsyncBuffer& buf );
		bool			write( AsyncBufferS& bufs );
	protected:

		virtual		void	onFileOpened( ) { }

		virtual		void	onFileRead( size_t size ){ }

		virtual		void	onFileWritten( size_t size) { }

		virtual		void	onFileError( int error){ close(); }
		
	private:
		EventLoop&		mLoop;
		
	};

}//namespace LibAsync

#endif//__libasync_diskfile_header_file_h__
//vim: ts=4:sw=4:autoindent:fileencodings=gb2312

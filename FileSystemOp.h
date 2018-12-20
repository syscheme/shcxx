#ifndef __FILESYSTEM__
#define __FILESYSTEM__


#include "ZQ_common_conf.h"
#include <string>
#include <vector>


#if defined(ZQ_OS_MSWIN)

#include <process.h>
#include <direct.h>
#include <shlwapi.h>

#elif defined(ZQ_OS_LINUX)

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>
#include <glob.h>
#include <inttypes.h>
#include <fnmatch.h>

#endif


namespace FS {

#if defined(ZQ_OS_MSWIN)

	class FileAttributes {

	public:

		FileAttributes(const std::string& path):_path(path), _isValid(true) {
			if(!GetFileAttributesEx(path.c_str(), ::GetFileExInfoStandard, &_attrs)) {
				_isValid = false;
			}
		}

	public:
        
        std::string name() const {
            if(isRegular()) {
                return _path.substr(_path.find_last_of('\\')+1);
            }
            return _path;
        }

		bool isDirectory() const {
			return (_isValid && (_attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
		}

		bool isRegular() const {
			return (!isDirectory());
		}

		bool exists() const {
			return _isValid;
		}

		int64 size() const {
			return _isValid ? ((_attrs.nFileSizeHigh << sizeof(DWORD))+_attrs.nFileSizeLow) : 0;
		}

        time_t mtime() const {
            FILETIME ft = _attrs.ftLastWriteTime;
            
            LONGLONG ll = ft.dwHighDateTime;
            ll <<= 32;
            ll |= ft.dwLowDateTime;
            ll  -= 116444736000000000;
            return (time_t)(ll / 10000000);
        }

	private:
        std::string _path;

		WIN32_FILE_ATTRIBUTE_DATA _attrs;
		bool _isValid;

	};

#elif defined(ZQ_OS_LINUX)

	class FileAttributes {

	public:

		FileAttributes(const std::string& path):_path(path),_isValid(true) {
			if(lstat64(_path.c_str(), &_attrs) == (-1)) {
				_isValid = false;
			}
		}

	public:

        std::string name() const {
            if(isRegular()) {
                char *path, *base;
                path = strdup(_path.c_str());
                base = basename(path);
                
                std::string n(base);
                free(path);

                return n;
            }
            return _path;
        }

		bool isDirectory() const {
			return (_isValid && (S_ISDIR(_attrs.st_mode)));
		}

		bool isRegular() const {
			return (_isValid && (S_ISREG(_attrs.st_mode)));
		}

		bool exists() const {
			return _isValid;
		}

		int64 size() const {
			return _isValid ? _attrs.st_size : 0;
		}

        time_t mtime() const {
            return _isValid ? _attrs.st_mtime : 0;
        }

	private:
        std::string _path;

		struct stat64 _attrs;
		bool _isValid;

	};

#endif

	extern bool createDirectory(const std::string& path, bool recursive=false);

	extern std::string getWorkingDirectory();

	extern std::string getImagePath();

	extern bool remove(const std::string& path, bool recursive=false);

	extern bool getDiskSpace(const std::string& path, uint64& avail, uint64& total, uint64& free); 

	extern std::vector<std::string> searchFiles(const std::string& path, const std::string& pattern); 

	extern std::vector<std::string> matchFiles(const std::vector<std::string>& names, const std::string& pattern);
}



#endif


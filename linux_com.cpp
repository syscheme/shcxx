#include "ZQ_common_conf.h"
#include "XMLPreferenceEx.h"
#include "strHelper.h"

#include <fcntl.h>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace ZQ::common;

#define LONG_LEN 64

XMLPreferenceEx* getPreferenceNode(
                    const std::string& path, 
                    XMLPreferenceDocumentEx& config) {

    if(path.empty() || !config.isValid()) {
        return 0;
    }

    std::vector<std::string> nodes;
    stringHelper::SplitString(path, nodes, "/", "/");

    if(!nodes.size()) {
        return 0;
    }

    XMLPreferenceEx* tmp1 = config.getRootPreference();
    if(!tmp1) {
        return 0;
    }

    XMLPreferenceEx* tmp2 = 0;

    std::vector<std::string>::iterator iter = nodes.begin();
    for(; iter != nodes.end(); ++iter) {
        tmp2 = tmp1->firstChild((*iter).c_str());
			
        if(!tmp2) {
            break;
        }
    
        tmp1->free();
        
        tmp1 = tmp2;
    }

    return tmp2;
}

typedef std::vector<std::string> SVCSET;

void listService(SVCSET* set = 0) {
    if(set) {
        set->clear();
    }

    const std::string basedir = "/proc/";

    DIR* dir = opendir(basedir.c_str());
    if(!dir) {
        fprintf(stderr, "failed to open /proc, no permission or broken system?\n");
        return;
    }

   /* 
    *  we traverse all the running processes 
    *  and try to find a command start with 
    *  'SystemShell' and that's it.
    */
    std::ostringstream myself;
    myself << getpid();

    struct dirent* entry;
    while((entry = readdir(dir))!=0) {
        std::string tmp = basedir + entry->d_name;

        struct stat st;
        if(!stat(tmp.c_str(), &st) && 
            S_ISDIR(st.st_mode)    && 
            ((*entry->d_name)>'0'  && (*entry->d_name)<='9') &&
            strcmp(entry->d_name, myself.str().c_str())) {

            tmp += "/cmdline";
        }    
        else {
            continue;
        }

        FILE* f = fopen(tmp.c_str(), "r");
        if(!f) {
            continue;
        }

        char buff[255];
        memset(buff, '\0', sizeof(buff));
        if(!fgets(buff, sizeof(buff), f)) {
            fclose(f);
            continue;
        }

        char* name  = basename(buff);
        if(!strcmp(name, "SystemShell")) {
            long pos = ftell(f);
            /* take care of the overflow */
            if(pos < (long)sizeof(buff)) {
                if((name = strrchr(buff, '\0'))) {
                    ++name;
                }
            }
            if(set) {
                set->push_back(name);
            }
            else {
                printf("%s\n", name); 
            }
        }
        fclose(f);
    }
    closedir(dir);
}

bool isSvcRunning(const std::string& f) {
	int fd = open(f.c_str(), O_RDONLY);
	if(fd < 0) {
		return false;
	}
	char buf[255] = {0};
	ssize_t bytes = read(fd, buf, sizeof(buf));
	if(bytes == (-1)) {
		close(fd);
		return false;
	}
	std::string path = std::string("/proc/")+buf;
	struct stat st;
	int ret = stat(path.c_str(), &st);
	if(ret < 0) {
		close(fd);
		return false;
	}
	close(fd);
	return true;
}

bool checkLock(const std::string& file) {
	
op:
	int fd = open(file.c_str(), O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IXUSR);
	if(fd < 0) {
		if(errno == EEXIST) {
			if(isSvcRunning(file)) {
				return false;
			}
			else {
				remove(file.c_str());
				goto op;
			}
		}
		else {
			return false;
		}
	}		

	FILE* lock = fdopen(fd, "r+");
	if(!lock) {
		close(fd);
		return false;
	}

	if(lockf(fileno(lock), F_TLOCK, 0) == (-1)) {
		fclose(lock);
		return false;
	}
	fcntl(fileno(lock), F_SETFD, 1);

	rewind(lock);
	fprintf(lock, "%d", getpid());
	fflush(lock);

	ftruncate(fileno(lock), ftell(lock));
	fclose(lock);
	
	return true;
}

char* itoa(int value, char*  str, int radix)
{
    int  rem = 0;
    int  pos = 0;
    char ch  = '!' ;
    do
    {
        rem    = value % radix ;
        value /= radix;
        if ( 16 == radix )
        {
            if( rem >= 10 && rem <= 15 )
            {
                switch( rem )
                {
                    case 10:
                        ch = 'a' ;
                        break;
                    case 11:
                        ch ='b' ;
                        break;
                    case 12:
                        ch = 'c' ;
                        break;
                    case 13:
                        ch ='d' ;
                        break;
                    case 14:
                        ch = 'e' ;
                        break;
                    case 15:
                        ch ='f' ;
                        break;
                }
            }
        }
        if( '!' == ch )
        {
            str[pos++] = (char) ( rem + 0x30 );
        }
        else
        {
            str[pos++] = ch ;
        }
    }while( value != 0 );
    str[pos] = '\0' ;
    return strrev(str);
}

char* strrev(char* szT)
{
    if ( !szT )                
        return NULL;
    int i = strlen(szT);
    int t = !(i%2)? 1 : 0;   
    for(int j = i-1 , k = 0 ; j > (i/2 -t) ; j-- )
    {
        char ch  = szT[j];
        szT[j]   = szT[k];
        szT[k++] = ch;
    }
    return szT;
}

long watol(wchar_t* wch)
{
	if(wch == NULL)
		return 0;

	char chdes[LONG_LEN] = {0};
	int res = wctomb(chdes,*wch);

	long lval = 0;
	if(res > 0)
		lval = atol(chdes);

	return lval;
}

// vim: ts=4 sw=4 bg=dark

#include "HttpCRGConfig.h"
#include <ZQ_common_conf.h>
#ifdef ZQ_OS_MSWIN
	#include <io.h>
#else
	#include <glob.h>
#endif

namespace ZQTianShan{
namespace HttpCRG{

#ifdef ZQ_OS_MSWIN
static void findWithWildcard(const std::string &filespec, ::std::vector<std::string> &result)
{
    result.clear();

    _finddata_t fileInfo;
    long hFind = ::_findfirst(filespec.c_str(), &fileInfo);
    if(-1 == hFind)
        return;
    // get the folder
	std::string folder = ::ZQ::common::Config::parseFilePath(filespec).first;
    do
    {
        // save file path
        result.push_back(folder + fileInfo.name);
    }while(0 == ::_findnext(hFind, &fileInfo));
    ::_findclose(hFind);
    return;
}

#else
static void findWithWildcard(const std::string &filespec, ::std::vector<std::string> &result)
{
    result.clear();

 	glob_t gl;
    if(!glob(filespec.c_str(), GLOB_PERIOD|GLOB_TILDE, 0, &gl))
    {
        for(size_t i = 0; i < gl.gl_pathc; ++i)
        {
			result.push_back(gl.gl_pathv[i]);
		}	
	}
}


#endif

// PluginsConfig
void PluginsConfig::populate(const std::string &filespec)
{
    if(filespec.empty())
        return;
    std::vector<std::string> files;
    findWithWildcard(filespec, files);
    std::copy(files.begin(), files.end(), std::inserter(modules, modules.end()));
}

} // namespace HttpCRG
} // namespace ZQTianShan 


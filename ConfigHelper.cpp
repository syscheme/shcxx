#include <ZQ_common_conf.h>
#include "ConfigHelper.h"
namespace ZQ{
namespace common{

//////////////////////////////////////////////////////////////////////////
// xml utility
namespace XMLUtil{
static void _xmlnode_deleter(ZQ::common::XMLPreferenceEx* p)
{
    if(p) p->free();
}

XmlNode toShared(ZQ::common::XMLPreferenceEx* p)
{
    return XmlNode(p, _xmlnode_deleter);
}

XmlNodes locate(XmlNode root, const std::string &path)
{
    if(!root)
<<<<<<< HEAD
        throwf<NavigationException>(EXFMT(NavigationException, "XMLUtil::locate() bad node object. path [%s]"), path.c_str());

=======
    {
        throwf<NavigationException>(EXFMT(NavigationException, "XMLUtil::locate() bad node object. path [%s]"), path.c_str());
    }
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
    // step 1: parse the path
    typedef std::vector< std::string > StandardPath;
    StandardPath stdpath;
    ZQ::common::stringHelper::SplitString(path, stdpath, "/", "/");
    
    std::string targetTag = "";
    if(!stdpath.empty())
    {
        targetTag = stdpath.back();
        stdpath.pop_back();
    }
    
    // step 2: navigate through the path
    XmlNode current = root; 
    for(StandardPath::iterator it_tag = stdpath.begin(); it_tag != stdpath.end(); ++it_tag)
    {
        current = toShared(current->firstChild(it_tag->c_str()));
        if(!current)
<<<<<<< HEAD
            throwf<NavigationException>(EXFMT(NavigationException, "XMLUtil::locate() failed to find element[%s] in path[%s]"), it_tag->c_str(), path.c_str());
=======
        {
            throwf<NavigationException>(EXFMT(NavigationException, "XMLUtil::locate() can't find node [%s] in path [%s]"), it_tag->c_str(), path.c_str());
        }
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
    }
    
    XmlNodes result;
    if(!targetTag.empty())
    {
        XmlNode targetNode = toShared(current->firstChild(targetTag.c_str()));
        while(targetNode)
        {
            result.push_back(targetNode);
            targetNode = toShared(current->nextChild());
        }
    }
    else
    {
        // the case of empty path
        result.push_back(current);
    }
<<<<<<< HEAD

=======
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
    return result;
}

std::string fullPath(XmlNode node)
{
    std::vector<char> buf;
    buf.resize(512);
    node->getPreferenceName(&buf[0], false, (int)buf.size() - 1);
    return &buf[0];
}
<<<<<<< HEAD

} // namespace XMLUtil

=======
} // namespace XMLUtil
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
//////////////////////////////////////////////////////////////////////////
#define PPLOG if(_pLog) (*_pLog)
// preprocessor that implement the macro replacement function
bool Preprocessor::define(const std::string &macroName, const std::string &macroDef)
{
    // check macro name
    if(macroName.empty() || macroName.find("${") != std::string::npos)
    {
<<<<<<< HEAD
        PPLOG(Log::L_WARNING, "bad macro name[%s]", macroName.c_str());
        return false;
    }
    
=======
        PPLOG(Log::L_WARNING, "Bad macro name, [%s]", macroName.c_str());
        return false;
    }
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
    std::string macro = std::string("${") + macroName + "}";
//    VariableMap::iterator it = _variables.find(macro);
//     if(it == _variables.end())
//     {
//         PPLOG(Log::L_DEBUG, "Added macro [%s]", macroName.c_str());
//     }
//     else
//     {
//         PPLOG(Log::L_DEBUG, "Updated macro [%s]", macroName.c_str());
//     }
    std::string macroValue = macroDef;
    if(fixup(macroValue))
    {
        _variables[macro] = macroValue;
<<<<<<< HEAD
        PPLOG(Log::L_DEBUG, CLOGFMT(Preprocessor, "associated %s[%s]"), macroName.c_str(), macroValue.c_str());
        return true;
    }

	PPLOG(Log::L_WARNING, CLOGFMT(Preprocessor, "failed to associate %s[%s]"), macroName.c_str(), macroDef.c_str());
=======
        PPLOG(Log::L_DEBUG, CLOGFMT(Preprocessor, "associated macro[%s]=value[%s]"), macroName.c_str(), macroValue.c_str());
        return true;
    }

	PPLOG(Log::L_ERROR, CLOGFMT(Preprocessor, "failed to associate macro[%s]=exp[%s]"), macroName.c_str(), macroDef.c_str());
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
	return false;
}

bool Preprocessor::define(const Macros& macros)
{
    for(Macros::const_iterator it = macros.begin(); it != macros.end(); ++it)
    {
        if(!define(it->first, it->second))
        {
<<<<<<< HEAD
            PPLOG(Log::L_ERROR, CLOGFMT(Preprocessor, "failed to associate %s[%s]"), it->first.c_str(), it->second.c_str());
=======
            PPLOG(Log::L_ERROR, CLOGFMT(Preprocessor, "failed to associate macro[%s] with exp[%s]"), it->first.c_str(), it->second.c_str());
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
            return false;
        }
    }

    return true;
}

bool Preprocessor::fixup(std::string &str) const
{
	size_t nResolved =0, nResolvedOfRound =0;

	do {
		nResolved += nResolvedOfRound;
		nResolvedOfRound =0;
		if((nResolved) > 256)
		{
			PPLOG(ZQ::common::Log::L_ERROR, CLOGFMT(Preprocessor, "macro[%s] nested too much"), str.c_str());
			throwf<PreprocessException>(EXFMT(PreprocessException, "macro[%s] nested too much"), str.c_str());
			return false;
		}

		std::string::size_type pos_macro_begin =0, pos_macro_end =0;
		for (pos_macro_begin =0; std::string::npos !=(pos_macro_begin = str.find("${", pos_macro_begin)); pos_macro_begin = pos_macro_end + 1)
		{
			// get macro string
			if(std::string::npos == (pos_macro_end = str.find_first_of('}', pos_macro_begin))) // not a valid macro reference
			{
				PPLOG(ZQ::common::Log::L_WARNING, CLOGFMT(Preprocessor, "unpaired brackets in macro[%s]"), str.c_str());
				break;
			}

			std::string macro = str.substr(pos_macro_begin, pos_macro_end + 1 - pos_macro_begin); // include the end '}'

			// try to fixup the macro
			std::map< std::string, std::string >::const_iterator cit_macro = _variables.find(macro);
			if(_variables.end() == cit_macro)
			{
				PPLOG(ZQ::common::Log::L_WARNING, CLOGFMT(Preprocessor, "unassociated macro[%s] referenced"), macro.c_str());
				continue;
			}

			str.replace(pos_macro_begin, macro.size(), cit_macro->second);
			nResolvedOfRound++;
		}

	} while(nResolvedOfRound >0);

	return true;
}

<<<<<<< HEAD
namespace Config {
=======
namespace Config{
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
//////////////////////////////////////////////////////////////////////////

// getter & setter of the global logger in the config module
static ZQ::common::Log* gConfLog = NULL;
<<<<<<< HEAD
Log* getConfLog() { return gConfLog; }
void setConfLog(ZQ::common::Log* logger) { gConfLog = logger; }
=======
Log* getConfLog() {
    return gConfLog;
}
void setConfLog(ZQ::common::Log* logger) {
    gConfLog = logger;
}
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

// set/check the flag that if the program set the global logger pointer
// in the Loader::setLogger(). Pass NULL for check-only.
// default flag: true
static bool gSetConfLogInLoader = true;
bool setConfLogInLoader(bool* pEnabled) {
    if(pEnabled) {
        gSetConfLogInLoader = *pEnabled;
    }
<<<<<<< HEAD

=======
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
    return gSetConfLogInLoader;
}

void MacroDefinition::structure(Holder<MacroDefinition>& holder)
{
    holder.addDetail("Definitions", "src", &MacroDefinition::src, "");
    holder.addDetail("Definitions", &MacroDefinition::readMacroReference, &MacroDefinition::registerNothing);
    holder.addDetail("Definitions/property", &MacroDefinition::readMacro, &MacroDefinition::registerNothing);
}

static bool isFullPath(const std::string &path)
{
    // assume path is a valid file path
#ifdef ZQ_OS_MSWIN
    return (std::string::npos != path.find(':'));
#endif
#ifdef ZQ_OS_LINUX
    return (path[0] == '/');
#endif
}

// auxiliary function of parsing a file path
std::pair<std::string, std::string> parseFilePath(const std::string &path)
{
    std::string::size_type pos = path.rfind(FNSEPC); // pos of '/'
<<<<<<< HEAD
    if(std::string::npos == pos)
        return std::make_pair<std::string>("", path);

    std::string::size_type name_start_pos = pos + 1; // one after the '/'
    std::string filename = path.substr(name_start_pos);
    std::string folder = path.substr(0, path.size() - filename.size());
    return std::make_pair(folder, filename);
=======
    if(std::string::npos != pos)
    {
        std::string::size_type name_start_pos = pos + 1; // one after the '/'
        std::string filename = path.substr(name_start_pos);
        std::string folder = path.substr(0, path.size() - filename.size());
        return std::make_pair(folder, filename);
    }
    else // a file name
    {
        return std::make_pair<std::string>("", path);
    }
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
}

void MacroDefinition::readMacroReference(XMLUtil::XmlNode node, const Preprocessor* hPP)
{
    // use as post action of got reference file
<<<<<<< HEAD
    if(src.empty())
        return;

    if(!isFullPath(src))
    {
        // treat as relative path
        src = folder + src;
    }

    Config::Loader<MacroDefinition> loader("");
    loader.setLogger(pLog); // enable log while loading
    loader.pLog = pLog; // pass the log object

    // parse the path
    loader.folder = parseFilePath(src).first;

    if(loader.load(src.c_str(), false))// disable the macro here
        macros.swap(loader.macros);
    else
        throwf<CfgException>(EXFMT(CfgException, "failed to load macro definition file [%s]"), src.c_str());
=======
    if(!src.empty())
    {
        if(!isFullPath(src))
        {
            // treat as relative path
            src = folder + src;
        }

        Config::Loader<MacroDefinition> loader("");
        loader.setLogger(pLog); // enable log while loading
        loader.pLog = pLog; // pass the log object
        // parse the path
        loader.folder = parseFilePath(src).first;

        if(loader.load(src.c_str(), false))// disable the macro here
        {
            macros.swap(loader.macros);
        }
        else
        {
            throwf<CfgException>(EXFMT(CfgException, "Failed to load macro definition file [%s]"), src.c_str());
        }
    }
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534
}

void MacroDefinition::readMacro(XMLUtil::XmlNode node, const Preprocessor* hPP)
{
    Config::Holder< NVPair > nvholder("");
    nvholder.read(node);
    macros.push_back(std::make_pair(nvholder.name, nvholder.value));
}

<<<<<<< HEAD
}}} // namespace ZQTianShan::common::Config

=======
} // namespace Config
} // namespace common
} // namespace ZQTianShan
>>>>>>> b6d312f638ee3d740af4a0af01bcfa621a177534

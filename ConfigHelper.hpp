#ifndef __ZQ_COMMON_CONFIG_HELPER_HPP_IMPLEMENT__
#define __ZQ_COMMON_CONFIG_HELPER_HPP_IMPLEMENT__

#include "ConfigHelper.h"

#ifndef CONFIG_NO_SNMP
#include "../common/snmp/SubAgent.hpp"
#endif// CONFIG_NO_SNMP

namespace ZQ{
namespace common{
namespace Config{

template < class FragmentT >
struct Holder<FragmentT>::__Other
{
	__Other(const std::string &thePath, ReadOther theReadFunc, RegisterOther theRegisterFunc, const Range &theNodeCount, const std::string &theSnmpName)
		:path(thePath), readFunc(theReadFunc), registerFunc(theRegisterFunc), nodeCount(theNodeCount), snmpName(theSnmpName)
	{ }

	std::string   path;
	ReadOther     readFunc;
	RegisterOther registerFunc;
	Range         nodeCount;
	std::string   snmpName;
};

template < class FragmentT >
struct Holder<FragmentT>::__Detail
{
private:
	class IDetailElement;

	template<class T>    struct  DeduceType;
	template<class Type> class   __DetailElement;

	typedef boost::shared_ptr<IDetailElement>  IDetailElementPtr;

public:
	__Detail(const std::string& rootPath, const std::string &nodepath, const std::string &attrname, SnmpOption snmpOpt, const std::string &snmpName = "")
		:root(rootPath), path(nodepath), name(attrname), _snmpName(snmpName), _optional(false), _snmpOpt(snmpOpt)
	{ 
		_snmpOpt = optReadOnly;
	}

	std::string get(FragmentT &obj)
	{
		return _performer->get(obj); 
	}
	void  set(FragmentT& obj, const char* value)
	{
		if(NULL == value)
		{
			if(_optional)
				return; // leave with default value silently

			throwf<CfgException>(EXFMT(CfgException, "Detail::set() attribute missed, path[%s], name[%s]. root=%s"), path.c_str(), name.c_str(), root.c_str());
		}

		_performer->set(obj, value, *this);
	}
	void  setDefault(FragmentT& obj, const std::string& defaultValue)
	{
		_optional = true; 
		_performer->set(obj, defaultValue.c_str(), *this);
	}
	template<class Type> void setAddress(Type address, int length)
	{
		_performer = IDetailElementPtr(new __DetailElement<Type>(address, length));
	}
	void  snmpRegister(FragmentT& obj, const std::string &full_path);

public:
	std::string root;
	std::string path;
	std::string name;

private:
	IDetailElementPtr _performer;
	bool        _optional; // if this attribute is optional
	SnmpOption  _snmpOpt;  // for snmp variable
	std::string _snmpName;
};


template < class FragmentT >
void Holder<FragmentT>::__Detail::snmpRegister(FragmentT& obj, const std::string &full_path)
{
#ifndef CONFIG_NO_SNMP // disable SNMP if need
	if (optReadOnly != _snmpOpt)
		throwf<CfgException>(EXFMT(CfgException, "Detail::snmpRegister() bad snmp option, option[%d]"), _snmpOpt);

	using namespace ZQ::Snmp;
	ZQ::Snmp::Subagent* pSubagent = ZQ::Snmp::getSnmpFromShareMem();
	if (!pSubagent)
		throwf<CfgException>(EXFMT(CfgException, "Detail::snmpRegister() bad snmp pSubagent[empty]") );

	ZQ::Snmp::ModulePtr mod = pSubagent->module(ZQ::Snmp::Oid("2") );
	if (!mod)
		throwf<CfgException>(EXFMT(CfgException, "Detail::snmpRegister() bad snmp module[empty]") );

	// step 1: generate the snmp oid
	Oid valueId("1.1");
	Oid nameId ("2.1");
	Oid rwId("3.1");
	unsigned int instanceId = 1;	
	for (SmiValue value; noSuchName != mod->get(valueId, value) && ++instanceId; )
		valueId.replace(instanceId);

	rwId.replace(instanceId);
	nameId.replace(instanceId);

	//step 2: generate the variable's full name
	std::string varName = full_path;
	if(!_snmpName.empty())
		varName += this->_snmpName;
	else
	{
		if(!path.empty())
		{
			varName += this->path;
			varName += "/";
		}
		varName += this->name;
	}

	// step 3: register the variable
	bool valueRev = mod->addObject( valueId, _performer->value(obj) );    //value column
	bool nameRev  = mod->addObject( nameId,  _performer->name(varName) ); //name column
	bool rwRev    = mod->addObject( rwId,    _performer->readWrite() ); //rw column
	if (!nameRev || !valueRev)
		throwf<CfgException>(EXFMT(CfgException, "Detail::snmpRegister() bad snmp instanceId[%d], full name[%s], rev[%d_%d]"),
		instanceId, varName.c_str(), valueRev, nameRev);

#endif//CONFIG_NO_SNMP
	return;
}

#ifndef CONFIG_NO_SNMP
template < class FragmentT >
template<class DeduceT> 
struct Holder<FragmentT>::__Detail::DeduceType
{
	typedef  DECLARE_SNMP_RO_TYPE(int, int, int)                          roSnmpInt;
	typedef  DECLARE_SNMP_RO_TYPE(std::string, std::string, std::string)  roSnmpString;

	// add "class TDefault = char" for g++ support explicit specialization at class scope
	// not support others except specialization, as compile time guard
	template<class DT, class TDefault = char> struct DeduceDef
	{
		typedef DT NotSupported[0];
		enum{ Type = sizeof(NotSupported) }; 
		typedef  NotSupported regSnmpType;     typedef NotSupported SelfType;         typedef NotSupported varType;
	};//#error "Config Holder type not support found"
	
	template<class TDefault> struct DeduceDef<PMem_Int32, TDefault>    
	{ 
		enum{Type = ZQ::Snmp::AsnType_Integer}; 
		typedef roSnmpInt  regSnmpType;  typedef PMem_Int32 SelfType;     typedef int varType;
	};

	template<class TDefault> struct DeduceDef<PMem_StdString, TDefault>
	{ 
		enum{Type = ZQ::Snmp::AsnType_Octets}; 
		typedef roSnmpString regSnmpType; typedef PMem_StdString SelfType; typedef std::string varType;
	};

	template<class TDefault> struct DeduceDef<PMem_CharArray, TDefault>
	{ 
		enum{Type = ZQ::Snmp::AsnType_Octets};  
		typedef roSnmpString regSnmpType; typedef PMem_CharArray SelfType; typedef std::string varType;
	};

	enum{ Result = DeduceDef<DeduceT>::Type};

	typedef typename DeduceDef<DeduceT>::regSnmpType regResultType;
	typedef typename DeduceDef<DeduceT>::varType     regVarType;
	typedef typename DeduceDef<DeduceT>::SelfType    mySelfType;
};

struct SnmpConvert
{
	void operator()(int& converted, std::string& from)// for snmp AsnType_Integer
	{
		std::stringstream buf; 
		buf << from;
		buf >> converted;
	}
	void operator()(std::string& converted, std::string& from)// for snmp AsnType_Octets
	{
		converted = from;
	}
};
#endif//CONFIG_NO_SNMP

template < class FragmentT >
class Holder<FragmentT>::__Detail::IDetailElement
{
public:
	virtual ~IDetailElement(){}
	virtual std::string get(FragmentT &obj) = 0;
	virtual void set(FragmentT& obj, const char* value, __Detail& info) = 0;

#ifndef CONFIG_NO_SNMP
	virtual ZQ::Snmp::ManagedPtr value(FragmentT& obj)  = 0;
	virtual ZQ::Snmp::ManagedPtr name(const std::string& varName) = 0;
	virtual ZQ::Snmp::ManagedPtr readWrite(void) = 0;
#endif//CONFIG_NO_SNMP
};

template < class FragmentT >
template<class Type>
class Holder<FragmentT>::__Detail::__DetailElement : public IDetailElement
{
public:
	__DetailElement(Type& refStore, size_t length)
		:_refStore(refStore), _length(length)
	{ }

	virtual ~__DetailElement(){}
	virtual std::string  get(FragmentT& obj)
	{
		return _memUtil.fromMem(obj, _refStore);	
	}
	virtual void  set(FragmentT& obj, const char* value, __Detail& info)
	{
		_memUtil.toMem(obj, _refStore, value, info, _length); 
	}

#ifndef CONFIG_NO_SNMP
	virtual ZQ::Snmp::ManagedPtr value(FragmentT& obj)
	{
		using namespace ZQ::Snmp;
		typedef  typename DeduceType<Type>::regVarType     ValueVar;
		typedef  typename DeduceType<Type>::regResultType  RegSnmp;

		SnmpConvert  convertor;
		ValueVar     convartedVar;		
		std::string  mem2Str( _memUtil.fromMem(obj, _refStore) );

		convertor((typename DeduceType<Type>::regVarType&)convartedVar, mem2Str);

		return  ManagedPtr(new SimpleObject( VariablePtr(new RegSnmp(convartedVar)), DeduceType<Type>::Result, aReadOnly) );
	}
	virtual ZQ::Snmp::ManagedPtr name(const std::string& varName)		
	{
		using namespace ZQ::Snmp;
		typedef  DECLARE_SNMP_RO_TYPE(std::string, std::string, std::string)  roSnmpString;
		return ManagedPtr(new SimpleObject( VariablePtr(new roSnmpString(varName)), AsnType_Octets, aReadOnly) );
	}
	virtual ZQ::Snmp::ManagedPtr readWrite(void)		
	{
		using namespace ZQ::Snmp;
		typedef  DECLARE_SNMP_RO_TYPE(int, int, int)  roSnmpInt;
		return ManagedPtr(new SimpleObject( VariablePtr(new roSnmpInt(1)), AsnType_Integer, aReadOnly) );
	}
#endif//CONFIG_NO_SNMP

private:
	template<class DT, class TDefault = void>  struct MemUtil
	{// add "class TDefault = void" for g++ support explicit specialization at class scope, if there could be a way for member template specialization
		void toMem(FragmentT& obj, DT refStore, const char* value, __Detail& , size_t )
		{
			std::istringstream inBuf;  
			inBuf.str(value);
			inBuf >> (obj.*(DT)refStore);
		}
		std::string fromMem(FragmentT& obj, DT refStore)
		{
			std::ostringstream buf; 
			buf << (obj.*(DT)refStore); 
			return buf.str(); 
		}
	};
	template<class TDefault>  struct MemUtil<PMem_StdString, TDefault>
	{
		void toMem(FragmentT& obj, PMem_StdString refStore, const char* value, __Detail& , size_t )
		{
			(obj.*(PMem_StdString)refStore) = value;
		}
		std::string fromMem(FragmentT& obj, Type refStore)
		{
			return (obj.*(PMem_StdString)refStore); 
		}
	};
	template<class TDefault> struct MemUtil<PMem_CharArray, TDefault>    
	{
		void toMem(FragmentT& obj, PMem_CharArray refStore, const char* value, __Detail& info, size_t length)
		{
			size_t varLength = strlen(value);
			if (varLength >= length)
				throwf<CfgException>(EXFMT(CfgException, "Detail::set() value to long, path[%s], name[%s], length limit[%u]. root=%s"),
				info.path.c_str(), info.name.c_str(), length, info.root.c_str());

			strncpy(&(obj.*(PMem_CharArray)refStore), value, length);
		}
		std::string fromMem(FragmentT& obj, PMem_CharArray refStore)
		{
            return std::string( &(obj.*(PMem_CharArray)refStore) ); 
		}
	};

private:
	Type            _refStore;
	size_t          _length;   // for vtCharArray only
	MemUtil<Type>   _memUtil;
};

template < class FragmentT >
void Loader<FragmentT>::setLogger(ZQ::common::Log* pLog)
{
	__m_pLog = pLog;
	__m_PP.setLogger(pLog);
	if(setConfLogInLoader()) 
	{
		setConfLog(pLog);
	}
}

template < class FragmentT >
bool Loader<FragmentT>::load(const char* path, bool enablePP/* = true*/)
{
	if(NULL == path || 0 == (*path))
		return false;

	{
		// store the config file's information
		__m_filepath = path;
		std::pair<std::string, std::string> pathInfo = parseFilePath(path);
		__m_configfolder = pathInfo.first;
		__m_filename = pathInfo.second;
	}
	if(enablePP)
		LLOG(Log::L_DEBUG, "Start loading config [%s], enable preprocess.", path);
	else 
		LLOG(Log::L_DEBUG, "Start loading config [%s], disable preprocess.", path);

	ZQ::common::XMLPreferenceDocumentEx doc;
	try
	{
		if(!doc.open(path))
		{
			LLOG(Log::L_ERROR, "Failed to open file [%s].", path);
			return false;
		}

		LLOG(Log::L_DEBUG, "Opened file [%s] successfully.", path);
		Preprocessor * hPP = NULL;
		XMLUtil::XmlNode root = XMLUtil::toShared(doc.getRootPreference());

		if(enablePP)
		{
			hPP = &__m_PP;
			LLOG(Log::L_DEBUG, "Enable the preprocessor, start initializing. file [%s]", path);
			// initialize the preprocessor
			if(XMLUtil::locate(root, "Definitions").empty())
			{
				LLOG(Log::L_WARNING, "No <Definitions> node found in file [%s]", path);
			}
			else
			{
				Holder<MacroDefinition> macroholder("");
				macroholder.pLog = __m_pLog; // pass the logger
				macroholder.folder = __m_configfolder;
				macroholder.read(root);
				if(!__m_PP.define(macroholder.macros))
				{
					LLOG(Log::L_ERROR, "Failed to initialize preprocessor's macro definition in file [%s].", path);
					return false;
				}

				LLOG(Log::L_DEBUG, "Initialized preprocessor successfully for file [%s]", path);
			}
		}

		this->read(root, hPP);
	}
	catch(XMLException &e)
	{
		LLOG(Log::L_ERROR, "ZQ::common::XMLExcetion caught during parsing [%s]. desc [%s]", path, e.getString());
		return false;
	}
	catch(CfgException &e)
	{
		LLOG(Log::L_ERROR, "ZQ::common::CfgException caught during loading config [%s]. desc [%s]", path, e.getString());
		return false;
	}
	catch (Exception &e)
	{
		LLOG(Log::L_ERROR, "ZQ::common::Exception caught during loading config [%s]. desc [%s]", path, e.getString());
		return false;
	}
	catch(...)
	{
		LLOG(Log::L_ERROR, "Unknown exception caught during loading config [%s]", path);
		return false;
	}
	LLOG(Log::L_INFO, "Successful to load config [%s].", path);
	return true;
}

template < class FragmentT >
bool Loader<FragmentT>::loadInFolder(const char *folder, bool enablePP/* = true*/)
{
	if(NULL == folder || '\0' == (*folder))
	{
		return false;
	}
	LLOG(Log::L_DEBUG, "Start loading config file [%s] in folder [%s]", __m_filename.c_str(), folder);
	if(__m_filename.empty())
	{
		LLOG(Log::L_ERROR, "No config file name setting during loading config in folder [%s].", folder);
		return false;
	}
	// construct the file path
	std::string filepath = folder;
	if(FNSEPC != filepath[filepath.size() - 1])
		filepath.push_back(FNSEPC);

	filepath += __m_filename;
	return load(filepath.c_str(), enablePP);
}

template < class FragmentT >
void Holder<FragmentT>::read(XMLUtil::XmlNode node, const Preprocessor * hPP/* = NULL*/)
{
	if(!node)
	{
		throwf<CfgException>(EXFMT(CfgException, "Holder::read() bad node object"));
	}
	__m_rootPath = XMLUtil::fullPath(node);
	// step 1: gather the structure of the config
	{
		__m_details.clear();
		__m_others.clear();
	}
	FragmentT::structure(*this);

	// step 2: read the config of this fragment
	__readThis(node, hPP);

	// step 3: read the config of other fragments
	__readOthers(node, hPP);
}

template < class FragmentT > 
template<class AddressType>
void Holder<FragmentT>::addDetail(const std::string &path, const std::string &name, AddressType address, 
				const char *defaultValue/* = NULL*/, SnmpOption snmpOpt/* = optNone*/, const std::string &snmpName/* = ""*/)
{
	addDetail(path, name, address, 0, defaultValue, snmpOpt, snmpName);
}

template < class FragmentT >
template<class AddressType>
void Holder<FragmentT>::addDetail(const std::string &path, const std::string &name, AddressType address, size_t length, 
				const char *defaultValue, SnmpOption snmpOpt/* = optNone*/, const std::string &snmpName/* = ""*/)
{
	__Detail attrDetail(__m_rootPath, path, name, snmpOpt, snmpName);
	attrDetail.setAddress(address, (int)length);
	if(defaultValue)
		attrDetail.setDefault(*this, defaultValue);

	__m_details.push_back(attrDetail);
}

template < class FragmentT >
void Holder<FragmentT>::addDetail(const std::string &path, ReadOther readFunc, RegisterOther registerFunc, 
								  const Range nodeCount/* = Range(0, -1)*/, const std::string &snmpName/* = ""*/)
{
	__m_others.push_back(__Other(path, readFunc, registerFunc, nodeCount, snmpName));
}

template < class FragmentT >
void Holder<FragmentT>::snmpRegister(const std::string &full_path)
{
	// update the full path to this node
	std::string fullPath = "";
	// parse the path
	typedef std::vector< std::string > StandardPath;
	StandardPath stdpath;
	ZQ::common::stringHelper::SplitString(full_path, stdpath, "/", "/");
	if(!__m_keyAttr.empty())
	{
		std::string keyAttrValue;
		for(typename __Details::iterator it_detail = __m_details.begin(); it_detail != __m_details.end(); ++it_detail)
		{
			if(it_detail->path.empty() && it_detail->name == __m_keyAttr)
			{
				keyAttrValue = it_detail->get(*this); // get the attribute value
				break;
			}
		}
		if(keyAttrValue.empty())
			throwf<CfgException>(EXFMT(CfgException, "Holder::snmpRegister() bad value of key attribute [%s]. root=%s"), __m_keyAttr.c_str(), __m_rootPath.c_str());

		std::string tagName = "";
		if(!stdpath.empty())
		{
			tagName = stdpath.back();
			stdpath.pop_back();
		}
		// generate a unique tag name with the key attribute value
		stdpath.push_back(tagName + "[" + keyAttrValue + "]");
	}
	// generate the full path to this node
	{
		for (StandardPath::iterator it = stdpath.begin(); it != stdpath.end(); ++it)
		{
			fullPath += (*it);
			fullPath += "/";
		}
	}
	// step 1: register the variables with detail info
	for(typename __Details::iterator it_detail = __m_details.begin(); it_detail != __m_details.end(); ++it_detail)
	{
		it_detail->snmpRegister(*this, fullPath);
	}
	// step 2: register other fragment's variables by the register function
	for(typename __Others::iterator it_other = __m_others.begin(); it_other != __m_others.end(); ++it_other)
	{
		std::string pathToOther = fullPath + (it_other->snmpName.empty() ? it_other->path : it_other->snmpName);
		(this->*(it_other->registerFunc))(pathToOther);
	}
}

template < class FragmentT >
void Holder<FragmentT>::__readThis(XMLUtil::XmlNode node, const Preprocessor* hPP)
{
	for(typename __Details::iterator it_detail = __m_details.begin(); it_detail != __m_details.end(); ++it_detail)
	{
		XMLUtil::XmlNodes nodes = XMLUtil::locate(node, it_detail->path);
		if(nodes.size() != 1)
			throwf<CfgException>(EXFMT(CfgException, "Holder::__readThis() bad xml definition, found %u nodes of path [%s]. root=%s"), nodes.size(), it_detail->path.c_str(), __m_rootPath.c_str());

		// read the config of this attribute
		XMLUtil::XmlNode target    = nodes[0];
        const int CFG_BUFSIZE      = 512;
		char      buf[CFG_BUFSIZE] = {0};
		char*     value = buf;

		if(target->getAttributeValue(it_detail->name.c_str(), buf, sizeof(buf)))
		{
			if(hPP)
			{
				std::string str = buf;
				hPP->fixup(str);
				if( str.size() >= sizeof(buf) )
					throwf<CfgException>(EXFMT(CfgException, "Holder::__readThis() insufficient buffer. path [%s], attribute[%s], attribute size [%u], buffer size[%u]. root=%s")
					, it_detail->path.c_str(), it_detail->name.c_str(), str.size(), CFG_BUFSIZE, __m_rootPath.c_str());

				strcpy(buf, str.c_str());
			}
			
#ifdef CHECK_WITH_GLOG
			// print the config value through glog
			ZQ::common::Log* confGlog = getConfLog();
			if (confGlog) 
			{
				char namebuf[CFG_BUFSIZE] = {0};
				target->getPreferenceName(namebuf, false, CFG_BUFSIZE);
				(*confGlog)(Log::L_DEBUG, "got config item [%s] : [%s] = [%s]", namebuf, it_detail->name.c_str(), value);
			}
#endif //CHECK_WITH_GLOG
		}

		it_detail->set(*this, value);
	}
}

template < class FragmentT >
void Holder<FragmentT>::__readOthers(XMLUtil::XmlNode node, const Preprocessor* hPP)
{
	for(typename __Others::iterator it_other = __m_others.begin(); it_other != __m_others.end(); ++it_other)
	{
		XMLUtil::XmlNodes target = XMLUtil::locate(node, it_other->path);
		if(target.size() < it_other->nodeCount.first || it_other->nodeCount.second < target.size())
		{
			throwf<CfgException>(EXFMT(CfgException, "Holder::__readOthers() bad xml definition, found %u nodes of path [%s], violate the range[%u, %u]. root=%s"),
				target.size(), it_other->path.c_str(), it_other->nodeCount.first, it_other->nodeCount.second, __m_rootPath.c_str());
		}

		for(XMLUtil::XmlNodes::iterator it_node = target.begin(); it_node != target.end(); ++it_node)
		{
			(this->*(it_other->readFunc))(*it_node, hPP); // read other nodes
		}
	}
}


} // namespace Config
} // namespace common
} // namespace ZQ

#endif//__ZQ_COMMON_CONFIG_HELPER_HPP_IMPLEMENT__
#ifndef __ZQ_COMMON_CONFIG_HELPER_H__
#define __ZQ_COMMON_CONFIG_HELPER_H__
#include <ZQ_common_conf.h>
#include <XMLPreferenceEx.h>
#include <errno.h>

#define CONFIG_NO_SNMP

//#ifndef CONFIG_NO_SNMP // disable SNMP if need
//#	ifdef ZQ_OS_MSWIN
//	#include <ZQSNMPManPkg.h>
//#	else
//	#include <SnmpManPkg.h>
//#	endif
//#endif
#include <climits>
#include <strHelper.h>
#include <boost/shared_ptr.hpp>

namespace ZQ {
namespace common {

// -----------------------------
// exception CfgException
// -----------------------------
class CfgException : public ZQ::common::Exception
{
public:
	CfgException(const std::string& what_arg) throw()
		:ZQ::common::Exception(what_arg)
	{
	}
};

// -----------------------------
// exception NavigationException
// -----------------------------
class NavigationException : public CfgException
{
public:
	NavigationException(const std::string& what_arg) throw()
		:CfgException(what_arg)
	{
	}
};

// -----------------------------
// exception PreprocessException
// -----------------------------
class PreprocessException : public CfgException
{
public:
	PreprocessException(const std::string& what_arg) throw()
		:CfgException(what_arg)
	{
	}
};
template< class ExceptionT >
void throwf(const char *fmt, ...)  PRINTFLIKE(1, 2);

template< class ExceptionT >
void throwf(const char *fmt, ...)
{
	char msg[2048] = {0};
	va_list args;
	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);
	throw ExceptionT(msg);
}
#define EXFMT(T, fmt) (#T " : " fmt)

// -----------------------------
// namespace XMLUtil
// -----------------------------
namespace XMLUtil{

	/// smart pointer type of xml dom node.
	typedef boost::shared_ptr< ZQ::common::XMLPreferenceEx > XmlNode;
	/// group type of xml dom node
	typedef std::vector< XmlNode > XmlNodes;

	/// Convert an original pointer of XMLPreferenceEx to a smart pointer.
	/// @param p original pointer.
	/// @return the smart pointer that hold p.
	XmlNode toShared(ZQ::common::XMLPreferenceEx* p);

	/// Locate the target xml nodes through a path string.
	/// @param[in]  root the start node.
	/// @param[out] path the relative path from the root that specify the target node.
	///                  A path string is a filepath-like string that consist of a list of
	///                  tag name that separated by '/'.
	/// @return the target xml nodes. The result may contain nothing if no target node found.
	/// @note The function may throw a NavigationException if it can't locate the target nodes.
	///       The reason may be a bad root node object or an ill path.
	XmlNodes locate(XmlNode root, const std::string &path);

	/// get the full path of the node
	/// @param[in] the target node
	/// @return the node's full path string
	std::string fullPath(XmlNode node);
} // namespace XMLUtil

//////////////////////////////////////////////////////////////////////////
typedef std::vector< std::pair< std::string, std::string > > Macros;

// -----------------------------
// class Preprocessor
// -----------------------------
/// Preprocessor implement the macro replacement function.
class Preprocessor
{
public:
	Preprocessor():_pLog(NULL){}
	/// Define a macro.
	/// @param[in]  macroName the name of the macro. The macroName must consist of
	///                       [A-Za-z0-9_] only.
	/// @param[in]  macroDef the definition of the macro. The macroDef may embed
	///                      other macros that can be expanded.
	/// @return true for success and false for failure.
	bool define(const std::string &macroName, const std::string &macroDef);

	bool define(const Macros& macros);

	/// Expand the macros in the string.
	/// @param[in,out]  str a string with macro embedded.
	/// @return true for success and false for failure.
	/// @note A macro reference follow the patten ${MACRONAME}. The function will replace
	///       every macro reference with the correspond definition.
	bool fixup(std::string &str) const;

	void setLogger(ZQ::common::Log *pLog) { _pLog = pLog; }
private:

	typedef std::map< std::string, std::string > VariableMap;
	VariableMap _variables;
	ZQ::common::Log *_pLog;
};

//////////////////////////////////////////////////////////////////////////
namespace Config {

// getter & setter of the global logger in the config module
Log* getConfLog();
void setConfLog(ZQ::common::Log*);
// set/check the flag that if the program set the global logger pointer
// in the Loader::setLogger(). Pass NULL for check-only.
// default flag: true
bool setConfLogInLoader(bool* pEnabled = NULL);

/// SNMP register option
enum SnmpOption
{
	optNone,
	optReadOnly,
	optReadWrite
};
typedef std::pair<uint32, uint32> Range;

// -----------------------------
// class Holder
// -----------------------------
/// Holder class that read and hold the content of the config
template < class FragmentT >
class Holder : public FragmentT
{
public:
	typedef int32 FragmentT::*       PMem_Int32;     ///< type of pointer to int32 member
	typedef char FragmentT::*        PMem_Char;      ///< type of pointer to char member
	typedef PMem_Char                PMem_CharArray; ///< type of pointer to char[] member
	typedef std::string FragmentT::* PMem_StdString; ///< type of pointer to std::string member

	/// member function type of reading the content of other config fragment
	typedef void (FragmentT::* ReadOther)(XMLUtil::XmlNode node, const Preprocessor* hPP);
	/// member function type of registering snmp viable of other config fragment
	typedef void (FragmentT::* RegisterOther)(const std::string &full_path);
public:
	explicit Holder(const std::string& keyAttr = "")
		:__m_keyAttr(keyAttr)
	{
	}

	/// Read the content of config from xml dom node.
	/// @param[in]  node the xml dom node that contain the config data.
	/// @param[in]  hPP the Preprocessor object handle that was used to expand the xml config data.
	void read(XMLUtil::XmlNode node, const Preprocessor* hPP = NULL)
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

	/// Add config's detail info to the Holder object.
	/// @param[in]  path the relative path of the target node.
	/// @param[in]  name the attribute of this config item.
	/// @param[in]  address the address of the config's storage for int32 type.
	/// @param[in]  defaultValue the default value of this config item. NULL for required config.
	/// @param[in]  snmpOpt snmp register option.
	/// @param[in]  snmpName a short name of this item for the snmp variable management.
	void addDetail(const std::string &path, const std::string &name, PMem_Int32 address, const char *defaultValue = NULL, SnmpOption snmpOpt = optNone, const std::string &snmpName = "")
	{
		__Detail attrDetail(__m_rootPath, path, name, snmpOpt, snmpName);
		attrDetail.setAddress(address);
		if(defaultValue)
			attrDetail.setDefault(*this, defaultValue);
		__m_details.push_back(attrDetail);
	}

	/// Add config's detail info to the Holder object.
	/// @param[in]  address the address of the config's storage for std::string type.
	void addDetail(const std::string &path, const std::string &name, PMem_StdString address, const char *defaultValue = NULL, SnmpOption snmpOpt = optNone, const std::string &snmpName = "")
	{
		__Detail attrDetail(__m_rootPath, path, name, snmpOpt, snmpName);
		attrDetail.setAddress(address);
		if(defaultValue)
			attrDetail.setDefault(*this, defaultValue);
		__m_details.push_back(attrDetail);
	}

	/// Add config's detail info to the Holder object.
	/// @param[in]  address the address of the config's storage for char[] type, may need type cast.
	/// @param[in]  length the config's storage's size.
	void addDetail(const std::string &path, const std::string &name, PMem_CharArray address, size_t length, const char *defaultValue = NULL, SnmpOption snmpOpt = optNone, const std::string &snmpName = "")
	{
		__Detail attrDetail(__m_rootPath, path, name, snmpOpt, snmpName);
		attrDetail.setAddress(address, length);
		if(defaultValue)
			attrDetail.setDefault(*this, defaultValue);
		__m_details.push_back(attrDetail);
	}

	/// Add a millisecond attribute to the Holder object.
	/// @param[in]  path the relative path of the target node.
	/// @param[in]  name the attribute of this config item.
	/// @param[in]  address the address of the config's storage for int32 type.
	/// @param[in]  defaultValue the default value of this config item. NULL for required config.
	/// @param[in]  snmpOpt snmp register option.
	/// @param[in]  snmpName a short name of this item for the snmp variable management.
	void addMilliSecond(const std::string &path, const std::string &name, PMem_Int32 address, const char *defaultValue = NULL, SnmpOption snmpOpt = optNone, const std::string &snmpName = "")
	{
		__Detail attrDetail(__m_rootPath, path, name, snmpOpt, snmpName);
		attrDetail.setAddressMillisecond(address);
		if(defaultValue)
			attrDetail.setDefault(*this, defaultValue);
		__m_details.push_back(attrDetail);
	}

	/// Add config's detail info to the Holder object.
	/// @param[in]  path the relative path of the target node.
	/// @param[in]  readFunc the member function that read the content of other config fragment.
	/// @param[in]  readFunc the member function that register snmp variable of other config fragment.
	/// @param[in]  nodeCount the acceptable range of the target node's count.
	/// @param[in]  snmpName a short name of this item for the snmp variable management.
	void addDetail(const std::string &path, ReadOther readFunc, RegisterOther registerFunc, const Range nodeCount = Range(0, -1), const std::string &snmpName = "")
	{
		__m_others.push_back(__Other(path, readFunc, registerFunc, nodeCount, snmpName));
	}

	/// Register SNMP variables.
	/// @param[in]  full_path the variable name's prefix which also the full path of this fragment's node.
	void snmpRegister(const std::string &full_path)
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

private:
	void __readThis(XMLUtil::XmlNode node, const Preprocessor* hPP)
	{
		for(typename __Details::iterator it_detail = __m_details.begin(); it_detail != __m_details.end(); ++it_detail)
		{
			XMLUtil::XmlNodes nodes = XMLUtil::locate(node, it_detail->path);
			if(nodes.size() != 1)
				throwf<CfgException>(EXFMT(CfgException, "Holder::__readThis() bad xml definition, found %u nodes of path [%s]. root=%s"), 
				(unsigned int)nodes.size(), it_detail->path.c_str(), __m_rootPath.c_str());

			// read the config of this attribute
			XMLUtil::XmlNode target = nodes[0];

#define CFG_BUFSIZE 512
			char buf[CFG_BUFSIZE];
			buf[0] = '\0';

			char* value = NULL;
			if(target->getAttributeValue(it_detail->name.c_str(), buf, sizeof(buf)))
			{
				if(hPP)
				{
					std::string str = buf;
					hPP->fixup(str);
					if(str.size() < sizeof(buf))
					{
						strcpy(buf, str.c_str());
					}
					else
					{
						throwf<CfgException>(EXFMT(CfgException, "Holder::__readThis() insufficient buffer. path [%s], attribute[%s], attribute size [%u], buffer size[%u]. root=%s"),
							it_detail->path.c_str(), it_detail->name.c_str(), (unsigned int)str.size(), (unsigned int)CFG_BUFSIZE, __m_rootPath.c_str());
					}
				}
				value = buf;
#ifdef CHECK_WITH_GLOG
				// print the config value through glog
				ZQ::common::Log* confGlog = getConfLog();
				if (confGlog) {
					char namebuf[CFG_BUFSIZE];
					namebuf[0] = '\0';
					target->getPreferenceName(namebuf, false, CFG_BUFSIZE);
					(*confGlog)(Log::L_DEBUG, "got config item [%s] : [%s] = [%s]"
						, namebuf, it_detail->name.c_str(), value);
				}
#endif
			}
#undef CFG_BUFSIZE

			it_detail->set(*this, value);
		}
	}

	void __readOthers(XMLUtil::XmlNode node, const Preprocessor* hPP)
	{
		for(typename __Others::iterator it_other = __m_others.begin(); it_other != __m_others.end(); ++it_other)
		{
			XMLUtil::XmlNodes target = XMLUtil::locate(node, it_other->path);
			if(target.size() < it_other->nodeCount.first || it_other->nodeCount.second < target.size())
			{
				throwf<CfgException>(EXFMT(CfgException, "Holder::__readOthers() bad xml definition, found %u nodes of path [%s], violate the range[%u, %u]. root=%s"),
					(unsigned int)target.size(), it_other->path.c_str(), (unsigned int)it_other->nodeCount.first, (unsigned int)it_other->nodeCount.second, __m_rootPath.c_str());
			}

			for(XMLUtil::XmlNodes::iterator it_node = target.begin(); it_node != target.end(); ++it_node)
			{
				(this->*(it_other->readFunc))(*it_node, hPP); // read other nodes
			}
		}
	}

private:
	struct __Detail
	{
		__Detail(const std::string& rootPath, const std::string &nodepath, const std::string &attrname, SnmpOption snmpOpt, const std::string &snmpName = "")
			:root(rootPath), path(nodepath), name(attrname), _snmpOpt(snmpOpt), _snmpName(snmpName)
		{
			_type = dtVoid;
			_address = 0;
			_optional = false;
			_length = true;
		}

		char* skipWhiteSpace(char* data) 
		{
			while(*data != '\0') 
			{
				if(!(*data == ' ' || *data == '\t' || *data == '\r' || *data == '\n' || *data == '\v' || *data == '\f')) 
					break;
				data++;
			}

			return data;
		}

		long str2long(const char* str)
		{	
			if(str== NULL || str[0] == '\0')
				return 0;

			char* data = strdup(str);
			data = skipWhiteSpace(data);
			if(*data == '\0')
			{
				free(data);
				return 0;
			}

			char* pDigitBegin = data;
			size_t dotCount = 0;
			while(*data != '\0')
			{
				if(isdigit(*data) || *data == '.') {
					if(*data == '.') {
						dotCount++;
					}
					data++;						
					continue;
				}
				break;
			}

			char* pDigitEnd = data;

			if(dotCount > 1)
			{
				free(data);
				errno = ERANGE; //this is the only error we can returned
				return LONG_MAX;
			}

			char tmp = *pDigitEnd;
			*pDigitEnd = '\0';
			double ret = atof(pDigitBegin);
			*pDigitEnd = tmp;
			data = skipWhiteSpace(data);
			if(*data == '\0')
			{
				/*			if(dotCount > 0) {
				free(pDigitBegin);
				errno = ERANGE;
				return LONG_MAX;
				}		
				*pDigitEnd = '\0';
				*/
				long ret = atol(pDigitBegin);		
				free(pDigitBegin);
				return ret;
			}

			char nextCh = *(data+1);
			bool f1024Mode = nextCh == 'i' || nextCh == 'I';
			size_t shiftCount = 0;
			switch(*data)
			{
			case 'k':
			case 'K':
				shiftCount = 1;
				break;

			case 'm':
			case 'M':
				shiftCount = 2;
				break;

			case 'g':
			case 'G':
				shiftCount = 3;
				break;

			default:
				shiftCount = 0;
				break;
			}

			long factor = 1;
			for(size_t i = 0 ; i < shiftCount; i++ )
				factor = factor * (f1024Mode ? 1024 : 1000);

			*pDigitEnd = '\0';
			if(dotCount>0) {
				//float
				double dblData = atof(pDigitBegin);
				dblData = dblData * factor;
				ret = (long)dblData;
			} 
			else {
				//long		
				ret = atol(pDigitBegin);
				ret = ret * factor;		
			}

			free(pDigitBegin);
			return (long)ret;
		}

		long str2msec(const char* str)
		{	
			// supports format of: [Ndays][Mmin][Xsec], such as 1.5days, 4.0hours, 1.5secs 4.1minutes 1hr50sec123, 1234, 1.234
			if (str== NULL || str[0] == '\0')
				return 0;

			std::string valstr = str;

			size_t pos = valstr.find_first_not_of(" \t\r\n");
			if (std::string::npos != pos)
				valstr.erase(0, pos);

			pos = valstr.find_last_not_of(" \t\r\n");
			if (std::string::npos != pos)
				valstr.erase(pos+1, valstr.length());

			if (std::string::npos == (pos = valstr.find_first_not_of("0123456789")))
				return str2long(valstr.c_str());

			std::transform(valstr.begin(), valstr.end(), valstr.begin(), tolower);

			std::string field;
			double val =0.0f;
			if (std::string::npos != (pos= valstr.find('d'))) // day
			{
				field = valstr.substr(0, pos);
				val += atof(field.c_str());
				valstr = valstr.substr(pos+1);

				if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
					valstr = valstr.substr(pos);
			}
			val*=24;
			
			if (std::string::npos != (pos= valstr.find('h'))) // hour
			{
				field = valstr.substr(0, pos);
				val += atof(field.c_str());
				valstr = valstr.substr(pos+1);

				if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
					valstr = valstr.substr(pos);
			}
			val*=60;

			if (std::string::npos != (pos= valstr.find('m'))) // minute
			{
				field = valstr.substr(0, pos);
				val += atof(field.c_str());
				valstr = valstr.substr(pos+1);

				if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
					valstr = valstr.substr(pos);
			}
			val*=60;

			field ="";
			if (std::string::npos != (pos= valstr.find('s')))
			{
				field = valstr.substr(0, pos);
				valstr = valstr.substr(pos+1);
			}
			else if (std::string::npos != (pos= valstr.find('.')))
			{
				pos= valstr.find_first_not_of("0123456789", pos+1);
				field = valstr.substr(0, pos);
				valstr = (std::string::npos != pos) ? valstr.substr(pos+1) :"";
			}

			if (std::string::npos != (pos = valstr.find_first_of("0123456789")))
				valstr = valstr.substr(pos);

			if (!field.empty()) // second
				val += atof(field.c_str());

			val*=1000;

			// the millisecond
			val += atol(valstr.c_str());

			return (long) val;
		}

		std::string get(FragmentT &obj)
		{
			switch(_type)
			{
			case dtInt32:
				{
					int32 val = (obj.*(PMem_Int32)_address);
					char buf[22] = {0};
					return itoa(val, buf, 10);
				}

			case dtStdString:
				return (obj.*(PMem_StdString)_address);

			case dtCharArray:
				return (&(obj.*(PMem_CharArray)_address));

			default:
				throwf<CfgException>(EXFMT(CfgException, "Detail::get() bad data type, type[%d]"), (int)_type);
				return "";
			}
		}

		void set(FragmentT& obj, const char* value)
		{
			if(NULL == value)
			{
				if(_optional)
					return; // leave with default value silently
				else
					throwf<CfgException>(EXFMT(CfgException, "Detail::set() attribute missed, path[%s], name[%s]. root=%s"), path.c_str(), name.c_str(), root.c_str());
			}

			switch(_type)
			{
			case dtInt32:
				{
					//long lval = strtol(value, NULL, 0);
					long lval = str2long(value);
					if( (LONG_MAX == lval ||LONG_MIN == lval)
						&& (ERANGE == errno))
					{
						throwf<CfgException>(EXFMT(CfgException, "Detail::set() integer attribute range error, path[%s], name[%s], value[%s]. root=%s"), 
							path.c_str(), name.c_str(), value, root.c_str());
					}
					(obj.*(PMem_Int32)_address) = lval;
				}
				break;

			case dtMillisecond:
				{
					long lval = str2msec(value);
					if( (LONG_MAX == lval ||LONG_MIN == lval) && (ERANGE == errno))
					{
						throwf<CfgException>(EXFMT(CfgException, "Detail::set() millisecond attribute range error: path[%s] name[%s] value[%s] root=%s"), 
							path.c_str(), name.c_str(), value, root.c_str());
					}
					(obj.*(PMem_Int32)_address) = lval;
				}
				break;

			case dtStdString:
				(obj.*(PMem_StdString)_address) = value;
				break;

			case dtCharArray:
				if(strlen(value) < _length)
					strncpy(&(obj.*(PMem_CharArray)_address), value, _length);
				else
					throwf<CfgException>(EXFMT(CfgException, "Detail::set() value to long, path[%s], name[%s], length limit[%u]. root=%s"), 
					path.c_str(), name.c_str(), (unsigned int)_length, root.c_str());
				break;

			default:
				throwf<CfgException>(EXFMT(CfgException, "Detail::set() bad data type, type[%d]"), (int)_type);
			}
		}

		void setDefault(FragmentT& obj, const std::string& defaultValue)
		{
			_optional = true;
			set(obj, defaultValue.c_str());
		}

		void setAddress(PMem_Int32 address)
		{
			_type = dtInt32;
			_address = (PMem_Void)address;
		}

		void setAddressMillisecond(PMem_Int32 address)
		{
			_type = dtMillisecond;
			_address = (PMem_Void)address;
		}

		void setAddress(PMem_StdString address)
		{
			_type = dtStdString;
			_address = (PMem_Void)address;
			if(_snmpOpt == optReadWrite)
				_snmpOpt = optReadOnly;
		}

		void setAddress(PMem_CharArray address, size_t length)
		{
			_type = dtCharArray;
			_address = (PMem_Void)address;
			_length = length;
		}

		void snmpRegister(FragmentT& obj, const std::string &full_path)
		{
#ifndef CONFIG_NO_SNMP // disable SNMP if need
			// step 1: compute the snmp properties of the variable
#ifdef ZQ_OS_MSWIN
			BOOL varRO = TRUE;
			switch(_snmpOpt)
			{
			case optNone:
				return;
			case optReadOnly:
				break;
			case optReadWrite:
				varRO = FALSE;
				break;
			default:
				throwf<CfgException>(EXFMT(CfgException, "Detail::snmpRegister() bad snmp option, option[%d]"), _snmpOpt);
			}
#else
			bool varRO = true;
			switch(_snmpOpt)
			{
			case optNone:
				return;
			case optReadOnly:
				break;
			case optReadWrite:
				varRO = false;
				break;
			default:
				throwf<CfgException>(EXFMT(CfgException, "Detail::snmpRegister() bad snmp option, option[%d]"), _snmpOpt);
			}
#endif

			uint32 varType = -1;
			void *varAddress = NULL;
			switch(_type)
			{
			case dtInt32:
			case dtMillisecond:
				varType = ZQSNMP_VARTYPE_INT32;
				varAddress = &(obj.*(PMem_Int32)_address);
				break;

			case dtCharArray:
				varType = ZQSNMP_VARTYPE_STRING;
				varAddress = &(obj.*(PMem_CharArray)_address);
				break;

			case dtStdString:
				varType = ZQSNMP_VARTYPE_STRING;
				varAddress = (void*)((obj.*(PMem_StdString)_address).c_str());
				break;

			default:
				throwf<CfgException>(EXFMT(CfgException, "Detail::snmpRegister() bad data type, type[%d]"), _type);
			}

			// step 2: generate the variable's full name
			std::string varName = full_path;
			if (!_snmpName.empty())
			{
				varName += _snmpName;
			}
			else
			{
				if(!path.empty())
				{
					varName += path;
					varName += "/";
				}
				varName += name;
			}
			// step 3: register the variable
			SNMPManageVariable(varName.c_str(), varAddress, varType, varRO);
#endif
		}

	public:
		std::string root;
		std::string path;
		std::string name;

	private:
		enum DataType
		{
			dtVoid,
			dtInt32,
			dtCharArray,
			dtStdString,
			dtMillisecond
		} _type;

		typedef void* FragmentT::* PMem_Void;
		PMem_Void _address;

		// for snmp variable
		SnmpOption _snmpOpt;
		std::string _snmpName;

		bool _optional; // if this attribute is optional
		size_t _length; // for vtCharArray only
	};

	typedef std::vector<__Detail> __Details;
	__Details __m_details;

	struct __Other
	{
		__Other(const std::string &thePath, ReadOther theReadFunc, RegisterOther theRegisterFunc, const Range &theNodeCount, const std::string &theSnmpName)
			:path(thePath), readFunc(theReadFunc), registerFunc(theRegisterFunc), nodeCount(theNodeCount), snmpName(theSnmpName)
		{
		}
		std::string path;
		ReadOther readFunc;
		RegisterOther registerFunc;
		Range nodeCount;
		std::string snmpName;
	};

	typedef std::vector<__Other> __Others;
	__Others __m_others;

	std::string __m_keyAttr;
	std::string __m_rootPath; // the root path of the node that this holder bind
};

// -----------------------------
// auxiliary classes
// -----------------------------

struct NVPair
{
	std::string name;
	std::string value;
	static void structure(Holder<NVPair>& holder)
	{
		holder.addDetail("", "name", &NVPair::name);
		holder.addDetail("", "value", &NVPair::value, NULL, optReadOnly);
	}
};

struct MacroDefinition
{
	std::string folder;
	std::string src;
	Macros macros;
	Log *pLog;
	MacroDefinition():pLog(NULL){}
	static void structure(Holder<MacroDefinition>& holder);
	void readMacroReference(XMLUtil::XmlNode node, const Preprocessor* hPP);
	void readMacro(XMLUtil::XmlNode node, const Preprocessor* hPP);
	void registerNothing(const std::string&){}
};

// -----------------------------
// interface ILoader
// -----------------------------
class ILoader
{
public:
	virtual ~ILoader(){}
	/// Load config.
	/// @param[in]  path the config file's full path.
	/// @param[in]  enablePP enable/disable the preprocess
	/// @return true for success and false for failure.
	virtual bool load(const char *path, bool enablePP) = 0;

	/// Load config in config folder.
	/// @param[in]  folder the config folder.
	/// @param[in]  enablePP enable/disable the preprocess
	/// @return true for success and false for failure.
	virtual bool loadInFolder(const char *folder, bool enablePP) = 0;

	/// get the config file's folder
	virtual const std::string& getConfigFolder() = 0;

	/// get the config file's name
	virtual const std::string& getConfigFileName() = 0;

	/// get the config file's full path
	virtual const std::string& getConfigFilePath() = 0;

	/// set logger instance.
	/// @param[in]  pLog the logger instance's pointer.
	virtual void setLogger(ZQ::common::Log* pLog) = 0;
};

/// auxiliary function of parsing a file path
/// @return a pair of strings that the first member contains the file's folder
///         and the second member contains the file name.
std::pair<std::string, std::string> parseFilePath(const std::string &path);

#define LLOG if (__m_pLog) (*__m_pLog)

// -----------------------------
// class Holder
// -----------------------------
/// Loader class that load config from file.
template< class FragmentT >
class Loader : public Holder< FragmentT >, public ILoader
{
public:
	/// Constructor
	/// @param[in]  cfgFileName config file's name.
	explicit Loader(const std::string &cfgFileName)
		:Holder<FragmentT>(""), __m_filename(cfgFileName), __m_pLog(NULL)
	{
	}
	/// Destructor
	virtual ~Loader(){}

	/// refer to the preprocessor object of the Loader
	Preprocessor& PP(){ return __m_PP; }
	/// set logger instance.
	/// @param[in]  pLog the logger instance's pointer.
	virtual void setLogger(ZQ::common::Log* pLog)
	{
		__m_pLog = pLog;
		__m_PP.setLogger(pLog);
		if(setConfLogInLoader()) {
			setConfLog(pLog);
		}
	}

	/// Load config.
	/// @param[in]  path the config file's full path.
	/// @param[in]  enablePP enable/disable the preprocess
	/// @return true for success and false for failure.
	virtual bool load(const char* path, bool enablePP = true)
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

		LLOG(Log::L_DEBUG, CLOGFMT(Loader, "loading config[%s] %s preprocessor"), path, enablePP?"with":"withno");

		ZQ::common::XMLPreferenceDocumentEx doc;
		try
		{
			if(!doc.open(path))
			{
				LLOG(Log::L_ERROR, CLOGFMT(Loader, "failed to open file[%s]"), path);
				return false;
			}

			LLOG(Log::L_DEBUG, CLOGFMT(Loader, "opened file[%s]"), path);

			XMLUtil::XmlNode root = XMLUtil::toShared(doc.getRootPreference());
			if (enablePP)
			{
				LLOG(Log::L_DEBUG, CLOGFMT(Loader, "enabling preprocessor on file[%s]"), path);
				// initialize the preprocessor
				if (XMLUtil::locate(root, "Definitions").empty())
				{ LLOG(Log::L_WARNING, CLOGFMT(Loader, "no elem <Definitions> found in file[%s]"), path); }
				else
				{
					Holder<MacroDefinition> macroholder("");
					macroholder.pLog = __m_pLog; // pass the logger
					macroholder.folder = __m_configfolder;
					macroholder.read(root);
					if(!__m_PP.define(macroholder.macros))
					{
						//failed to initialize the preprocessor.
						LLOG(Log::L_ERROR, CLOGFMT(Loader, "failed to initialize preprocessor on file[%s]."), path);
						return false;
					}

					LLOG(Log::L_DEBUG, CLOGFMT(Loader, "initialized preprocessor on file[%s]"), path);
				}

				this->read(root, &__m_PP);
			}
			else
			{
				this->read(root, NULL);
			}
		}
		catch(XMLException &e)
		{
			LLOG(Log::L_ERROR, CLOGFMT(Loader, "XMLExcetion caught when parsing [%s]: %s"), path, e.getString());
			return false;
		}
		catch(CfgException &e)
		{
			LLOG(Log::L_ERROR, CLOGFMT(Loader, "CfgException caught when loading [%s]: %s"), path, e.getString());
			return false;
		}
		catch (Exception &e)
		{
			LLOG(Log::L_ERROR, CLOGFMT(Loader, "Exception caught when loading [%s]: %s"), path, e.getString());
			return false;
		}
		catch(...)
		{
			LLOG(Log::L_ERROR, CLOGFMT(Loader, "exception caught during loading [%s]"), path);
			return false;
		}

		LLOG(Log::L_INFO, CLOGFMT(Loader, "loaded config[%s]."), path);
		return true;
	}

	/// Load config in config folder.
	/// @param[in]  folder the config folder.
	/// @param[in]  enablePP enable/disable the preprocess
	/// @return true for success and false for failure.
	virtual bool loadInFolder(const char *folder, bool enablePP = true)
	{
		if(NULL == folder || '\0' == (*folder))
			return false;

		if(__m_filename.empty())
		{
			LLOG(Log::L_ERROR, CLOGFMT(Loader, "empty filename specified when loading config in folder[%s]"), folder);
			return false;
		}

		LLOG(Log::L_DEBUG, CLOGFMT(Loader, "loading config file[%s] in folder[%s]"), __m_filename.c_str(), folder);

		// construct the file path
		std::string filepath = folder;
		if(FNSEPC != filepath[filepath.size() - 1])
			filepath.push_back(FNSEPC);

		filepath += __m_filename;
		return load(filepath.c_str(), enablePP);
	}

	/// get the config file's folder
	virtual const std::string& getConfigFolder()
	{
		return __m_configfolder;
	}

	/// get the config file's name
	virtual const std::string& getConfigFileName()
	{
		return __m_filename;
	}

	/// get the config file's full path
	virtual const std::string& getConfigFilePath()
	{
		return __m_filepath;
	}

private:
	std::string __m_filename;
	std::string __m_configfolder;
	std::string __m_filepath;
	Preprocessor __m_PP;
	ZQ::common::Log* __m_pLog;
};

}}} // namespace ZQ::common::Config

#endif // __ZQ_COMMON_CONFIG_HELPER_H__

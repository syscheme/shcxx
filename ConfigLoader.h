#ifndef _BASE_CONFIG__PARSER_H__
#define _BASE_CONFIG__PARSER_H__

#ifdef ZQ_OS_MSWIN
#pragma warning(disable:4503)
#endif

#include "Log.h"
#include "XMLPreferenceEx.h"
#include "strHelper.h"
#include <map>
#include <string>

namespace ZQ{
namespace common{
	
template<class classType>
class auto_free
{
public:
	auto_free(classType _p)
	{
		_ptr=_p;
	}
	~auto_free()
	{
		if(_ptr!=NULL)
		{
			_ptr->free();
			_ptr=NULL;
		}
	}	
public:
	classType& operator->()
	{
		return _ptr;
	}
	classType& operator=(const classType t)
	{
		if(_ptr!=NULL)
		{
			_ptr->free();
			_ptr=NULL;
		}
		_ptr=t;
		return _ptr;
	}
	operator classType()
	{
		return _ptr;
	}
	bool operator==(classType& t)
	{
		if(t==_ptr)
			return true;
		else
			return false;
	}
	bool operator!()
	{
		return !_ptr;
	}
public:
	classType	_ptr;
};
typedef auto_free<ZQ::common::XMLPreferenceEx*>		PREF;


class ZQ_COMMON_API ConfigLoader
{
public:
	ConfigLoader();
	virtual ~ConfigLoader();

public:
	typedef struct _tagConfigSchemaItem
	{
		char*			path;		
		char*			key;
		void*			value;
		int				vLen;		
		bool			bReadOnly;
		enum DataType
		{
			TYPE_INTEGER,
			TYPE_FLOAT,
			TYPE_STRING,
			TYPE_ENUM
		}type;		
	} ConfigSchemaItem, *PConfigSchema;
	typedef std::map<std::string,std::string>	KEYVALUEMAP;
	typedef std::vector<KEYVALUEMAP>			VECKVMAP;
	typedef std::map<std::string,VECKVMAP>	ENUMMAP;

public:
	virtual bool						load(const TCHAR* szConfigPath);
	
	virtual bool						loadWithConfigFolder(const TCHAR* szConfigFolder=NULL);

	//set configuration file name(NOT full path name)
	//ConfigLoader will combination configFolder and configFilename,and load the configuration
	void								setConfigFileName(const TCHAR* strFileName)
	{
		m_strConfFileName=strFileName;
	}
	const TCHAR*						getLastErrorDesc()
	{
		return m_strLastErr.c_str();
	}
	const TCHAR*						getConfigFilePathName()
	{
		return m_strConfPathName.c_str();
	}
	///Get Enum value with it's name
	///Example:
	//	<IceProperties>
	//			<prop name="propName" value="propValue" />
	//			<prop name="propName" value="propValue" />
	//	</IceProperties>
	///you can get the key-value pair through the name 'prop'	
	VECKVMAP&							getEnumValue(const std::string& strName);

	KEYVALUEMAP							getNameValuePair(const std::string& strName);

	void								SetLogInstance(ZQ::common::Log* pLog)
	{
		m_pLog=pLog;
	}
protected:
	
	virtual PConfigSchema		getSchema();
	virtual PConfigSchema		getDefault();


	bool						Parse(ConfigSchemaItem** pPreperty,ZQ::common::XMLPreferenceEx* pPref);
	bool						ParseChild(ConfigSchemaItem** p,ZQ::common::XMLPreferenceEx* pPref,int& TargetLevel);	
	int							CheckLevel(ConfigSchemaItem* p);
	bool						ParseNode(ConfigSchemaItem** p,ZQ::common::XMLPreferenceEx* pPref);
	void						SetLastErr(const TCHAR* fmt,...) PRINTFLIKE(2, 3);
	void						log(ZQ::common::Log::loglevel_t level,const char* fmt,...) PRINTFLIKE(3, 4);
private:	
	class levelControl
	{
	public:
		levelControl(int &level):m_level(level)
		{
			m_level++;
		}
		~levelControl()
		{
			m_level--;
		}
	private:
		int& m_level;
	};
	ZQ::common::stringHelper::STRINGVECTOR		m_curPath;	
	ZQ::common::stringHelper::STRINGVECTOR		m_lastVecPath;
	int											m_currentLevel;
	int											m_sameLevel;
	bool										m_bFirstCheck;
	ENUMMAP										m_mapEnum;
	ZQ::common::Log	*							m_pLog;
	
#if defined _UNICODE || defined UNICODE
	std::wstring								m_strLastErr;
	std::wstring								m_strConfFileName;
	std::wstring								m_strConfPathName;
#else//UNICODE
	std::string									m_strLastErr;
	std::string									m_strConfFileName;
	std::string									m_strConfPathName;
#endif//UNICODE
	
};
}}//namespace ZQ::common
#endif//_BASE_CONFIG__PARSER_H__

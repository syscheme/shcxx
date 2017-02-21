#include "ConfigLoader.h"
#include "Exception.h"

extern "C" {
#include <stdio.h>
#include <stdarg.h>
}

#include <string>
#ifdef ZQ_OS_MSWIN
#include <tchar.h>
#include <ZQSNMPManPkg.h>
#else
#include "SnmpManPkg.h"
#endif

//#define MYLOG(level,x)	if(m_pLog){(*m_pLog)(level,x);}

using namespace ZQ::common;

ConfigLoader::ConfigLoader()
{
	m_bFirstCheck=true;
	m_currentLevel=0;
	m_sameLevel=0;
	m_pLog=NULL;
	setConfigFileName(_T(""));
}
ConfigLoader::~ConfigLoader()
{
}
ConfigLoader::ConfigSchemaItem* ConfigLoader::getSchema()
{
	static ConfigSchemaItem defaultCfgMap[] = {		
		{NULL,NULL,NULL,0,true,ConfigSchemaItem::TYPE_STRING}
	};
	
	return defaultCfgMap;	
}
ConfigLoader::ConfigSchemaItem* ConfigLoader::getDefault()
{
	return NULL;
}
void ConfigLoader::SetLastErr(const TCHAR* fmt,...)
{
	TCHAR szBuf[1024];
//	ZeroMemory(szBuf,sizeof(szBuf));
	memset(szBuf,0,sizeof(szBuf));
	va_list args;
	va_start(args, fmt);
	_vstprintf(szBuf, fmt, args);
	va_end(args);
	m_strLastErr=szBuf;
}
void ConfigLoader::log(ZQ::common::Log::loglevel_t level,const char* fmt,...)
{
	if(!m_pLog)
		return;
	char szBuf[1024];
//	ZeroMemory(szBuf,sizeof(szBuf));
	memset(szBuf,0,sizeof(szBuf));
	va_list args;
	va_start(args, fmt);
	vsprintf(szBuf, fmt, args);
	va_end(args);	
	(*m_pLog)(level,szBuf);
}
bool ConfigLoader::loadWithConfigFolder(const TCHAR* szConfigFolder/* =NULL */)
{
	if( !(szConfigFolder && _tcsclen(szConfigFolder) > 0  ) )
	{
		SetLastErr(_T("invalid folder"));
		return false;
	}
	if(m_strConfFileName.empty())
	{
		log(ZQ::common::Log::L_INFO,"No config file is specified ,return with succeed");
		return true;
	}
#if defined _UNICODE || defined UNICODE
	std::wstring strFile;
#else
	std::string	strFile;
#endif
	strFile=szConfigFolder;
	if(strFile.at(strFile.length()-1)!=_T(FNSEPC))
		strFile+=_T(FNSEPS);
	strFile+=m_strConfFileName;
	return load(strFile.c_str());
}
bool ConfigLoader::load(const TCHAR* szConfigPath)
{
	if(!( szConfigPath && _tcsclen( szConfigPath )>0 ))
	{
		SetLastErr(_T("invalid configuration file path name"));
		return false;
	}
	m_strConfPathName=szConfigPath;
//	ComInitializer init;

	ZQ::common::XMLPreferenceDocumentEx root;
	

	PREF pref = NULL,  child = NULL;
	try
	{
		char* pFilePath=NULL;
#if defined _UNICODE || defined UNICODE
		int iStringSize=WideCharToMultiByte(CP_ACP,0,szConfigPath,wcslen(szConfigPath),NULL,0,NULL,NULL);
		pFilePath=new char[iStringSize+1];
//		ZeroMemory(pFilePath,(iStringSize+1));
		memset(pFilePath,0,(iStringSize+1));
		WideCharToMultiByte(CP_ACP,0,szConfigPath,wcslen(szConfigPath),pFilePath,iStringSize,NULL,NULL);		
#else
		pFilePath=(char*)szConfigPath;
#endif
		if(!root.open(pFilePath))
		{
			SetLastErr(_T("can't open configuration file %s"),szConfigPath);
			return false;
		}
#if defined _UNICODE ||defined UNICODE
		delete[] pFilePath;
		pFilePath=NULL;
#endif
		
		pref=root.getRootPreference();
		ConfigSchemaItem* p=getSchema();
		if(!p)
		{
			SetLastErr(_T("no configuration Schema is found"));
			return false;
		}
		if(!Parse(&p,pref))
		{
			return false;
		}
		p=getDefault();
		if(p)
		{
			return Parse(&p,pref);
		}

	}
	catch (ZQ::common::Exception& e)
	{
#if defined UNICODE || defined _UNICODE

		TCHAR *pBuf=NULL;
		int iLens=strlen( e.what() );
		pBuf=new TCHAR[iLens+1];
//		ZeroMemory(pBuf,(iLens+1)*sizeof(TCHAR));
		memset(pBuf,0,(iLens+1)*sizeof(TCHAR));
		MultiByteToWideChar(CP_ACP,
							0,
							e.what(),
							iLens,
							pBuf,
							iLens);
		SetLastErr(pBuf);
		delete[] pBuf;
#else
		SetLastErr(e.getString());
#endif	
		return false;			 
	}
	catch (...)
	{
		m_strLastErr=_T("unexpect error when parse configuration file ");
		m_strLastErr+=szConfigPath;
		return false;
	}
	return true;
}

bool ConfigLoader::Parse(ConfigSchemaItem** p,ZQ::common::XMLPreferenceEx* pPref)
{
	if(! (p && *p) )
	{
		SetLastErr(_T("invalid configuration schema"));		
		return false;
	}
	if(m_currentLevel!=0)
	{
		SetLastErr(_T("parse configuration failed"));
		return false;
	}
	int iLevel=CheckLevel(*p);
	if(iLevel<0)
	{
		SetLastErr(_T("invalid schema"));
		return false;
	}
	while ((*p)->key!=NULL)
	{
		if(!ParseChild(p,pPref,iLevel))
			return false;		
	}
	return true;
}

bool ConfigLoader::ParseNode(ConfigSchemaItem** p,ZQ::common::XMLPreferenceEx* pPref)
{
	if(!p)
	{
		SetLastErr(_T("invalid configuration schema"));
		return false;
	}
	char szBuf[1024];
	if((*p)->type==ConfigLoader::ConfigSchemaItem::TYPE_ENUM)
	{
		PREF pChild=pPref->firstChild( (*p)->key );
		while (pChild!=NULL)
		{
			KEYVALUEMAP proper=pChild->getProperties();
			//m_mapEnum.insert(std::make_pair<std::string,KEYVALUEMAP>((*p)->key,proper));
			m_mapEnum[(*p)->key].push_back(proper);
			

			KEYVALUEMAP::const_iterator itTemp=proper.begin();
			for(;itTemp!=proper.end();itTemp++)
			{
				log(ZQ::common::Log::L_INFO,"Get enum [%s/%s]\tkey=[%s]\tvalue=[%s]",
							(char*)((*p)->path),(*p)->key,itTemp->first.c_str(),itTemp->second.c_str());
#ifdef _DEBUG
				printf("ENUM(%s)  key(%s)\tvalue(%s)\n",(*p)->key,itTemp->first.c_str(),itTemp->second.c_str());
#endif
			}
			//enum attribute here
			pChild=pPref->nextChild();
		}
		
	}
	else
	{		
//		ZeroMemory(szBuf,sizeof(szBuf));
		memset(szBuf,0,sizeof(szBuf));
		pPref->get((*p)->key,szBuf,"",sizeof(szBuf)-1);
		log(ZQ::common::Log::L_INFO,"Get value [%s/%s] =  [%s]",(char*)((*p)->path),(char*)((*p)->key),(char*)szBuf);
#ifdef _DEBUG
		printf("find a key '%s' with value '%s' \n",(*p)->key,szBuf);
#endif

		uint32 snmpDataType=0;
	
		switch((*p)->type)
		{
		case ConfigLoader::ConfigSchemaItem::TYPE_INTEGER:
			{
                snmpDataType = ZQSNMP_VARTYPE_INT32;
				if(strlen(szBuf)<=0)
				{
					log(ZQ::common::Log::L_INFO,"no value is found ,use default value key=%s/%s value=%d",
						(*p)->path,(*p)->key,*(long*)((*p)->value));
				}
				else
				{
					long lTemp=atol(szBuf);
					if((*p)->vLen < (int)sizeof(lTemp))
						log(ZQ::common::Log::L_WARNING," there is not enough room to hold key=%s/%s and valye=%s  dataType is INTEGER dataSize=%d",
						(*p)->path,(*p)->key,szBuf,(*p)->vLen);
					/*memcpy((*p)->value,&lTemp, (*p)->vLen);*/
					*((long*)(*p)->value)=lTemp;
				}
			}
			break;
		case ConfigLoader::ConfigSchemaItem::TYPE_STRING:
			{
#ifdef ZQ_OS_MSWIN
                snmpDataType = ZQSNMP_VARTYPE_STRING;
#else
                snmpDataType = ZQSNMP_VARTYPE_CSTRING;
#endif
				if(strlen(szBuf)<=0)
				{
					log(ZQ::common::Log::L_INFO,"no value is found ,use default value key=%s/%s and value=%s",
						(*p)->path,(*p)->key,(char*)((*p)->value));
				}
				else
				{
					if((*p)->vLen < (int)strlen(szBuf)+1)
						log(ZQ::common::Log::L_WARNING," there is not enough room to hold key=%s/%s and valye=%s  dataType is STRING dataSize=%d",
						(*p)->path,(*p)->key,szBuf,(*p)->vLen);
					memcpy((*p)->value,szBuf,((int)(strlen(szBuf)+1)>(*p)->vLen)?(*p)->vLen:(strlen(szBuf)+1) );
				}
			}
			break;
            //removed by xiaohui.chai
            /*
		case ConfigLoader::ConfigSchemaItem::TYPE_FLOAT:
			{
				if(strlen(szBuf)<=0)
				{
					log(ZQ::common::Log::L_INFO,"no value is found ,use default value key=%s value=%f",(*p)->key,*(double*)((*p)->value));
				}
				else
				{
					snmpDataType=ZQSNMP_FLOAT;
					float fTemp=(float)atof(szBuf);
					if((*p)->vLen < sizeof(fTemp))
						log(ZQ::common::Log::L_WARNING," there is not enough room to hold key=%s/%s and valye=%s dataType is FLOAT dataSize=%d",
						(*p)->path,(*p)->key,szBuf,(*p)->vLen);
					//memcpy((*p)->value,&fTemp, (*p)->vLen);
					*((float*)(*p)->value)=fTemp;
				}
			}
            */
		default:
			{
				SetLastErr(_T("not supported schema data type"));
				return false;
			}
			break;
		}
	
		
		//find a valid configuration,register it in SNMP
//		unsigned long dwRet=0;
		TCHAR*	pKey=NULL;
		char*	pTempBuf=NULL;
		
		int iNameSize = strlen( (*p)->path );
		iNameSize += strlen( (*p)->key );
		iNameSize += 2; //for . and \0
		
		pTempBuf=new char[iNameSize];
//		ZeroMemory(pTempBuf,iNameSize);
		memset(pTempBuf,0,iNameSize);

		strcpy(pTempBuf,(*p)->path);
		strcat(pTempBuf,".");
		strcat(pTempBuf,(*p)->key);
#if defined _UNICODE ||defined UNICODE
		
		int iStringSize=MultiByteToWideChar(CP_ACP,0,pTempBuf,strlen(pTempBuf),NULL,0);
		pKey=new TCHAR[iStringSize+1];
//		ZeroMemory(pKey,(iStringSize+1)*sizeof(TCHAR));
		memset(pKey,0,(iStringSize+1)*sizeof(TCHAR));
		MultiByteToWideChar(CP_ACP,0,pTempBuf,strlen(pTempBuf),pKey,iStringSize);
#else
		pKey=pTempBuf;
#endif

		std::string strKeyName = (std::string)((*p)->path) + (std::string)("/") + (const char*)((*p)->key);
		//SNMPManagerAddVar((char*)strKeyName.c_str(),(DWORD)(*p)->value,snmpDataType,(*p)->bReadOnly,&dwRet);
        SNMPManageVariable(strKeyName.c_str(), (*p)->value, snmpDataType, (*p)->bReadOnly);

#if defined _UNICODE ||defined UNICODE
		delete[] pKey;
		pKey=NULL;
		delete[] pTempBuf;
		pTempBuf=NULL;
#endif
	}
	return true;
}

bool ConfigLoader::ParseChild(ConfigSchemaItem** p,ZQ::common::XMLPreferenceEx* pPref,int& TargetLevel)
{	
	if((*p)->key==NULL)
		return true;
	if( !( p && (*p) ) )
	{
		SetLastErr(_T("invalid configuration schema"));
		return false;
	}
	
	levelControl lc(m_currentLevel);
//	const char* pTest=m_lastVecPath[m_currentLevel-1].c_str();
	PREF pChild=pPref->firstChild(m_lastVecPath[m_currentLevel-1].c_str());
	if(!pChild)
	{
		//SetLastErr(_T("Parse configuration failed,no %s is found in configuration file"),pTest);
		
		switch( (*p)->type)
		{
		case ConfigLoader::ConfigSchemaItem::TYPE_INTEGER:
			log(ZQ::common::Log::L_INFO,"Can't find path %s,use default value [%s]=[%d]", (*p)->path,(*p)->key,*(int*)(*p)->value);
			break;
		case ConfigLoader::ConfigSchemaItem::TYPE_FLOAT:
			log(ZQ::common::Log::L_INFO,"Can't find path %s,use default value [%s]=[%f]", (*p)->path,(*p)->key,*(float*)(*p)->value);
			break;
		case ConfigLoader::ConfigSchemaItem::TYPE_STRING:
			log(ZQ::common::Log::L_INFO,"Can't find path %s,use default value [%s]=[%s]", (*p)->path,(*p)->key,(char*)(*p)->value);
			break;
		case ConfigLoader::ConfigSchemaItem::TYPE_ENUM:
			log(ZQ::common::Log::L_INFO,"Can't find value with path=[%s] key=[%s]", (*p)->path,(*p)->key);
			break;
		default:		
			break;
		}		
		(*p)++;
		if((*p)!=NULL && (*p)->key!=NULL)
		{
			TargetLevel=CheckLevel(*p);
		}
		return true;
	}
	if(TargetLevel>m_currentLevel)
	{//go to next level
		do 
		{
			if(p!=NULL && (*p)->key!=NULL)
			{
				if(!ParseChild(p,pChild,TargetLevel))
					return false;
				if(TargetLevel <= m_currentLevel || m_sameLevel<m_currentLevel)
					return true;
			}
			else
			{
				return true;
			}

		
		} while(1);
	}
	else if(TargetLevel<m_currentLevel)
	{
		SetLastErr(_T("Parse configuration failed"));
		return false;
	}
	else/* if(TargetLevel==m_currentLevel)*/
	{
		//parse it
		ParseNode(p,pChild);
		(*p)++;
		if((*p)!=NULL && (*p)->key!=NULL)
		{
			TargetLevel=CheckLevel(*p);
		}
	}
	return true;
}
int ConfigLoader::CheckLevel(ConfigSchemaItem* p)
{
	if(!p)
	{
		SetLastErr(_T("invalid configuration schema"));
		return -1;
	}

	ZQ::common::stringHelper::STRINGVECTOR vecTempPath;
	ZQ::common::stringHelper::SplitString((p)->path,vecTempPath,"/"," ;\t\"/");	

	int iLevel=0;	
	if(m_bFirstCheck)
	{
	
		m_bFirstCheck=false;
		iLevel=0;
	}
	else
	{		
		ZQ::common::stringHelper::STRINGVECTOR::const_iterator it=m_lastVecPath.begin();
		int iVecSize=vecTempPath.size();
		for(;it!=m_lastVecPath.end(),iLevel<iVecSize;it++,iLevel++)
		{
			if((*it)!=vecTempPath[iLevel])
			{
				break;
			}
		}		
	}
	m_sameLevel=iLevel;
	if(vecTempPath.size()>0)
	{
		m_lastVecPath=vecTempPath;
	}
	return vecTempPath.size();
}
ConfigLoader::VECKVMAP&	ConfigLoader::getEnumValue(const std::string& strName)
{
	return m_mapEnum[strName];
}
ConfigLoader::KEYVALUEMAP	ConfigLoader::getNameValuePair(const std::string& strName)
{
	ConfigLoader::VECKVMAP& vecKV=getEnumValue(strName);
	KEYVALUEMAP valueMap;
	int iSize=(int)vecKV.size();
	for(int i=0;i<iSize;i++)
	{
		valueMap.insert(std::make_pair<std::string,std::string>(vecKV[i]["name"],vecKV[i]["value"]));
	}
	return valueMap;
}


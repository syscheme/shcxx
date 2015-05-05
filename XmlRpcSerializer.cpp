
#include "XmlRpcSerializer.h"
#include "base64.h"

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef ZQ_OS_MSWIN
#  include <TCHAR.h>
#endif
}

#include <sstream>   // for sstream

namespace ZQ {
namespace XmlRpc {

#define ATT_ID_START 1000
#define DATATIME_FMT "%4d%2d%2dT%2d:%2d:%2d"

/// -----------------------------
/// class XmlRpcUnserializer
/// -----------------------------
XmlRpcUnserializer::XmlRpcUnserializer(ZQ::common::Variant& var, tistream& istrm)
:Unserializer(var, istrm)
{
}

XmlRpcUnserializer::~XmlRpcUnserializer()
{
}

const _TCHAR* XmlRpcSerializer::BOOLEAN_TAG   = _T("boolean");
const _TCHAR* XmlRpcSerializer::DOUBLE_TAG    = _T("double");
const _TCHAR* XmlRpcSerializer::INT_TAG       = _T("int");
const _TCHAR* XmlRpcSerializer::I4_TAG        = _T("i4");
const _TCHAR* XmlRpcSerializer::STRING_TAG    = _T("string");
const _TCHAR* XmlRpcSerializer::DATETIME_TAG  = _T("dateTime.iso8601");
const _TCHAR* XmlRpcSerializer::BASE64_TAG    = _T("base64");

const _TCHAR* XmlRpcSerializer::ARRAY_TAG     = _T("array");

const _TCHAR* XmlRpcSerializer::STRUCT_TAG    = _T("struct");

const _TCHAR* XmlRpcSerializer::NAME_TAG      = _T("name");
const _TCHAR* XmlRpcSerializer::VALUE_TAG     = _T("value");

const _TCHAR* XmlRpcSerializer::DATA_TAG      = _T("data");
const _TCHAR* XmlRpcSerializer::MEMBER_TAG    = _T("member");

typedef enum 
{
	ATT_VAR     = -100,
	ATT_NAME    =ZQ::common::Variant::T_STRING,
	ATT_MEMBER  =ZQ::common::Variant::T_STRUCT +ATT_ID_START,
	ATT_DATA,
	ATT_VALUE,
} add_tagtype;


void XmlRpcUnserializer::unserialize() throw (ZQ::common::UnserializeException)
{
	_TCHAR line [80];

	int len = sizeof(line)/sizeof(_TCHAR);

	try {

		//	bLogicalClosed =false;
		while (len >= int(sizeof(line)/sizeof(_TCHAR)) -1)
		{
			_istrm.get(line, sizeof(line)/sizeof(_TCHAR));
			len =  _tcsclen(line);
			parse(line, len, false);
		}

		parse(line, 0, true);
	}
	catch (const ZQ::common::ExpatException& ex)
	{
		throw ZQ::common::UnserializeException(ex.what());
	}
	catch (...)
	{
		throw ZQ::common::UnserializeException("unserialize caught unknown exception");
	}
}

static int tagtoid(tstring typeTag)
{
#define TAG_ASSET(_STR, _ID) if (typeTag == _STR) { return _ID; }

	TAG_ASSET(XmlRpcSerializer::BOOLEAN_TAG,	ZQ::common::Variant::T_BOOL);
	TAG_ASSET(XmlRpcSerializer::I4_TAG,		ZQ::common::Variant::T_INT);
	TAG_ASSET(XmlRpcSerializer::INT_TAG,		ZQ::common::Variant::T_INT);
	
	TAG_ASSET(XmlRpcSerializer::DOUBLE_TAG,	ZQ::common::Variant::T_DOUBLE);
	TAG_ASSET(XmlRpcSerializer::STRING_TAG,	ZQ::common::Variant::T_STRING);
	TAG_ASSET(XmlRpcSerializer::DATETIME_TAG,	ZQ::common::Variant::T_TIME);
	TAG_ASSET(XmlRpcSerializer::BASE64_TAG,	ZQ::common::Variant::T_BASE64);
	TAG_ASSET(XmlRpcSerializer::ARRAY_TAG,	ZQ::common::Variant::T_ARRAY);
	TAG_ASSET(XmlRpcSerializer::STRUCT_TAG,	ZQ::common::Variant::T_STRUCT);
	TAG_ASSET(XmlRpcSerializer::NAME_TAG,		ATT_NAME);
	TAG_ASSET(XmlRpcSerializer::VALUE_TAG,	ATT_VALUE);
	TAG_ASSET(XmlRpcSerializer::DATA_TAG,		ATT_DATA);
	TAG_ASSET(XmlRpcSerializer::MEMBER_TAG,	ATT_MEMBER);

	return -10;
}

// overrideable callbacks, from ExpatBase
void XmlRpcUnserializer::OnStartElement(const XML_Char* name, const XML_Char** atts)
{
	node_t node;
	node.tag = tagtoid(name);

	if (node.tag >= 0)
		_stack.push_back(node);
#if defined(_DEBUG) && 0
	dumpstack(_stack);
#endif
}

#if defined(_DEBUG)
void XmlRpcUnserializer::_dumpstack(stack_t& stk)
{
#if 0
	printf("\n>> dump stack...\n");
	for (stack_t::iterator i = stk.begin(); i< stk.end(); i++)
	{
		if (i -> tag == ATT_VAR)
		{
#if 0
			std::strstream ss;
			XmlRpcSerializer(i->val, ss).serialize();


			printf("VALUE (%d) %s\n", i->val.type(), ss.str());
#else
			printf("VALUE (%d)\n", i->val.type());
#endif
		}
		else printf("TAG (%d)\n", i->tag);
	}
#endif
}
#endif // #if defined(_DEBUG)

static void trim(tstring& str)
{
	static const _TCHAR* WHITESPACES=_T(" \t\r\n");
	tstring::size_type pos = str.find_last_not_of(WHITESPACES);
	if(pos != tstring::npos)
	{
		str.erase(pos + 1);
		pos = str.find_first_not_of(WHITESPACES);
		if(pos != tstring::npos) str.erase(0, pos);
	}
	else str.erase(str.begin(), str.end());
}

void XmlRpcUnserializer::OnCharData(const XML_Char* data , int len)
{
	node_t node;
	tstring tmpbody;
#if defined(_DEBUG) && 0
	dumpstack(_stack);
#endif
	try{
		node = _stack.back();
		bool bPoped= false;

		// get the previous data to append
		if (node.tag == ATT_VAR && (node.val.type() == ZQ::common::Variant::T_STRING || node.val.type() == ZQ::common::Variant::T_NIL))
		{
			if (node.val.type() == ZQ::common::Variant::T_STRING)
				tmpbody = (tstring&)node.val;

			_stack.pop_back();
			bPoped = true;

		// ignore any non-atomic value tag
//		if (_stack.back().tag < ATT_ID_START && (node.val.type() == ZQ::common::Variant::T_STRING || node.val.type() == ZQ::common::Variant::T_NIL))
//		{
		}

		tmpbody += tstring(data, len);
		trim(tmpbody);
		node.tag = ATT_VAR;
		node.val = tmpbody;

//		if (node.tag == ATT_VAR  && (node.val.type() == ZQ::common::Variant::T_STRING || node.val.type() == ZQ::common::Variant::T_NIL))
//			node.val = tmpbody;
//		node_t& top = _stack.back();

		if ((_stack.back().tag < ATT_ID_START && _stack.back().tag >0) || bPoped)
		{
			_stack.push_back(node);
			return;
		}
	}
	catch(...) {}
#if defined(_DEBUG) && 0
	dumpstack(_stack);
#endif
}

void XmlRpcUnserializer::OnEndElement(const XML_Char* name)
{
	int crnt_tag = tagtoid(name);
#if defined(_DEBUG) && 0
	dumpstack(_stack);
#endif
	node_t& node = _stack.back();
	
	stack_t values;
	while(node.tag == ATT_VAR)
	{
		values.push_back(node);
		_stack.pop_back();
		node = _stack.back();
	}
#if defined(_DEBUG) && 0
	dumpstack(values);
#endif

	if (node.tag != crnt_tag)
		throwExcpt(_T("XmlRpc tag stack mismatch, tag=%d"), crnt_tag);

	node_t new_node;
	ZQ::common::Variant var;
	int i =0;

	switch(node.tag)
	{
	case ZQ::common::Variant::T_BOOL:
			{
				bool value = (_ttol(((tstring)values.back().val).c_str()) !=0);
				var = ZQ::common::Variant(value);
			}
			break;
		
	case ZQ::common::Variant::T_INT:
			{
				long value = _ttol(((tstring)values.back().val).c_str());
				var = ZQ::common::Variant(value);
			}
			break;

	case ZQ::common::Variant::T_DOUBLE:
			{
				double value = atof(((tstring)values.back().val).c_str());
				var = ZQ::common::Variant(value);
			}
			break;
		
//	case ATT_NAME:
	case ZQ::common::Variant::T_STRING:
			{
				var = values.empty() ? ZQ::common::Variant(_T("")) : ZQ::common::Variant(((tstring)values.back().val));
			}
			break;
		
	case ZQ::common::Variant::T_TIME:
			{
				struct tm t;
				memset(&t, 0, sizeof(t));
				_stscanf(((tstring)values.back().val).c_str(), _T(DATATIME_FMT),&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec);
				t.tm_isdst = -1;
				var = ZQ::common::Variant(&t);
			}
			break;

	case ZQ::common::Variant::T_BASE64:
			{
				// convert from base64 to binary
				int iostatus = 0;
				base64<_TCHAR> decoder;
				ZQ::common::Variant::BinaryData bindata;
				tstring tmpbody = values.back().val;
				
				std::back_insert_iterator<ZQ::common::Variant::BinaryData> ins = std::back_inserter(bindata);
				decoder.get(tmpbody.begin(), tmpbody.end(), ins, iostatus);

				initBinary(var, bindata);
			}
			break;

	case ZQ::common::Variant::T_ARRAY:
			for (i=0; !values.empty(); values.pop_back(), i++)
			{
				var.set(i, values.back().val);
			}
			break;

	case ZQ::common::Variant::T_STRUCT:
			for (i=0; !values.empty(); values.pop_back(), i++)
			{
#if defined(_DEBUG) && 0
				dumpstack(values);
#endif
				tstring key= values.back().val;
				values.pop_back();
				if (!values.empty())
					var.set(key.c_str(), values.back().val);
			}
			break;

	case ATT_VALUE:
			{
				var = values.empty() ? ZQ::common::Variant(_T("")) : values.back().val;
			}
			break;

	case ATT_DATA:
	case ATT_MEMBER:
		_stack.pop_back();
		for (i=0; !values.empty(); values.pop_back(), i++)
		{
			_stack.push_back(values.back());
		}
		return;

	}

	_stack.pop_back();
	new_node.tag = ATT_VAR;
	new_node.val = var;
	_stack.push_back(new_node);
#if defined(_DEBUG) && 0
	dumpstack(_stack);
#endif
}

// void XmlRpcUnserializer::OnStartNamespace(const XML_Char* prefix, const XML_Char* uri);
// void XmlRpcUnserializer::OnEndNamespace(const XML_Char*);

void XmlRpcUnserializer::OnLogicalClose()
{
	if (!_stack.empty() && _stack.back().tag == ATT_VAR)
		_var = _stack.back().val;
}

void XmlRpcSerializer::serialize()
{
	serializeEx(_var, _ostrm);
}

void XmlRpcSerializer::serializeEx(ZQ::common::Variant& var, tostream& ostrm)
{
	switch (var.type())
	{
	case ZQ::common::Variant::T_BOOL: 
		{
			bool val = var;
			ostrm << "<" << BOOLEAN_TAG << ">" << (val ? 1:0) << "</" << BOOLEAN_TAG << ">";
		}
		break;

	case ZQ::common::Variant::T_INT:
		ostrm << "<" << INT_TAG << ">" << ((int)var) << "</" << INT_TAG << ">";
		break;

	case ZQ::common::Variant::T_DOUBLE:
		ostrm << "<" << DOUBLE_TAG << ">" << ((double)var) << "</" << DOUBLE_TAG << ">";
		break;

	case ZQ::common::Variant::T_TIME:
		{
			_TCHAR dtstr[80];
			struct tm tmst = var;
#ifdef ZQ_OS_MSWIN
			_stprintf(dtstr, _T(DATATIME_FMT), tmst.tm_year, tmst.tm_mon, tmst.tm_mday, tmst.tm_hour, tmst.tm_min, tmst.tm_sec);
#else
	#ifdef _UNICODE 
			swprintf(dtstr,sizeof(dtstr),_T(DATATIME_FMT),tmst.tm_year, tmst.tm_mon, tmst.tm_mday, tmst.tm_hour, tmst.tm_min, tmst.tm_sec);
	#else
			sprintf(dtstr,_T(DATATIME_FMT),tmst.tm_year, tmst.tm_mon, tmst.tm_mday, tmst.tm_hour, tmst.tm_min, tmst.tm_sec);
	#endif
#endif
			ostrm << "<" << DATETIME_TAG << ">" << dtstr << "</" << DATETIME_TAG << ">";
		}
		break;

	case ZQ::common::Variant::T_STRING:
		ostrm << "<" << STRING_TAG << ">" << ((tstring)var) << "</" << STRING_TAG << ">";
		break;

	case ZQ::common::Variant::T_BASE64:
		{
			// convert to base64
			ZQ::common::Variant::BinaryData bindata = var;
			std::vector<_TCHAR> base64data;
			int iostatus = 0;
			base64<_TCHAR> encoder;
			std::back_insert_iterator<std::vector<_TCHAR> > ins = std::back_inserter(base64data);
			encoder.put(bindata.begin(), bindata.end(), ins, iostatus, base64<>::crlf());
			
			ostrm << "<" << BASE64_TAG << ">" ;
			for (std::vector<_TCHAR>::iterator it = base64data.begin(); it < base64data.end() ; it ++)
				ostrm << ((_TCHAR) *it);
			
			ostrm << "</" << BASE64_TAG << ">";
			
		}
		break;

	case ZQ::common::Variant::T_ARRAY:
		{
			ostrm << "<" << ARRAY_TAG << ">" << "<" << DATA_TAG << ">";
			for (int i=0; i< var.size(); i++)
			{
				ostrm << "<" << VALUE_TAG << ">";
				serializeEx(var[i], ostrm);
				ostrm << "</" << VALUE_TAG << ">";
			}
			ostrm << "</" << DATA_TAG << ">" << "</" << ARRAY_TAG << ">";
		}
		break;

	case ZQ::common::Variant::T_STRUCT:
		{
			ostrm << "<" << STRUCT_TAG << ">";
			for (int i=0; i< var.size(); i++)
			{
				tstring key = var.key(i);
				ostrm << "<" << MEMBER_TAG << ">" << "<" << NAME_TAG << ">" << key <<"</" << NAME_TAG << ">"
					<< "<" << VALUE_TAG << ">";

				serializeEx(var[key], ostrm);
				ostrm << "</" << VALUE_TAG << ">" << "</" << MEMBER_TAG << ">";
			}
			ostrm << "</" << STRUCT_TAG << ">";
		}
		break;

	default:
		break;

	}
}

} // namespace common
} // namespace ZQ

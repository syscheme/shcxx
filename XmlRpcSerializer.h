#ifndef __ZQ_XMLRpcSerializer_H__
#define __ZQ_XMLRpcSerializer_H__

#include "ZQ_common_conf.h"
#include "Variant.h"
#include "expatxx.h"

#if defined(_DEBUG) && 0
#define dumpstack	_dumpstack
#else
#define dumpstack
#endif

namespace ZQ {
namespace XmlRpc {

class ZQ_COMMON_API XmlRpcSerializer;
class ZQ_COMMON_API XmlRpcUnserializer;
	
/// -----------------------------
/// class XmlRpcUnserializer
/// -----------------------------
class XmlRpcUnserializer : public ZQ::common::Unserializer, protected ZQ::common::ExpatBase
{
public:
	XmlRpcUnserializer(ZQ::common::Variant& var, tistream& istrm);
	virtual ~XmlRpcUnserializer();

	virtual void unserialize() throw (ZQ::common::UnserializeException);

protected:

	// overrideable callbacks, from ExpatBase
	virtual void OnStartElement(const XML_Char* name, const XML_Char** atts);
	virtual void OnEndElement(const XML_Char*);
	virtual void OnCharData(const XML_Char*, int len);
//	virtual void OnStartNamespace(const XML_Char* prefix, const XML_Char* uri);
//	virtual void OnEndNamespace(const XML_Char*);
	
	virtual void OnLogicalClose();
#if defined(_DEBUG)
	static void _dumpstack(stack_t& stk);
#endif // #if defined(_DEBUG)
};

class XmlRpcSerializer : public ZQ::common::Serializer
{
public:
	XmlRpcSerializer(ZQ::common::Variant& var, tostream& ostrm) : Serializer(var, ostrm) {}
	virtual ~XmlRpcSerializer(){}
	
	virtual void serialize();

	static void serializeEx(ZQ::common::Variant& var, tostream& ostrm);

	static const _TCHAR* BOOLEAN_TAG;
	static const _TCHAR* DOUBLE_TAG;
	static const _TCHAR* INT_TAG;
	static const _TCHAR* I4_TAG;
	static const _TCHAR* STRING_TAG;
	static const _TCHAR* DATETIME_TAG;
	static const _TCHAR* BASE64_TAG;
	
	static const _TCHAR* ARRAY_TAG;
	
	static const _TCHAR* STRUCT_TAG;
	
	static const _TCHAR* NAME_TAG;
	static const _TCHAR* VALUE_TAG;
	
	static const _TCHAR* DATA_TAG;
	static const _TCHAR* MEMBER_TAG;

};

} // namespace common
} // namespace ZQ

#endif//__ZQ_XMLRpcSerializer_H__

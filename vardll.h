//vardll.h
//daniel wang

#include <windows.h>
#include <wchar.h>

#define EXPORT __declspec( dllexport )
#define IMPORT __declspec( dllimport )
#define NOMORL

#ifdef _DLL
#	ifndef LIBPORT
#		define LIBPORT EXPORT
#	endif//LIBPORT
#else
#	ifndef LIBPORT
#		define LIBPORT IMPORT
#	endif//LIBPORT
#endif//_DLL

namespace _varxml
{
	class Variant;
}


class LIBPORT Variant
{
public:
	enum Type {
			TypeInvalid,
				TypeBoolean,
				TypeInt,
				TypeDouble,
				TypeString,
				TypeDateTime,
				TypeBase64,
				TypeArray,
				TypeStruct
		};

protected:
	_varxml::Variant*	m_pInstance;

	Variant(const char* str, int* offset);

	Variant(const _varxml::Variant& inst);

	Variant& operator=(const _varxml::Variant& rhs);

public:
	//default constructor
	Variant();

	//boolean
	Variant(bool value);

	//intger
	Variant(int value);

	//double float
	Variant(double value);

	//string
	Variant(const char* value);

	//date time
	Variant(tm* value);

	//base64
	Variant(void* value, int nBytes);

	
	Variant(Variant const& rhs);

	virtual ~Variant();


	void clear();

	Variant& operator=(Variant const& rhs);

	Variant& operator=(int const& rhs);

	Variant& operator=(double const& rhs);

	Variant& operator=(const char* rhs);

	bool operator==(Variant const& other) const;

	bool operator!=(Variant const& other) const;

	operator bool&();

	operator int&();

	operator double&();

	//to string
	char* getString(char* strBuffer, size_t zBufLen) const;

	operator tm&();

	//to binary data
	char* getBinaryData(char* strBuffer, size_t zBufLen) const;

	Variant operator[](int i) const;

	/*
	Variant& operator[](int i);

	Variant& operator[](const char* k);
	*/

	const Variant& setArray(int i, const Variant& var);

	Variant getArray(int i) const;

	const Variant& setStruct(const char* k, const Variant& var);

	Variant getStruct(const char* k) const;

	bool valid() const;

	Type getType() const;

	int size() const;

	void setSize(int size);

	bool hasMember(const char* name) const;

	static char* getDoubleFormat(char* strBuffer, size_t zBufLen);

	static void setDoubleFormat(const char* f);
};

class LIBPORT VariantXml : public Variant
{
public:
	//default constructor
	VariantXml();

	//boolean
	VariantXml(bool value);

	//intger
	VariantXml(int value);

	//double float
	VariantXml(double value);

	//string
	VariantXml(const char* value);

	//date time
	VariantXml(tm* value);

	//base64
	VariantXml(void* value, int nBytes);

	
	VariantXml(VariantXml const& rhs);


	VariantXml(const char* xml, int* offset);


	virtual ~VariantXml();

	bool fromXml(const char* valueXml, int* offset);

	char* toXml(char* strBuffer, size_t zLen) const;
};

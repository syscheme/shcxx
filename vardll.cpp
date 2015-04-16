
#include "vardll.h"
#include "variant.h"
#include <algorithm>
#include <assert.h>

Variant::Variant(const char* str, int* offset)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(std::string(str), offset);
}

Variant::Variant(const _varxml::Variant& inst)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(inst);
}

Variant& Variant::operator=(const _varxml::Variant& rhs)
{
	if (m_pInstance)
	{
		delete m_pInstance;
	}
	m_pInstance = new _varxml::Variant(rhs);
	return *this;
}

Variant::Variant()
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant;
}

Variant::Variant(bool value)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(value);
}

Variant::Variant(int value)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(value);
}

Variant::Variant(double value)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(value);
}

Variant::Variant(const char* value)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(value);
}

Variant::Variant(tm* value)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(value);
}

Variant::Variant(void* value, int nBytes)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(value, nBytes);
}

Variant::Variant(Variant const& rhs)
:m_pInstance(NULL)
{
	m_pInstance = new _varxml::Variant(*rhs.m_pInstance);
}

Variant::~Variant()
{
	if (m_pInstance)
	{
		delete m_pInstance;
	}
}

void Variant::clear()
{
	if (m_pInstance)
	{
		m_pInstance->clear();
	}
}

Variant& Variant::operator=(Variant const& rhs)
{
	if (m_pInstance)
	{
		delete m_pInstance;
	}
	m_pInstance = new _varxml::Variant(*rhs.m_pInstance);
	return *this;
}

Variant& Variant::operator=(int const& rhs)
	{ return operator=(Variant(rhs)); }

Variant& Variant::operator=(double const& rhs)
	{ return operator=(Variant(rhs)); }

Variant& Variant::operator=(const char* rhs)
	{ return operator=(Variant(rhs)); }

bool Variant::operator==(Variant const& other) const
{
	return *m_pInstance == *other.m_pInstance;
}

bool Variant::operator!=(Variant const& other) const
{
	return !(operator==(other));
}

Variant::operator bool&()
{
	return m_pInstance->operator bool&();
}

Variant::operator int&()
{
	return m_pInstance->operator int&();
}

Variant::operator double&()
{
	return m_pInstance->operator double&();
}


char* Variant::getString(char* strBuffer, size_t zBufLen) const
{
	assert(strBuffer);
	strncpy(strBuffer, m_pInstance->operator std::string&().c_str(), zBufLen);
	return strBuffer;
}

Variant::operator tm&()
{
	return m_pInstance->operator tm&();
}

char* Variant::getBinaryData(char* strBuffer, size_t zBufLen) const
{
	assert(strBuffer);
	std::vector<char> arrBuffer =(*m_pInstance);
	int nMin = __min(zBufLen, arrBuffer.size());
	std::copy(arrBuffer.begin(), arrBuffer.begin()+nMin, strBuffer);
	return strBuffer;
}

Variant Variant::operator[](int i) const
{
	return m_pInstance->operator[](i);
}


const Variant& Variant::setArray(int i, const Variant& var)
{
	m_pInstance->setArray(i, *var.m_pInstance);
	return *this;
}

Variant Variant::getArray(int i) const
{
	return (*m_pInstance)[i];
}

const Variant& Variant::setStruct(const char* k, const Variant& var)
{
	m_pInstance->setStruct(k, *var.m_pInstance);
	return *this;
}

Variant Variant::getStruct(const char* k) const
{
	return (*m_pInstance)[k];
}

bool Variant::valid() const
{
	return m_pInstance->valid();
}

Variant::Type Variant::getType() const
{
	return (Variant::Type)(m_pInstance->getType());
}

int Variant::size() const
{
	return m_pInstance->size();
}

void Variant::setSize(int size)
{
	m_pInstance->setSize(size);
}

bool Variant::hasMember(const char* name) const
{
	assert(name);
	return m_pInstance->hasMember(std::string(name));
}

char* Variant::getDoubleFormat(char* strBuffer, size_t zBufLen)
{
	assert(strBuffer);
	std::string strFmt = _varxml::Variant::getDoubleFormat();
	strncpy(strBuffer, strFmt.c_str(), zBufLen);
	return strBuffer;
}

void Variant::setDoubleFormat(const char* f)
{
	assert(f);
	_varxml::Variant::setDoubleFormat(f);
}













VariantXml::VariantXml()
{}

VariantXml::VariantXml(bool value)
:Variant(value)
{
}

VariantXml::VariantXml(int value)
:Variant(value)
{
}

VariantXml::VariantXml(double value)
:Variant(value)
{
}

VariantXml::VariantXml(const char* value)
:Variant(value)
{
}

VariantXml::VariantXml(tm* value)
:Variant(value)
{
}

VariantXml::VariantXml(void* value, int nBytes)
:Variant(value, nBytes)
{
}

VariantXml::VariantXml(VariantXml const& rhs)
:Variant((Variant)rhs)
{
}

VariantXml::VariantXml(const char* xml, int* offset)
:Variant(xml, offset)
{
}

VariantXml::~VariantXml()
{
}

bool VariantXml::fromXml(const char* valueXml, int* offset)
{
	return m_pInstance->fromXml(std::string(valueXml), offset);
}

char* VariantXml::toXml(char* strBuffer, size_t zLen) const
{
	assert(strBuffer);
	std::string strXml = m_pInstance->toXml();
	int nMin = __min(zLen, strXml.length());

	strncpy(strBuffer, strXml.c_str(), nMin);
	return strBuffer;
}
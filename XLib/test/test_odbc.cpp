
#include <xodbc.h>
#include <xstring.h>

using namespace ZQ;

class XTitlesRs : public XRecordset {
public:
	XTitlesRs(XDatabase* pDb = NULL) : XRecordset(pDb)
	{
		//m_szTitle[0] = _T('\0');

	}

	virtual void DoFieldExchange(XFieldExchange* pFX)
	{
		RFX_Text(pFX, _T("[Title]"), m_szTitle, 256);
		// RFX_Long(pFX, _T("[Pub_ID]"), m_PubID);
		//RFX_Text(pFX, _T("[ISBN]"), m_ISBN);
	}

	TCHAR m_szTitle[256];
	//XString m_szTitle;
	XString m_ISBN;
	LONG m_PubID;

protected:

	virtual void GetDefaultConnect(XString& strDSN, XString& strUser, XString& strPwd) 
	{
		strDSN = _T("test");
		strUser = _T("");
		strPwd = _T("");	
	}

	virtual XString GetDefaultSQL()
	{
		return _T("SELECT Title FROM Titles");
	}


};


int main()
{
	XTitlesRs rs;
	rs.Open();
	rs.Edit();
	printf("%s", (LPCTSTR )rs.m_szTitle);
	strcpy(rs.m_szTitle, "Think in Java");
	rs.Update();

	/*
	while (!rs.IsEof()) {
		printf("%s, %d\n", (LPCTSTR )rs.m_szTitle, rs.m_PubID);
		rs.MoveNext();
	}
	*/
		
	return 0;
}


// CyberstellaDoc.cpp : implementation of the CCyberstellaDoc class
//

#include "pch.hxx"
#include "Cyberstella.h"

#include "CyberstellaDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaDoc

IMPLEMENT_DYNCREATE(CCyberstellaDoc, CDocument)

BEGIN_MESSAGE_MAP(CCyberstellaDoc, CDocument)
	//{{AFX_MSG_MAP(CCyberstellaDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaDoc construction/destruction

CCyberstellaDoc::CCyberstellaDoc()
{
	// TODO: add one-time construction code here

}

CCyberstellaDoc::~CCyberstellaDoc()
{
}

BOOL CCyberstellaDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CCyberstellaDoc serialization

void CCyberstellaDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaDoc diagnostics

#ifdef _DEBUG
void CCyberstellaDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CCyberstellaDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCyberstellaDoc commands

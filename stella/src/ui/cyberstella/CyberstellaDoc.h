// CyberstellaDoc.h : interface of the CCyberstellaDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CYBERSTELLADOC_H__7FB621FC_3CB8_11D6_ACFC_0048546D2F04__INCLUDED_)
#define AFX_CYBERSTELLADOC_H__7FB621FC_3CB8_11D6_ACFC_0048546D2F04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CCyberstellaDoc : public CDocument
{
protected: // create from serialization only
	CCyberstellaDoc();
	DECLARE_DYNCREATE(CCyberstellaDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCyberstellaDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCyberstellaDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CCyberstellaDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CYBERSTELLADOC_H__7FB621FC_3CB8_11D6_ACFC_0048546D2F04__INCLUDED_)

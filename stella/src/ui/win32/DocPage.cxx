//
// StellaX
// Jeff Miller 05/12/2000
//
#include "pch.hxx"
#include "DocPage.hxx"
#include "resource.h"

#include <oleauto.h>

static LPCTSTR g_ctszDocFile = _T("\\docs\\stella.pdf");

const CLSID CPDFControl::clsid = { 0xCA8A9780, 0x280D, 0x11CF, 
	{ 0xA2, 0x4D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

void CPDFControl::OnInitialUpdate(
	void
    )
{
	// This will do a control.src = "c:\temp\a.pdf"

	IDispatch* piDispatch = NULL;
	if (GetDispInterface(&piDispatch) != S_OK)
		return;

	TCHAR tszDir[MAX_PATH + 1];
	::GetCurrentDirectory(MAX_PATH, tszDir);
	lstrcat(tszDir, g_ctszDocFile);

	VARIANT var;
	VariantInit(&var);
	var.vt = VT_BSTR;

#if defined(_UNICODE)
	BSTR bstr = ::SysAllocString(tszDir);
	if (bstr == NULL)
		return;
#else
	int nLen = ::MultiByteToWideChar(CP_ACP, 0, tszDir, -1, NULL, NULL);
	BSTR bstr = ::SysAllocStringLen(NULL, nLen);
	if (bstr == NULL)
		return;
	::MultiByteToWideChar(CP_ACP, 0, tszDir, -1, bstr, nLen);
#endif

	var.bstrVal = bstr;

	if (PutPropertyByName(L"src", &var) != S_OK)
	{
		VariantClear(&var);
		piDispatch->Release();
		return;
	}

	VariantClear(&var);

	// The following features are only in PDF 4.0 +

	VARIANT varRet;
	VariantInit(&var);

	// This will do a control.setZoom(90)

	V_VT(&var) = VT_I1;
	V_I1(&var) = 90;

	if (Invoke1(L"setZoom", &var, &varRet) != S_OK)
	{
		VariantClear(&var);
		piDispatch->Release();
		return;
	}

	VariantClear(&var);
	VariantClear(&varRet);

	piDispatch->Release();
} 

CDocPage::CDocPage(
    ) : \
	CPropertyPage(IDD_DOC_PAGE),
	m_pHost(NULL)
{
}

BOOL CDocPage::OnInitDialog(
	HWND hwnd
    )
{
	TCHAR tszDir[MAX_PATH + 1];
	::GetCurrentDirectory(MAX_PATH, tszDir);
	lstrcat(tszDir, g_ctszDocFile);

	// verify file exists

	WIN32_FIND_DATA findFileData;
	HANDLE hFind = ::FindFirstFile(tszDir, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		TCHAR tszMsg[MAX_PATH * 2];
		lstrcpy(tszMsg, _T("ERROR: Cannot find "));
		lstrcat(tszMsg, tszDir);
		
		HWND hwndInst = ::GetDlgItem(hwnd, IDC_INSTRUCTIONS);
		::SetWindowText(hwndInst, tszMsg);
		::ShowWindow(hwndInst, SW_SHOW);

		return TRUE;
	}
	::FindClose(hFind);

	m_pHost = new CControlHost(new CPDFControl);
	m_pHost->SetHwnd(hwnd);

	if (m_pHost->CreateControl() != S_OK)
	{
		::ShowWindow(::GetDlgItem(hwnd, IDC_INSTRUCTIONS), SW_SHOW);
		::ShowWindow(::GetDlgItem(hwnd, IDC_ADOBE), SW_SHOW);

		m_hlAdobe.SubclassDlgItem(hwnd, IDC_ADOBE);
		m_hlAdobe.SetURL(_T("http://www.adobe.com/prodindex/acrobat/alternate.html"));

		m_pHost = NULL;
	}

	// return FALSE if SetFocus is called
	return TRUE;
}

void CDocPage::OnDestroy(
	void
    )
{
	delete m_pHost;
}

void CDocPage::OnActivate( 
    UINT state, 
    HWND hwndActDeact, 
    BOOL fMinimized 
    )
{
    if ( state == WA_ACTIVE && !fMinimized && m_pHost )
    {
        HWND hwnd = m_pHost->GetControlHWND();
        if ( hwnd )
        {
            ::InvalidateRect( hwnd, NULL, TRUE );
            ::UpdateWindow( hwnd );
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
// ShellBrowser.cpp: implementation of the CShellBrowser class.
//

#include "pch.hxx"
#include "BrowseForFolder.hxx"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CBrowseForFolder::CBrowseForFolder(const HWND hParent, const LPITEMIDLIST pidl, 
                                   LPCTSTR strTitle)
{
	m_hwnd = NULL;
	SetOwner(hParent);
	SetRoot(pidl);
	m_bi.lpfn = BrowseCallbackProc;
	m_bi.lParam = reinterpret_cast<LPARAM>( this );
	m_bi.pszDisplayName = m_szSelected;
    m_bi.lpszTitle = strTitle;
}

CBrowseForFolder::~CBrowseForFolder()
{
}

void CBrowseForFolder::SetOwner(const HWND hwndOwner)
{
	if (m_hwnd != NULL)
		return;

	m_bi.hwndOwner = hwndOwner;
}

void CBrowseForFolder::SetRoot(const LPITEMIDLIST pidl)
{
	if (m_hwnd != NULL)
		return;

	m_bi.pidlRoot = pidl;
}

LPCTSTR CBrowseForFolder::GetTitle() const
{
	return m_bi.lpszTitle;
}

void CBrowseForFolder::SetFlags(const UINT ulFlags)
{
	if (m_hwnd != NULL)
		return;

	m_bi.ulFlags = ulFlags;
}

LPCTSTR CBrowseForFolder::GetSelectedFolder(
    void
    ) const
{
	return m_szSelected;
}

bool CBrowseForFolder::SelectFolder()
{
	bool bRet = false;

	LPITEMIDLIST pidl;
	if ((pidl = ::SHBrowseForFolder(&m_bi)) != NULL)
	{
		m_strPath.Set( _T("") );
		if (SUCCEEDED(::SHGetPathFromIDList(pidl, m_szSelected)))
		{
			bRet = true;
			m_strPath.Set( m_szSelected );
		}

		LPMALLOC pMalloc;
		//Retrieve a pointer to the shell's IMalloc interface
		if (SUCCEEDED(SHGetMalloc(&pMalloc)))
		{
			// free the PIDL that SHBrowseForFolder returned to us.
			pMalloc->Free(pidl);
			// release the shell's IMalloc interface
			(void)pMalloc->Release();
		}
	}
	m_hwnd = NULL;

	return bRet;
}

void CBrowseForFolder::OnInit() const
{

}

void CBrowseForFolder::OnSelChanged(const LPITEMIDLIST pidl) const
{
	(void)pidl;
}

void CBrowseForFolder::EnableOK(const bool bEnable) const
{
	if (m_hwnd == NULL)
		return;

	// (void)SendMessage(m_hwnd, BFFM_ENABLEOK, static_cast(bEnable), NULL);
    (void)SendMessage( m_hwnd, BFFM_ENABLEOK, NULL, static_cast<LPARAM>(bEnable) );
}

void CBrowseForFolder::SetSelection(const LPITEMIDLIST pidl) const
{
	if (m_hwnd == NULL)
		return;

	(void)SendMessage(m_hwnd, BFFM_SETSELECTION, FALSE, reinterpret_cast<LPARAM>(pidl));
}

void CBrowseForFolder::SetSelection(
    LPCTSTR strPath
    ) const
{
	if (m_hwnd == NULL)
		return;

	(void)SendMessage(m_hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(strPath));
}

void CBrowseForFolder::SetStatusText(
    LPCTSTR strText
    ) const
{
	if (m_hwnd == NULL)
		return;

    (void)SendMessage(m_hwnd, BFFM_SETSTATUSTEXT, NULL, reinterpret_cast<LPARAM>(strText));
}

int __stdcall CBrowseForFolder::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	CBrowseForFolder* pbff = reinterpret_cast<CBrowseForFolder*>( lpData );
	pbff->m_hwnd = hwnd;
	if (uMsg == BFFM_INITIALIZED)
		pbff->OnInit();
	else if (uMsg == BFFM_SELCHANGED)
		pbff->OnSelChanged( reinterpret_cast<LPITEMIDLIST>( lParam ));
	
	return 0;
}
//////////////////////////////////////////////////////////////////////
//
// ShellBrowser.cpp: implementation of the CShellBrowser class.
//

#include "pch.hxx"
#include "BrowseForFolder.hxx"

//////////////////////////////////////////////////////////////////////
//
// Construction/Destruction
//

CBrowseForFolder::CBrowseForFolder(
    const HWND hParent, 
    const LPITEMIDLIST pidl, 
    LPCTSTR strTitle)
{
	m_hwnd = NULL;

	myBrowseInfo.pidlRoot = pidl;
  myBrowseInfo.hwndOwner = NULL;
	myBrowseInfo.pszDisplayName = mySelected;
  myBrowseInfo.lpszTitle = "Open ROM Folder ";
  myBrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS;
	myBrowseInfo.lParam = reinterpret_cast<LPARAM>( this );

//	myBrowseInfo.lpfn = BrowseCallbackProc;
}

CBrowseForFolder::~CBrowseForFolder()
{
}

//////////////////////////////////////////////////////////////////////
//
// Implementation
//

void CBrowseForFolder::SetOwner(const HWND hwndOwner)
{
	if (m_hwnd != NULL)
		return;

	myBrowseInfo.hwndOwner = hwndOwner;
}

void CBrowseForFolder::SetRoot(const LPITEMIDLIST pidl)
{
	if (m_hwnd != NULL)
		return;

	myBrowseInfo.pidlRoot = pidl;
}

LPCTSTR CBrowseForFolder::GetTitle() const
{
	return myBrowseInfo.lpszTitle;
}

bool CBrowseForFolder::SetTitle(
    LPCTSTR strTitle
    )
{
	if (m_hwnd != NULL)
		return false;

    if ( strTitle == NULL )
    {
        return false;
    }

	if ( ! m_pchTitle.Set( strTitle ) )
    {
        return false;
    }

	myBrowseInfo.lpszTitle = m_pchTitle.Get();

    return true;
}

void CBrowseForFolder::SetFlags(const UINT ulFlags)
{
	if (m_hwnd != NULL)
		return;

	myBrowseInfo.ulFlags = ulFlags;
}

LPCTSTR CBrowseForFolder::GetSelectedFolder(
    void
    ) const
{
	return mySelected;
}

bool CBrowseForFolder::SelectFolder()
{
	bool bRet = false;

	LPITEMIDLIST pidl;
	if ((pidl = ::SHBrowseForFolder(&myBrowseInfo)) != NULL)
	{
		myPath.Set( _T("") );
		if (SUCCEEDED(::SHGetPathFromIDList(pidl, mySelected)))
		{
			bRet = true;
			myPath.Set( mySelected );
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


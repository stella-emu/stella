//
// StellaX
// Jeff Miller 05/01/2000
//
#include "pch.hxx"
#include "FileDialog.hxx"

CFileDialog::CFileDialog(
	BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
	LPCTSTR lpszDefExt /* = NULL */,
	LPCTSTR lpszFileName /* = NULL */,
	DWORD dwFlags /* = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT */,
	LPCTSTR lpszFilter /* = NULL */,
	HWND hwndParent /* = NULL */
    )
{
    UNUSED_ALWAYS( hwndParent );

	memset(&m_ofn, 0, sizeof(m_ofn)); // initialize structure to 0/NULL
	m_szFileName[0] = '\0';
	m_szFileTitle[0] = '\0';

	m_bOpenFileDialog = bOpenFileDialog;

	m_ofn.lStructSize = sizeof(m_ofn);
	m_ofn.lpstrFile = m_szFileName;
	m_ofn.nMaxFile = _countof(m_szFileName);
	m_ofn.lpstrDefExt = lpszDefExt;
	m_ofn.lpstrFileTitle = (LPTSTR)m_szFileTitle;
	m_ofn.nMaxFileTitle = _countof(m_szFileTitle);
	m_ofn.Flags = (dwFlags | OFN_EXPLORER);
	m_ofn.hInstance = NULL;

	// setup initial file name
	if (lpszFileName != NULL)
		lstrcpyn(m_szFileName, lpszFileName, _countof(m_szFileName));

	// Translate filter into commdlg format (lots of \0)
	if (lpszFilter != NULL)
	{
		lstrcpy(m_szFilter, lpszFilter);
		LPTSTR pch = m_szFilter; // modify the buffer in place
		// MFC delimits with '|' not '\0'
		while ((pch = _tcschr(pch, '|')) != NULL)
			*pch++ = '\0';
		m_ofn.lpstrFilter = m_szFilter;
		// do not call ReleaseBuffer() since the string contains '\0' characters
	}
}

int CFileDialog::DoModal(
	void
    )
{
	int nResult;

	if (m_bOpenFileDialog)
    {
		nResult = ::GetOpenFileName(&m_ofn);
    }
	else
    {
		nResult = ::GetSaveFileName(&m_ofn);
    }

	return nResult ? nResult : IDCANCEL;
}



//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef FILEDLG_H
#define FILEDLG_H
#pragma once

class CFileDialog
{
public:

	OPENFILENAME m_ofn;

	CFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		HWND hwndParent = NULL);

	virtual int DoModal();

	LPCTSTR GetPathName() const
        {
            return m_ofn.lpstrFile;
        }

	LPCTSTR GetFileName() const
        {
            return m_ofn.lpstrFileTitle;
        }

protected:

	BOOL m_bOpenFileDialog;       // TRUE for file open, FALSE for file save

	TCHAR m_szFilter[1024];          // filter string
						// separate fields with '|', terminate with '||\0'

	TCHAR m_szFileTitle[64];       // contains file title after return
	TCHAR m_szFileName[_MAX_PATH]; // contains full path name after return

private:

	CFileDialog( const CFileDialog& );  // no implementation
	void operator=( const CFileDialog& );  // no implementation

};

#endif
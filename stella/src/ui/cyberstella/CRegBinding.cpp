/*
 * CREGBINDING.CPP
 *
 * Registry Class
 *
 * Copyright (C) 1998 by Joerg Dentler (dentler@tpnet.de)
 * All rights reserved
 *
 */
 //
// Version 1.0.0        98-11-20 created

/////////////////////////////////////////////////////////////////////////////
// Permission is granted to anyone to use this software for any
// purpose and to redistribute it in any way, subject to the following 
// restrictions:
//
// 1. The author is not responsible for the consequences of use of
//    this software, no matter how awful, even if they arise
//    from defects in it.
//
// 2. The origin of this software must not be misrepresented, either
//    by explicit claim or by omission.
//
// 3. Altered versions must be plainly marked as such, and must not
//    be misrepresented (by explicit claim or omission) as being
//    the original software.
// 
// 4. This notice must not be removed or altered.
/////////////////////////////////////////////////////////////////////////////

#include <afxwin.h>
#include "cregbinding.h"

// CEntry class 
class CEntry : public CObject
{
 public: 
  enum Type 
  {
    CR_INVALID,
    CR_INT,
    CR_DWORD,
    CR_STRING,
    CR_BYTE_ARR,
    CR_CWND
  };
  Type     m_eType;
  CString  m_strName;
  LPVOID   m_pData;

  CEntry() : 
    m_eType(CR_INVALID), m_pData(NULL) {  }
  CEntry(const CEntry &e) : 
    m_eType(e.m_eType), m_pData(e.m_pData), m_strName(e.m_strName) {  }
};

// CRegBinding class members
BOOL CRegBinding::Hook(const HWND hwnd)
{
  CEntry *e;
  if (FindAndRemove(e, hwnd)) {
    SaveWindowState(*e);
    delete e;
    return TRUE;
  }
  return FALSE;
}

BOOL CRegBinding::FindAndRemove(CEntry *&e, const HWND hwnd)
{
  POSITION pos = m_list.GetHeadPosition(), spos;
  while (pos != NULL) {
    spos = pos;
    CObject *o = m_list.GetNext(pos);
    ASSERT_VALID(o);
    e = (CEntry *)o;
    if (e->m_eType == CEntry::CR_CWND) {
      CWnd *wnd = (CWnd *)e->m_pData;
      if (wnd->GetSafeHwnd() == hwnd) {
        m_list.RemoveAt(spos);
        return TRUE;
      }
    }
  }
  return FALSE;
}

void CRegBinding::Bind(int &value, const char *name, const int def)
{
  value = m_pApp->GetProfileInt(m_strSection, name, def);
  CEntry e;
  e.m_eType = CEntry::CR_INT;
  e.m_pData = &value;
  e.m_strName = name;
  Insert(e);
}

void CRegBinding::Bind(DWORD &value, const char *name, const DWORD def)
{
  value = m_pApp->GetProfileInt(m_strSection, name, def);
  CEntry e;
  e.m_eType = CEntry::CR_DWORD;
  e.m_pData = &value;
  e.m_strName = name;
  Insert(e);
}

void CRegBinding::Bind(CString &value, const char *name, const char *def)
{
  const char *ds = def ? def : "";
  value = m_pApp->GetProfileString(m_strSection, name, ds);
  CEntry e;
  e.m_eType = CEntry::CR_STRING;
  e.m_pData = &value;
  e.m_strName = name;
  Insert(e);
}

void CRegBinding::Bind(CByteArray &value, const char *name,
  const CByteArray *def)
{ 
  LPBYTE  data = NULL;
  UINT    bytes;
  m_pApp->GetProfileBinary(m_strSection, name, &data, &bytes);
  if (bytes > 0) {
    // registry contains some data
    value.SetAtGrow(bytes - 1, 0);
    BYTE *p = value.GetData();
    CopyMemory(p, data, bytes);
    delete data;
  } else {
    // assume default value
    if (def) {
      ASSERT_VALID(def);
      value.Copy(*def);
    }
  }
  CEntry e;
  e.m_eType = CEntry::CR_BYTE_ARR;
  e.m_pData = &value;
  e.m_strName = name; 
  Insert(e);
}

void CRegBinding::Bind(CWnd *wnd, const char *name)
{
  ASSERT_VALID(wnd);
  ASSERT(wnd->GetSafeHwnd() != NULL);
  CString n;
  if (name)
    n = name;
  else {
    wnd->GetWindowText(n);
    if (n.IsEmpty())
      n = "Window";
  }
  n += '_';

  WINDOWPLACEMENT wp;
  wp.length = sizeof (WINDOWPLACEMENT);
  wnd->GetWindowPlacement(&wp);

  if (((wp.flags =   
           m_pApp->GetProfileInt (m_strSection, n + "flags", -1)) != -1) &&
      ((wp.showCmd = 
           m_pApp->GetProfileInt (m_strSection, n + "showCmd", -1)) != -1) &&
      ((wp.rcNormalPosition.left = 
           m_pApp->GetProfileInt (m_strSection, n + "x1", -1)) != -1) &&
      ((wp.rcNormalPosition.top =  
           m_pApp->GetProfileInt (m_strSection, n + "y1", -1)) != -1) &&
      ((wp.rcNormalPosition.right = 
           m_pApp->GetProfileInt (m_strSection, n + "x2", -1)) != -1) &&
      ((wp.rcNormalPosition.bottom =
           m_pApp->GetProfileInt (m_strSection, n + "y2", -1)) != -1)) {

        wp.rcNormalPosition.left = min (wp.rcNormalPosition.left,
            ::GetSystemMetrics (SM_CXSCREEN) -
            ::GetSystemMetrics (SM_CXICON));
        wp.rcNormalPosition.top = min (wp.rcNormalPosition.top,
            ::GetSystemMetrics (SM_CYSCREEN) -
            ::GetSystemMetrics (SM_CYICON));
        wnd->SetWindowPlacement (&wp);
  }
  CEntry e;
  e.m_eType = CEntry::CR_CWND;
  e.m_pData = wnd;
  e.m_strName = n;
  Insert(e);
}

CWinApp *CRegBinding::m_pApp;
HHOOK    CRegBinding::m_hHook;

CRegBinding::CRegBinding(const char *section) : m_strSection(section)
{
  m_pApp = AfxGetApp();
  ASSERT_VALID(m_pApp);
  if (m_lInstances.IsEmpty()) {
    m_hHook = ::SetWindowsHookEx(WH_CBT, FilterHook, NULL, ::GetCurrentThreadId());
    if (m_hHook == NULL)
      AfxThrowMemoryException();
  }
  Bind(this);
}

void CRegBinding::Insert(const CEntry &e)
{
  // in autosave Mode the destructor of the CRegBinding Member Object 
  // must be called before any other member destructor is executed
#ifdef _DEBUG  
  LPVOID tp = (const LPVOID)this;
  LPVOID mp = (const LPVOID)(e.m_pData);
  if (!(mp < tp)) {
    TRACE("Please put the CRegBinding Object behind"
          "the last member to bind\n"
          "Only class members can registerd by CRegBinding\n");
    ASSERT(0);
  }
  POSITION pos = m_list.GetHeadPosition();
  while (pos != NULL) {
    CObject *o = m_list.GetNext(pos);
    ASSERT_VALID(o);
    CEntry *le = (CEntry *)o;
    if (le->m_strName == e.m_strName) {
      TRACE("The same key already exists in this section\n");
      ASSERT(0);
    }
  }
#endif
  CEntry *ne = new CEntry(e);
  ASSERT_VALID(ne);
  m_list.AddTail(ne);
}

void CRegBinding::SaveWindowState(const CEntry &e)
{
  ASSERT(e.m_eType == CEntry::CR_CWND);
  CWnd *wnd = (CWnd *)e.m_pData;
  ASSERT_VALID(wnd);
  CString n = e.m_strName;
  WINDOWPLACEMENT wp;
  wp.length = sizeof (WINDOWPLACEMENT);
  wnd->GetWindowPlacement(&wp);

  m_pApp->WriteProfileInt(m_strSection, n + "flags", wp.flags);
  m_pApp->WriteProfileInt(m_strSection, n + "showCmd", wp.showCmd);
  m_pApp->WriteProfileInt(m_strSection, n + "x1", wp.rcNormalPosition.left);
  m_pApp->WriteProfileInt(m_strSection, n + "y1", wp.rcNormalPosition.top);
  m_pApp->WriteProfileInt(m_strSection, n + "x2", wp.rcNormalPosition.right);
  m_pApp->WriteProfileInt(m_strSection, n + "y2", wp.rcNormalPosition.bottom);
}

CRegBinding::~CRegBinding()
{
  Write(TRUE);
  UnBind(this);
  if (m_lInstances.IsEmpty()) {
    UnhookWindowsHookEx(m_hHook);
  }
}

void CRegBinding::Write(BOOL del)
{
  POSITION pos1 = m_list.GetHeadPosition(), pos2;
  while (pos1 != NULL) {
    pos2 = pos1;
    CEntry *o = (CEntry *)m_list.GetNext(pos1);
    ASSERT_VALID(o);
    Write(o);
    if (del) {
      m_list.RemoveAt(pos2);
      delete o;
    }
  }
}

void CRegBinding::Write(const CEntry *e)
{
  ASSERT_VALID(e);
  switch (e->m_eType) {
    case CEntry::CR_INT:
      { 
        int *i = (int *)(e->m_pData);
        m_pApp->WriteProfileInt(m_strSection, e->m_strName, *i);
        return;
      }
    case CEntry::CR_DWORD:
      {
        DWORD *dw = (DWORD *)(e->m_pData);
        m_pApp->WriteProfileInt(m_strSection, e->m_strName, *dw);
        return;
      }
    case CEntry::CR_STRING:
      {
        CString *str = (CString *)(e->m_pData);
        m_pApp->WriteProfileString(m_strSection, e->m_strName, *str);
        return;
      }
    case CEntry::CR_BYTE_ARR:
      {
        CByteArray *p = (CByteArray *)(e->m_pData);
        ASSERT_VALID(p);
        m_pApp->WriteProfileBinary(m_strSection, e->m_strName, p->GetData(), 
          p->GetSize());
        return;
      }
    case CEntry::CR_CWND:
      SaveWindowState(*e);
      return;
  }
  ASSERT(0);
}

CObList CRegBinding::m_lInstances;

LRESULT CALLBACK 
CRegBinding::FilterHook(int code, WPARAM wParam, LPARAM lParam)
{
  if (code == HCBT_DESTROYWND) {
    ASSERT(wParam != NULL); // should be non-NULL HWND
    POSITION pos = m_lInstances.GetHeadPosition();
    while (pos != NULL) {
      CObject *o = m_lInstances.GetNext(pos);
      ASSERT_VALID(o);
      CRegBinding *rs = (CRegBinding *)o;
      if (rs->Hook((HWND)wParam))
        break;
    }
  }
  return CallNextHookEx(m_hHook, code, wParam, lParam);
}

void CRegBinding::Bind(CRegBinding *rs)
{
  m_lInstances.AddTail(rs);
}

void CRegBinding::UnBind(CRegBinding *rs)
{
  ASSERT_VALID(rs);
  POSITION f = m_lInstances.Find(rs);
  ASSERT(f != NULL);
  m_lInstances.RemoveAt(f);
}

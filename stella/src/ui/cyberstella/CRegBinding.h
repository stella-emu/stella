/*
 * CREGBINDING.H
 *
 * Registry Class
 *
 * Copyright (C) 1998 by Joerg Dentler (dentler@tpnet.de)
 * All rights reserved
 *
 */
 //
// Version 1.0.0        98-11-20 created

#ifndef __CREGBINDING_H__
#define __CREGBINDING_H__

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

#include <afx.h>

class CEntry;

// An alternative Registry Class
class CRegBinding : public CObject
{
 public:
  CRegBinding(const char *section);

  // Bind frequently used data types
  void Bind(int        &value, const char *name, const int def = 0);
  void Bind(DWORD      &value, const char *name, const DWORD def = 0);
  void Bind(CString    &value, const char *name, const char *def = NULL);
  void Bind(CByteArray &value, const char *name, const CByteArray *def = NULL);
  // Save the window's screen state
  void Bind(CWnd       *value, const char *name = NULL);
  // Write Data - optional clear variable bindings
  void Write(const BOOL clear = FALSE);

 ~CRegBinding();
 protected:
  // Data
  CObList  m_list;
  CString  m_strSection;
 protected:
  void SaveWindowState(const CEntry &e);
  void Write(const CEntry *e);
  void Insert(const CEntry &e);
  BOOL Hook(const HWND hwnd);
  BOOL FindAndRemove(CEntry *&e, const HWND hwnd);

  static CWinApp *m_pApp;
  static HHOOK    m_hHook;
  // reference counting
  static CObList  m_lInstances;

  static LRESULT CALLBACK FilterHook(int code, WPARAM wParam, LPARAM lParam);
  static void Bind(CRegBinding *rs);
  static void UnBind(CRegBinding *rs);
};

#endif


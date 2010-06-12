//============================================================================
//
//  BBBBB    SSSS   PPPPP   FFFFFF
//  BB  BB  SS  SS  PP  PP  FF
//  BB  BB  SS      PP  PP  FF
//  BBBBB    SSSS   PPPPP   FFFF    --  "Brad's Simple Portability Framework"
//  BB  BB      SS  PP      FF
//  BB  BB  SS  SS  PP      FF
//  BBBBB    SSSS   PP      FF
//
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef BSPF_HXX
#define BSPF_HXX

/**
  This file defines various basic data types and preprocessor variables
  that need to be defined for different operating systems.

  @author Bradford W. Mott
  @version $Id$
*/

#ifdef HAVE_INTTYPES
  #include <inttypes.h>

  // Types for 8-bit signed and unsigned integers
  typedef int8_t Int8;
  typedef uint8_t uInt8;
  // Types for 16-bit signed and unsigned integers
  typedef int16_t Int16;
  typedef uint16_t uInt16;
  // Types for 32-bit signed and unsigned integers
  typedef int32_t Int32;
  typedef uint32_t uInt32;
  // Types for 64-bit signed and unsigned integers
  typedef int64_t Int64;
  typedef uint64_t uInt64;
#elif defined BSPF_WIN32
  // Types for 8-bit signed and unsigned integers
  typedef signed char Int8;
  typedef unsigned char uInt8;
  // Types for 16-bit signed and unsigned integers
  typedef signed short Int16;
  typedef unsigned short uInt16;
  // Types for 32-bit signed and unsigned integers
  typedef signed int Int32;
  typedef unsigned int uInt32;
  // Types for 64-bit signed and unsigned integers
  typedef __int64 Int64;
  typedef unsigned __int64 uInt64;
#else
  #error Update BSPF.hxx for datatypes
#endif


// The following code should provide access to the standard C++ objects and
// types: cout, cerr, string, ostream, istream, etc.
#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
using namespace std;

// Defines to help with path handling
#if defined BSPF_UNIX
  #define BSPF_PATH_SEPARATOR  "/"
#elif (defined(BSPF_DOS) || defined(BSPF_WIN32) || defined(BSPF_OS2))
  #define BSPF_PATH_SEPARATOR  "\\"
#elif defined BSPF_MAC_OSX
  #define BSPF_PATH_SEPARATOR  "/"
#elif defined BSPF_GP2X
  #define BSPF_PATH_SEPARATOR  "/"
#endif

// I wish Windows had a complete POSIX layer
#if defined BSPF_WIN32 && !defined __GNUG__
  #define BSPF_strcasecmp stricmp
  #define BSPF_strncasecmp strnicmp
  #define BSPF_isblank(c) ((c == ' ') || (c == '\t'))
  #define BSPF_snprintf _snprintf
  #define BSPF_vsnprintf _vsnprintf
#else
  #include <strings.h>
  #define BSPF_strcasecmp strcasecmp
  #define BSPF_strncasecmp strncasecmp
  #define BSPF_isblank(c) isblank(c)
  #define BSPF_snprintf snprintf
  #define BSPF_vsnprintf vsnprintf
#endif

// CPU architecture type
// This isn't complete yet, but takes care of all the major platforms
#if defined(__i386__) || defined(_M_IX86)
  #define BSPF_ARCH "i386"
#elif defined(__x86_64__) || defined(_WIN64)
  #define BSPF_ARCH "x86_64"
#elif defined(__powerpc__) || defined(__ppc__)
  #define BSPF_ARCH "ppc"
#else
  #define BSPF_ARCH "NOARCH"
#endif

// Some convenience functions
template<typename T> inline void BSPF_swap(T& a, T& b) { T tmp = a; a = b; b = tmp; }
template<typename T> inline T BSPF_abs (T x) { return (x>=0) ? x : -x; }
template<typename T> inline T BSPF_min (T a, T b) { return (a<b) ? a : b; }
template<typename T> inline T BSPF_max (T a, T b) { return (a>b) ? a : b; }
inline bool BSPF_equalsIgnoreCase(const string& s1, const string& s2)
{
  return BSPF_strcasecmp(s1.c_str(), s2.c_str()) == 0;
}
inline bool BSPF_equalsIgnoreCase(const char* s1, const char* s2)
{
  return BSPF_strcasecmp(s1, s2) == 0;
}

static const string EmptyString("");

#ifdef _WIN32_WCE
  #include "missing.h"
#endif

#endif

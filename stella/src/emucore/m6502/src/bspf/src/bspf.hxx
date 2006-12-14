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
// Copyright (c) 1997-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: bspf.hxx,v 1.14 2006-12-14 00:07:47 stephena Exp $
//============================================================================

#ifndef BSPF_HXX
#define BSPF_HXX

/**
  This file defines various basic data types and preprocessor variables
  that need to be defined for different operating systems.

  @author Bradford W. Mott
  @version $Id: bspf.hxx,v 1.14 2006-12-14 00:07:47 stephena Exp $
*/

// Types for 8-bit signed and unsigned integers
typedef signed char Int8;
typedef unsigned char uInt8;

// Types for 16-bit signed and unsigned integers
typedef signed short Int16;
typedef unsigned short uInt16;

// Types for 32-bit signed and unsigned integers
typedef signed int Int32;
typedef unsigned int uInt32;

// The following code should provide access to the standard C++ objects and
// types: cout, cerr, string, ostream, istream, etc.
#ifdef BSPF_OLD_STYLE_CXX_HEADERS
  #include <iostream.h>
  #include <iomanip.h>
  #include <string>
#else
  #include <iostream>
  #include <iomanip>
  #include <string>
  using namespace std;
#endif

#ifdef PSP
  #include "pspstdint.h"
#endif
	
#ifdef HAVE_INTTYPES
  #include <inttypes.h>
#endif

// Defines to help with path handling
#if defined BSPF_UNIX
  #define BSPF_PATH_SEPARATOR  "/"
#elif (defined(BSPF_DOS) || defined(BSPF_WIN32) || defined(BSPF_OS2))
  #define BSPF_PATH_SEPARATOR  "\\"
#elif defined BSPF_MAC_OSX
  #define BSPF_PATH_SEPARATOR  "/"
#elif defined BSPF_PSP
    #define BSPF_PATH_SEPARATOR  "/"
#elif defined BSPF_GP2X
    #define BSPF_PATH_SEPARATOR  "/"
#endif

// I wish Windows had a complete POSIX layer
#ifdef BSPF_WIN32
  #define STR_CASE_CMP stricmp
  #define STR_N_CASE_CMP strnicmp
  #define IS_BLANK(c) ((c == ' ') || (c == '\t'))
  #define SNPRINTF _snprintf
  #define VSNPRINTF _vsnprintf
#else
  #define STR_CASE_CMP strcasecmp
  #define STR_N_CASE_CMP strncasecmp
  #define IS_BLANK(c) isblank(c)
  #define SNPRINTF snprintf
  #define VSNPRINTF vsnprintf
#endif


#ifdef _WIN32_WCE
  #include "missing.h"
#endif

#endif

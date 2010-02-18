/**
	*  Part of the foobar2000 Matroska plugin
	*
	*  Copyright (C) Jory Stone - 2003
	*
  *  This library is free software; you can redistribute it and/or
  *  modify it under the terms of the GNU Library General Public
  *  License as published by the Free Software Foundation; either
  *  version 2 of the License, or (at your option) any later version.
  *
  *  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
  *  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
	*
*/

#ifndef _DBG_OUT_H_
#define _DBG_OUT_H_

#ifndef STRICT
#define STRICT
#endif
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#ifdef _DEBUG
//#define DEBUG_OUT_PUT
#include <tchar.h>
#include <stdarg.h>
#include <fstream>
#include <string>
#include <locale>
namespace win32
{
    typedef std::basic_string< _TCHAR > string;
    typedef std::basic_fstream< _TCHAR > fstream;
}
#define HPRINTF_MES_LENGTH 256
inline void hprintf( const _TCHAR * format, ... )
{
    _TCHAR buff[HPRINTF_MES_LENGTH];
    va_list vl;
    va_start(vl, format);
    _vsntprintf(buff, HPRINTF_MES_LENGTH, format, vl);
    va_end(vl);
    OutputDebugString(buff);
    //*
    win32::string basicstring(buff);
    win32::fstream f("foo_input_matroska.debug.txt", std::ios::out|std::ios::app);
    f << basicstring.c_str();
	f.close();
    //*/
}
#include <boost/timer.hpp>
#define TIMER boost::timer t
#define RETIMER t.restart()
#define _TIMER(x) hprintf(L"%S: %f sec.\n",x,t.elapsed())
#else
#define TIMER __noop
#define RETIMER __noop
#define _TIMER(x) __noop
#define hprintf __noop
#endif

#ifdef DEBUG_OUT_PUT
void WINAPI DbgStringOut(const TCHAR *pFormat,...);
#ifdef UNICODE
void WINAPI DbgLogInfo(const CHAR *pFormat,...)
#endif
#define NOTE(x) DbgStringOut(x)
#define NOTE1(x, y1) DbgStringOut(x, y1)
#define NOTE2(x, y1, y2) DbgStringOut(x, y1, y2)
#define NOTE3(x, y1, y2, y3) DbgStringOut(x, y1, y2, y3)
#define NOTE4(x, y1, y2, y3, y4) DbgStringOut(x, y1, y2, y3, y4)
#define NOTE5(x, y1, y2, y3, y4, y5) DbgStringOut(x, y1, y2, y3, y4, y5)
#define NOTE6(x, y1, y2, y3, y4, y5, y6) DbgStringOut(x, y1, y2, y3, y4, y5, y6)
#define NOTE10(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10) DbgStringOut(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10)
#define ODS(x) DbgStringOut(x)
#define ODS1(x, y1) DbgStringOut(x, y1)
#define ODS2(x, y1, y2) DbgStringOut(x, y1, y2)
#define ODS3(x, y1, y2, y3) DbgStringOut(x, y1, y2, y3)
#define ODS4(x, y1, y2, y3, y4) DbgStringOut(x, y1, y2, y3, y4)
#define ODS5(x, y1, y2, y3, y4, y5) DbgStringOut(x, y1, y2, y3, y4, y5)
#define ODS6(x, y1, y2, y3, y4, y5, y6) DbgStringOut(x, y1, y2, y3, y4, y5, y6)
#define ODS10(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10) DbgStringOut(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10)
#else
#define NOTE(x)
#define NOTE1(x, y1)
#define NOTE2(x, y1, y2)
#define NOTE3(x, y1, y2, y3)
#define NOTE4(x, y1, y2, y3, y4)
#define NOTE5(x, y1, y2, y3, y4, y5)
#define NOTE6(x, y1, y2, y3, y4, y5, y6)
#define NOTE10(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10)
#define ODS(x)
#define ODS1(x, y1)
#define ODS2(x, y1, y2)
#define ODS3(x, y1, y2, y3)
#define ODS4(x, y1, y2, y3, y4)
#define ODS5(x, y1, y2, y3, y4, y5)
#define ODS6(x, y1, y2, y3, y4, y5, y6)
#define ODS10(x, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10)
#endif


#endif // _DBG_OUT_H_

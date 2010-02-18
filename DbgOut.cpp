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

#include "DbgOut.h"

#ifdef DEBUG_OUT_PUT
#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>

static DWORD g_dwTimeOffset = timeGetTime();

void WINAPI DbgStringOut(const TCHAR *pFormat,...)
{    	
  TCHAR szInfo[2000];

  // Format the variable length parameter list
  va_list va;
  va_start(va, pFormat);

  lstrcpy(szInfo, TEXT("foo_input_matroska - "));
  wsprintf(szInfo + lstrlen(szInfo),
            TEXT("(tid %x) %8d : "),
            GetCurrentThreadId(), timeGetTime() - g_dwTimeOffset);

  _vstprintf(szInfo + lstrlen(szInfo), pFormat, va);
  lstrcat(szInfo, TEXT("\r\n"));
  OutputDebugString(szInfo);

  va_end(va);
};

#ifdef UNICODE
void WINAPI DbgLogInfo(const CHAR *pFormat,...)
{
    TCHAR szInfo[2000];

    // Format the variable length parameter list
    va_list va;
    va_start(va, pFormat);

    lstrcpy(szInfo, TEXT("foo_input_matroska - "));
    wsprintf(szInfo + lstrlen(szInfo),
             TEXT("(tid %x) %8d : "),
             GetCurrentThreadId(), timeGetTime() - g_dwTimeOffset);

    CHAR szInfoA[2000];
    WideCharToMultiByte(CP_ACP, 0, szInfo, -1, szInfoA, sizeof(szInfoA)/sizeof(CHAR), 0, 0);

    wvsprintfA(szInfoA + lstrlenA(szInfoA), pFormat, va);
    lstrcatA(szInfoA, "\r\n");

    WCHAR wszOutString[2000];
    MultiByteToWideChar(CP_ACP, 0, szInfoA, -1, wszOutString, sizeof(wszOutString)/sizeof(WCHAR));
    DbgOutString(wszOutString);

    va_end(va);
}
#endif
#endif


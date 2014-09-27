// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__577FF9BC_3760_4BE3_91F9_499E95114492__INCLUDED_)
#define AFX_STDAFX_H__577FF9BC_3760_4BE3_91F9_499E95114492__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// debug message
#ifdef _DEBUG
# define KdPrint(str)		OutputDebugString str
#else
# define KdPrint(str)
#endif


// Insert your headers here
#define WINVER       0x0500
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// TODO: reference additional headers your program requires here
#include "../../library/lpi.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__577FF9BC_3760_4BE3_91F9_499E95114492__INCLUDED_)

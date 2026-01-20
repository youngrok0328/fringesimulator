#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.

// MFC의 공통 부분과 무시 가능한 경고 메시지에 대한 숨기기를 해제합니다.
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 핵심 및 표준 구성 요소입니다.
#include <afxext.h>         // MFC 확장입니다.

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // Internet Explorer 4 공용 컨트롤에 대한 MFC 지원입니다.
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // Windows 공용 컨트롤에 대한 MFC 지원입니다.
#endif // _AFX_NO_AFXCMN_SUPPORT

//#include <afxcontrolbars.h>     // MFC의 리본 및 컨트롤 막대 지원

#include "ipvmbasedef.h"

#define _XTP_STATICLINK
#define _XTP_EXCLUDE_MARKUP         
//#define _XTP_EXCLUDE_TABMANAGER     
#define _XTP_EXCLUDE_CONTROLS       
#define _XTP_EXCLUDE_CALENDAR       
#define _XTP_EXCLUDE_CHART          
//#define _XTP_EXCLUDE_COMMANDBARS    
//#define _XTP_EXCLUDE_DOCKINGPANE    
#define _XTP_EXCLUDE_FLOWGRAPH      
#define _XTP_EXCLUDE_GRAPHICLIBRARY 
//#define _XTP_EXCLUDE_PROPERTYGRID   
#define _XTP_EXCLUDE_REPORTCONTROL  
#define _XTP_EXCLUDE_RIBBON         
#define _XTP_EXCLUDE_SHORTCUTBAR    
#define _XTP_EXCLUDE_SKINFRAMEWORK  
#define _XTP_EXCLUDE_SYNTAXEDIT     
#define _XTP_EXCLUDE_TASKPANEL      
#include "XTToolkitPro.h"

enum
{
	UM_LOAD_IMAGE = WM_USER + 1,
	UM_SAVE_IMAGE,
	UM_GENERATE_FRINGE,
	UM_PROCESS_FRINGE,
	UM_VIEW_IMAGE,
	UM_VIEW_PROFILE,
	UM_AUTOGENERATE_FRINGE,
};

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

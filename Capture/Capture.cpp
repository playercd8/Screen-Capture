// Capture.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Capture.h"
#include "CaptureDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCaptureApp

BEGIN_MESSAGE_MAP(CCaptureApp, CWinApp)
	//{{AFX_MSG_MAP(CCaptureApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCaptureApp construction

CCaptureApp::CCaptureApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCaptureApp object

CCaptureApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCaptureApp initialization

BOOL CCaptureApp::InitInstance()
{
	// 假如應用程式資訊清單指定使用 ComCtl32.dll 6.0 版或更新版本
	// 以啟用視覺化樣式，則 Windows XP 需要 InitCommonControls()。否則的話，
	// 任何視窗的建立將失敗。
	InitCommonControls();

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	//變更視窗的ClassName
	{
		WNDCLASS wc;

		// Get the info for this class.
		// #32770 is the default class name for dialogs boxes.
		::GetClassInfo(AfxGetInstanceHandle(), "#32770", &wc);

		// Change the name of the class.
		wc.lpszClassName = "Capture_of_Player";

		// Register this class so that MFC can use it.
		AfxRegisterClass(&wc);
	}

	CCaptureDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
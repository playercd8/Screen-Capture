// CaptureDlg.h : header file
//

#if !defined(AFX_CAPTUREDLG_H__97B51708_C928_11D5_B7D6_0080C82BE86B__INCLUDED_)
#define AFX_CAPTUREDLG_H__97B51708_C928_11D5_B7D6_0080C82BE86B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TonyJpegLib/DIB.h"

#include "AviFile/AviFile.h"
//#include "AviFile/DynamicDC.h"

/////////////////////////////////////////////////////////////////////////////
// CCaptureDlg dialog

class CCaptureDlg : public CDialog
{
	// Construction
public:
	BOOL bTray;
	BOOL bRegistered;
	BOOL RegisterHotkey();
	UCHAR cKey;
	UCHAR cMask;
	void DeleteIcon();
	void AddIcon();
	UINT nCount;

	CCaptureDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCaptureDlg)
	enum { IDD = IDD_CAPTURE_DIALOG };
	CComboBox   m_SaveFileFormat;
	CSliderCtrl m_Quality;
	CComboBox	m_Key;
	BOOL	m_bControl;
	BOOL	m_bAlt;
	BOOL	m_bShift;
	CString	m_Path;
	CString	m_Number;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCaptureDlg)
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CCaptureDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnCancel();
	afx_msg void OnAbout();
	afx_msg void OnBrowse();
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	BOOL SelectDirectoryDialog(LPSTR lpszDirectoryName, LPSTR lpszDefaultDirectory, HWND hWnd = NULL);
	static int CALLBACK BrowseCallback(HWND hWnd, UINT uiMessage, LPARAM lParam, LPARAM lpData);

	afx_msg void OnBnClickedExplore();
	afx_msg void OnDevModeChange(LPTSTR lpDeviceName);
	afx_msg void OnCbnSelchangeSavefileformat();

	static int m_filecount;

	CDib	m_dib;
	CAviFile* m_pAvi;

	//抓圖
	void CaptureBmp(void);
	//單張格式存檔
	void SaveBmpFile(void);
	void SaveJpgFile(void);
	void SaveJppFile(void);
	void SaveJp2File(void);
	//動畫格式存檔
	void SaveAviFile(void);
	void SaveWmvFile(void);
	void SaveMovFile(void);

	UINT m_WM_Message;
	UINT_PTR m_nTimerID;

	afx_msg void OnNMCustomdrawQuality(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL DestroyWindow();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAPTUREDLG_H__97B51708_C928_11D5_B7D6_0080C82BE86B__INCLUDED_)

// CaptureDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Capture.h"
#include "CaptureDlg.h"
#include <windowsx.h>
#pragma comment(lib,"hook.lib")

int CCaptureDlg::m_filecount = 0;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDM_SHELL WM_USER+1

BOOL __declspec(dllexport)__stdcall  AddHotkey(HWND, UCHAR key, UCHAR mask);
BOOL __declspec(dllexport)__stdcall  DeleteHotkey(HWND, UCHAR key, UCHAR mask);

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
UCHAR Key_Table[] = { 0x78,0x79,0x7a,0x7b,0x6a,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39 };
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
		//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CString m_DispMode;
};

#include <ddraw.h>
#pragma comment( lib , "ddraw.lib" )
#pragma comment( lib , "dxguid.lib")

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
, m_DispMode(_T(""))
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT

	LPDIRECTDRAW		lpdd_temp = NULL;
	LPDIRECTDRAW4		lpdd = NULL;
	DDSURFACEDESC2		ddsd;
	LPDIRECTDRAWSURFACE4 lpdds_primary = NULL;
	DDPIXELFORMAT PixelFormat;
	ZeroMemory(&PixelFormat, sizeof(DDPIXELFORMAT));
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));

	if (DirectDrawCreate(NULL, &lpdd_temp, NULL) == DD_OK)
	{
		if (lpdd_temp->QueryInterface(IID_IDirectDraw4, (LPVOID*)&lpdd) == DD_OK)
		{
			memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
			ddsd.dwSize = sizeof(DDSURFACEDESC2);
			lpdd->GetDisplayMode(&ddsd);
			PixelFormat = ddsd.ddpfPixelFormat;
			lpdd->Release();
		}
		lpdd_temp->Release();
	}
	CString sTemp;
	if (PixelFormat.dwRGBBitCount >= 24)
		sTemp.Format("TrueColor %d bit Mode", PixelFormat.dwRGBBitCount);
	else if ((PixelFormat.dwRGBBitCount == 16) || (PixelFormat.dwRGBBitCount == 15))
		sTemp.Format("HiColor %d bit (%s)",
			PixelFormat.dwRGBBitCount,
			((PixelFormat.dwGBitMask == 0x07e0) ? "565 Mode" : "555 Mode"));
	else if (PixelFormat.dwRGBBitCount < 15)
		sTemp.Format("%d Color Mode",
			(2 << (PixelFormat.dwRGBBitCount - 1)));

	m_DispMode.Format("%d x %d %s",
		ddsd.dwWidth,
		ddsd.dwHeight,
		sTemp);
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_DispMode, m_DispMode);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCaptureDlg dialog

CCaptureDlg::CCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCaptureDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCaptureDlg)
	m_pAvi = NULL;
	m_WM_Message = NULL;
	m_nTimerID = NULL;

	m_bControl = FALSE;
	m_bAlt = FALSE;
	m_bShift = FALSE;

	cKey = 0;
	cMask = 0;

	nCount = 0;
	m_Number.Format(IDS_pictures, nCount);

	bRegistered = FALSE;
	bTray = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//C:\Documents and Settings\player\My Documents\My Pictures

	if (::SHGetSpecialFolderPath(NULL, m_Path.GetBuffer(MAX_PATH), CSIDL_PROFILE, FALSE) == TRUE)
	{
		m_Path.ReleaseBuffer();
		m_Path += _T("\\My Documents\\My Pictures\\");
	}
	else
	{
		m_Path = _T("c:\\");;
	}
}

#include <Shlwapi.h>

void CCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CCaptureDlg)
	DDX_Control(pDX, IDC_SaveFileFormat, m_SaveFileFormat);
	DDX_Control(pDX, IDC_Quality, m_Quality);

	DDX_Control(pDX, IDC_KEY, m_Key);
	DDX_Check(pDX, IDC_CONTROL, m_bControl);
	DDX_Check(pDX, IDC_ALT, m_bAlt);
	DDX_Check(pDX, IDC_SHIFT, m_bShift);
	DDX_Text(pDX, IDC_NUMBER, m_Number);
	//}}AFX_DATA_MAP

	{
		CString sPath = m_Path;

		HDC hdc = ::GetDC(this->m_hWnd);
		::PathCompactPath(hdc, sPath.GetBuffer(MAX_PATH), 300);
		::ReleaseDC(this->m_hWnd, hdc);
		sPath.ReleaseBuffer();

		DDX_Text(pDX, IDC_PATH, sPath);
	}
	{
		CString temp;
		if (::GetKeyState(VK_SCROLL) == NULL)
			temp = "全螢幕模式";
		else
			temp = "視窗模式";

		DDX_Text(pDX, IDC_Windows, temp);
	}

	{
		CString temp;
		int iQuality = m_Quality.GetPos();

		temp.Format("壓縮品質:%3d%%", iQuality);

		DDX_Text(pDX, IDC_QualityStr, temp);
	}
}

BEGIN_MESSAGE_MAP(CCaptureDlg, CDialog)
	//{{AFX_MSG_MAP(CCaptureDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_ABOUT, OnAbout)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(ID_CHANGE, OnChange)
	ON_BN_CLICKED(IDC_EXPLORE, OnBnClickedExplore)
	//}}AFX_MSG_MAP
	ON_WM_DEVMODECHANGE()
	ON_CBN_SELCHANGE(IDC_SaveFileFormat, OnCbnSelchangeSavefileformat)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_Quality, OnNMCustomdrawQuality)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCaptureDlg message handlers

BOOL CCaptureDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_SaveFileFormat.SetCurSel(0);
	m_Quality.SetRange(0, 100);
	m_Quality.SetPos(100);
	this->GetDlgItem(IDC_Quality)->EnableWindow(FALSE);

	m_Key.SetCurSel(0);
	RegisterHotkey();

	CMenu* pMenu = GetSystemMenu(FALSE);
	pMenu->DeleteMenu(SC_MAXIMIZE, MF_BYCOMMAND);
	pMenu->DeleteMenu(SC_SIZE, MF_BYCOMMAND);
	pMenu->DeleteMenu(SC_RESTORE, MF_BYCOMMAND);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCaptureDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCaptureDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCaptureDlg::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}

void CCaptureDlg::OnCancel()
{
	if (bTray)
		DeleteIcon();
	CDialog::OnCancel();
}

void CCaptureDlg::OnAbout()
{
	CAboutDlg dlg;
	dlg.DoModal();
}

void CCaptureDlg::OnBrowse()
{
	CString DefaultDirectory = m_Path.Left(m_Path.GetLength() - 1);
	CString SelectPath;

	SelectDirectoryDialog(SelectPath.GetBuffer(MAX_PATH), DefaultDirectory.GetBuffer(MAX_PATH));
	DefaultDirectory.ReleaseBuffer();
	SelectPath.ReleaseBuffer();

	if (SelectPath.IsEmpty())
		return;

	m_Path = SelectPath;
	if (SelectPath.GetAt(SelectPath.GetLength() - 1) != '\\')
		m_Path += "\\";

	UpdateData(FALSE);
}

int CALLBACK CCaptureDlg::BrowseCallback(HWND hWnd, UINT uiMessage, LPARAM lParam, LPARAM lpData)
{
	if (uiMessage == BFFM_INITIALIZED)
	{
		::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

BOOL CCaptureDlg::SelectDirectoryDialog(LPSTR lpszDirectoryName, LPSTR lpszDefaultDirectory, HWND hWnd /* = NULL */)
{
	LPMALLOC lpMalloc;
	if (SHGetMalloc(&lpMalloc) == NOERROR)
	{
		BROWSEINFO bi;
		if (hWnd != NULL)
		{
			bi.hwndOwner = hWnd;
		}
		else
		{
			bi.hwndOwner = m_hWnd;
		}

		bi.pidlRoot = NULL;
		bi.pszDisplayName = NULL;
		bi.lpszTitle = NULL;
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
		bi.lpfn = BrowseCallback;
		bi.lParam = (LPARAM)lpszDefaultDirectory;

		LPITEMIDLIST lpSelectDirectory = SHBrowseForFolder(&bi);
		if (lpSelectDirectory != NULL)
		{
			SHGetPathFromIDList(lpSelectDirectory, lpszDirectoryName);
			lpMalloc->Free(lpSelectDirectory);
			lpMalloc->Release();
			return TRUE;
		}
		else
		{
			if (lpSelectDirectory != NULL)
				lpMalloc->Free(lpSelectDirectory);
			lpMalloc->Release();
			return FALSE;
		}
	}
	return FALSE;
}

BOOL CCaptureDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
			return TRUE;
		if (pMsg->wParam == VK_RETURN)
			return TRUE;
	}
	return CDialog::PreTranslateMessage(pMsg);
}

LRESULT CCaptureDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if ((message == WM_HOTKEY) && (lParam == WM_KEYDOWN))
	{
		this->m_WM_Message = WM_HOTKEY;

		switch (m_SaveFileFormat.GetCurSel())
		{
		case 0:	//Bmp 24bit全彩圖檔
			CaptureBmp();
			SaveBmpFile();
			break;
		case 1:	//Jpg圖檔(可設定壓縮品質)
			CaptureBmp();
			SaveJpgFile();
			break;
		case 2:
			CaptureBmp();
			SaveJppFile();
			break;
		case 3:
			SaveAviFile();
			break;

		default:
			break;
		}

		return FALSE;
	}
	if ((message == WM_HOTKEY) && (wParam == VK_SCROLL))
	{
		UpdateData(FALSE);
	}

	if ((message == IDM_SHELL) && (lParam == WM_RBUTTONUP))
	{
		CMenu pop;
		pop.LoadMenu(MAKEINTRESOURCE(IDR_MENU1));
		CMenu* pMenu = pop.GetSubMenu(0);
		pMenu->SetDefaultItem(ID_EXITICON);
		CPoint pt;
		GetCursorPos(&pt);
		int id = pMenu->TrackPopupMenu(TPM_RIGHTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON,
			pt.x, pt.y, this);

		if (id == ID_EXITICON)
			DeleteIcon();
		else if (id == ID_EXIT)
			OnCancel();
		return FALSE;
	}

	LRESULT res = CDialog::WindowProc(message, wParam, lParam);

	if ((message == WM_SYSCOMMAND) && (wParam == SC_MINIMIZE))
		AddIcon();
	return res;
}

void CCaptureDlg::AddIcon()
{
	NOTIFYICONDATA data;
	CString tip;

	data.cbSize = sizeof(NOTIFYICONDATA);
	data.hIcon = GetIcon(0);
	data.hWnd = GetSafeHwnd();

	tip.LoadString(IDS_ICONTIP);
	strcpy_s(data.szTip, tip);
	data.uCallbackMessage = IDM_SHELL;
	data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	data.uID = 98;
	Shell_NotifyIcon(NIM_ADD, &data);
	ShowWindow(SW_HIDE);
	bTray = TRUE;
}

void CCaptureDlg::DeleteIcon()
{
	NOTIFYICONDATA data;

	data.cbSize = sizeof(NOTIFYICONDATA);
	data.hWnd = GetSafeHwnd();
	data.uID = 98;
	Shell_NotifyIcon(NIM_DELETE, &data);
	ShowWindow(SW_SHOW);
	SetForegroundWindow();
	ShowWindow(SW_SHOWNORMAL);
	bTray = FALSE;
}

void CCaptureDlg::OnChange()
{
	RegisterHotkey();
}

BOOL CCaptureDlg::RegisterHotkey()
{
	UpdateData();
	UCHAR mask = 0;
	UCHAR key = 0;

	if (m_bControl)
		mask |= 4;
	if (m_bAlt)
		mask |= 2;
	if (m_bShift)
		mask |= 1;
	key = Key_Table[m_Key.GetCurSel()];

	if (bRegistered)
	{
		DeleteHotkey(GetSafeHwnd(), cKey, cMask);
		bRegistered = FALSE;
	}

	cMask = mask;
	cKey = key;
	bRegistered = AddHotkey(GetSafeHwnd(), cKey, cMask);

	return bRegistered;
}

void CCaptureDlg::OnBnClickedExplore()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	::ShellExecute(NULL, _T("open"), m_Path, NULL, m_Path, SW_SHOWDEFAULT);
}

void CCaptureDlg::OnDevModeChange(LPTSTR lpDeviceName)
{
	CDialog::OnDevModeChange(lpDeviceName);

	// TODO: 在此加入您的訊息處理常式程式碼
}

void CCaptureDlg::OnCbnSelchangeSavefileformat()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	UpdateData(TRUE);

	int iSelFileFormat = m_SaveFileFormat.GetCurSel();
	if (iSelFileFormat == 1)
	{
		this->GetDlgItem(IDC_Quality)->EnableWindow(TRUE);
	}
	else
	{
		m_Quality.SetPos(100);
		this->GetDlgItem(IDC_Quality)->EnableWindow(FALSE);
	}
	UpdateData(FALSE);
}

/// <summary>
/// 抓圖
/// </summary>
/// <param name=""></param>
void CCaptureDlg::CaptureBmp(void)
{
	LPCTSTR lpszDrawDeviceName = "DISPLAY";
	CDC dc;
	int Width;
	int Height;
	RECT WndRect;

	//全螢幕
	dc.CreateDC(lpszDrawDeviceName, NULL, NULL, NULL);
	Width = GetSystemMetrics(SM_CXSCREEN);
	Height = GetSystemMetrics(SM_CYSCREEN);

	if (::GetKeyState(VK_SCROLL) == NULL)
	{
		WndRect.top = 0;
		WndRect.bottom = Height;
		WndRect.left = 0;
		WndRect.right = Width;
	}
	else
	{
		//視窗
		CWnd* pWnd = GetForegroundWindow();
		pWnd->GetWindowRect(&WndRect);

		if (WndRect.right < 0)
			WndRect.right = 0;
		if (WndRect.left > Width)
			WndRect.left = Width;
		if (WndRect.top < 0)
			WndRect.top = 0;
		if (WndRect.bottom > Height)
			WndRect.bottom = Height;

		Width = (WndRect.right - WndRect.left);
		if (Width > GetSystemMetrics(SM_CXSCREEN))
			Width = GetSystemMetrics(SM_CXSCREEN);
		Height = WndRect.bottom - WndRect.top;
	}

	CBitmap bm;

	bm.CreateCompatibleBitmap(&dc, Width, Height);

	CDC tdc;
	tdc.CreateCompatibleDC(&dc);

	CBitmap* pOld = tdc.SelectObject(&bm);
	tdc.BitBlt(0, 0, Width, Height, &dc, WndRect.left, WndRect.top, SRCCOPY);

	tdc.SelectObject(pOld);

	BITMAP btm;
	bm.GetBitmap(&btm);
	DWORD size = btm.bmWidthBytes * btm.bmHeight;
	LPSTR lpData = (LPSTR)GlobalAllocPtr(GPTR, size);

	BITMAPINFOHEADER bih;
	bih.biBitCount = btm.bmBitsPixel;
	bih.biClrImportant = 0;
	bih.biClrUsed = 0;
	bih.biCompression = 0;
	bih.biHeight = btm.bmHeight;
	bih.biPlanes = 1;
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biSizeImage = size;
	bih.biWidth = btm.bmWidth;
	bih.biXPelsPerMeter = 3780;
	bih.biYPelsPerMeter = 3780;

	if (btm.bmBitsPixel > 16)
	{
		GetDIBits(dc, bm, 0, bih.biHeight, lpData, (BITMAPINFO*)&bih, DIB_RGB_COLORS);
	}
	else
	{
		bm.GetBitmapBits(size, lpData);
	}

	BITMAPFILEHEADER bfh;
	bfh.bfReserved1 = bfh.bfReserved2 = 0;
	bfh.bfType = ((WORD)('M' << 8) | 'B');
	bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size;
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//轉值修正
	//int size2 = 0;
	LPBYTE lpData2 = NULL;

	//16色模式
	if (btm.bmBitsPixel < 8)
	{
		GlobalFreePtr(lpData);
		return;
	}

	//256色模式
	else if (btm.bmBitsPixel == 8)
	{
		bih.biBitCount = 24;
		int bmWidthBytes2 = btm.bmWidth * 3;
		if ((bmWidthBytes2 % 4) != 0) {
			bmWidthBytes2 = ((bmWidthBytes2 / 4) + 1) * 4;
		}
		bih.biSizeImage = bmWidthBytes2 * btm.bmHeight;

		lpData2 = new(BYTE[bih.biSizeImage]);
		bih.biCompression = BI_RGB;

		BYTE Color;
		LPBYTE chBuffer = (LPBYTE)lpData,
			chRgbBuffer = lpData2;
		int	ColorIndex = 0,
			RgbIndex = 0;

		PALETTEENTRY chColorTable[256];
		::GetSystemPaletteEntries(dc.m_hDC, 0, 256, chColorTable);

		for (int y = 0; y < btm.bmHeight; y++)
		{
			ColorIndex = btm.bmWidthBytes * y;
			RgbIndex = bmWidthBytes2 * y;
			for (int x = 0; x < btm.bmWidth; x++)
			{
				Color = chBuffer[ColorIndex];

				//B
				chRgbBuffer[RgbIndex] = chColorTable[Color].peBlue;
				//G
				chRgbBuffer[RgbIndex + 1] = chColorTable[Color].peGreen;
				//R
				chRgbBuffer[RgbIndex + 2] = chColorTable[Color].peRed;

				ColorIndex++;
				RgbIndex += 3;
			}
		}
	}

	//16bit HiColor模式 (555與565)
	else if (btm.bmBitsPixel == 16)
	{
		bih.biBitCount = 24;
		int bmWidthBytes2 = btm.bmWidth * 3;
		if ((bmWidthBytes2 % 4) != 0) {
			bmWidthBytes2 = ((bmWidthBytes2 / 4) + 1) * 4;
		}
		bih.biSizeImage = bmWidthBytes2 * btm.bmHeight;

		lpData2 = new(BYTE[bih.biSizeImage]);
		bih.biCompression = BI_RGB;

		WORD hiColor;
		LPBYTE chBuffer = (LPBYTE)lpData,
			chRgbBuffer = lpData2;
		int	HiColorIndex = 0,
			RgbIndex = 0;

		BOOL bIsHiColor555 = TRUE;

		LPDIRECTDRAW		lpdd_temp = NULL;
		LPDIRECTDRAW4		lpdd = NULL;
		DDSURFACEDESC2		ddsd;
		LPDIRECTDRAWSURFACE4 lpdds_primary = NULL;
		DDPIXELFORMAT PixelFormat;

		if (DirectDrawCreate(NULL, &lpdd_temp, NULL) == DD_OK)
		{
			if (lpdd_temp->QueryInterface(IID_IDirectDraw4, (LPVOID*)&lpdd) == DD_OK)
			{
				memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
				ddsd.dwSize = sizeof(DDSURFACEDESC2);

				lpdd->GetDisplayMode(&ddsd);
				{
					PixelFormat = ddsd.ddpfPixelFormat;
					if (PixelFormat.dwGBitMask == 0x07e0)
					{
						bIsHiColor555 = FALSE;
					}
				}
				lpdd->Release();
			}
			lpdd_temp->Release();
		}

		if (bIsHiColor555 == TRUE)
		{
			//555 mode
			for (int y = 0; y < btm.bmHeight; y++)
			{
				HiColorIndex = btm.bmWidthBytes * y;
				RgbIndex = bmWidthBytes2 * y;

				for (int x = 0; x < btm.bmWidth; x++)
				{
					hiColor = (WORD)chBuffer[HiColorIndex] + (WORD)(chBuffer[HiColorIndex + 1] << 8);

					//B
					chRgbBuffer[RgbIndex] = (BYTE)((hiColor & 0x001F) << 3);
					//G
					chRgbBuffer[RgbIndex + 1] = (BYTE)((hiColor & 0x03E0) >> 2);
					//R
					chRgbBuffer[RgbIndex + 2] = (BYTE)((hiColor & 0x7C00) >> 7);

					HiColorIndex += 2;
					RgbIndex += 3;
				}
			}
		}
		else
		{
			//565 mode
			for (int y = 0; y < btm.bmHeight; y++)
			{
				HiColorIndex = btm.bmWidthBytes * y;
				RgbIndex = bmWidthBytes2 * y;

				for (int x = 0; x < btm.bmWidth; x++)
				{
					hiColor = (WORD)chBuffer[HiColorIndex] + (WORD)(chBuffer[HiColorIndex + 1] << 8);

					//B
					chRgbBuffer[RgbIndex] = (BYTE)((hiColor & 0x001F) << 3);
					//G
					chRgbBuffer[RgbIndex + 1] = (BYTE)((hiColor & 0x07E0) >> 3);
					//R
					chRgbBuffer[RgbIndex + 2] = (BYTE)((hiColor & 0xF800) >> 8);

					HiColorIndex += 2;
					RgbIndex += 3;
				}
			}
		}
	}

	// 24Bit True Color 模式
	else if (btm.bmBitsPixel == 24)
	{
		bih.biBitCount = 24;
		bih.biSizeImage = size;
		lpData2 = new(BYTE[bih.biSizeImage]);
		::memcpy(lpData2, lpData, bih.biSizeImage);
		bih.biCompression = BI_RGB;
	}

	// 32Bit True Color 模式
	else if (btm.bmBitsPixel == 32)
	{
		bih.biBitCount = 24;
		int bmWidthBytes2 = btm.bmWidth * 3;
		if ((bmWidthBytes2 % 4) != 0) {
			bmWidthBytes2 = ((bmWidthBytes2 / 4) + 1) * 4;
		}
		bih.biSizeImage = bmWidthBytes2 * btm.bmHeight;

		lpData2 = new(BYTE[bih.biSizeImage]);
		bih.biCompression = BI_RGB;

		LPBYTE chBuffer = (LPBYTE)lpData,
			chRgbBuffer = lpData2;
		int	ColorIndex = 0,
			RgbIndex = 0;

		for (int y = 0; y < btm.bmHeight; y++)
		{
			ColorIndex = btm.bmWidthBytes * y;
			RgbIndex = bmWidthBytes2 * y;

			for (int x = 0; x < btm.bmWidth; x++)
			{
				//B
				chRgbBuffer[RgbIndex] = chBuffer[ColorIndex];
				//G
				chRgbBuffer[RgbIndex + 1] = chBuffer[ColorIndex + 1];
				//R
				chRgbBuffer[RgbIndex + 2] = chBuffer[ColorIndex + 2];

				ColorIndex += 4;
				RgbIndex += 3;
			}
		}
	}

	m_dib.SetDIBits(&bfh, &bih, lpData2);

	delete[] lpData2;

	GlobalFreePtr(lpData);
}

void CCaptureDlg::SaveBmpFile(void)
{
	CString name;
	SYSTEMTIME st;
	::GetLocalTime(&st);

	m_filecount++;

	name.Format("%02i%02i%02i_%02i%02i%02i(%d).bmp",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		m_filecount);

	name = m_Path + name;

	m_dib.Save(name);

	m_Number.Format(IDS_pictures, m_filecount);
	UpdateData(FALSE);
}

void CCaptureDlg::SaveJpgFile(void)
{
	CString name;
	SYSTEMTIME st;
	::GetLocalTime(&st);

	m_filecount++;

	name.Format("%02i%02i%02i_%02i%02i%02i(%d).jpg",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		m_filecount);

	name = m_Path + name;

	int iQuality = m_Quality.GetPos();

	m_dib.SaveJpg(name, TRUE, iQuality);

	m_Number.Format(IDS_pictures, m_filecount);
	UpdateData(FALSE);
}

void CCaptureDlg::SaveJp2File(void)
{
	CString name;
	SYSTEMTIME st;
	::GetLocalTime(&st);

	m_filecount++;

	name.Format("%02i%02i%02i_%02i%02i%02i(%d).jp2",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		m_filecount);

	name = m_Path + name;

	int iQuality = m_Quality.GetPos();

	m_dib.SaveAs(name, iQuality);

	m_Number.Format(IDS_pictures, m_filecount);
	UpdateData(FALSE);
}

void CCaptureDlg::SaveJppFile(void)
{
	CString name;
	SYSTEMTIME st;
	::GetLocalTime(&st);

	m_filecount++;

	name.Format("%02i%02i%02i_%02i%02i%02i(%d).jpp",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		m_filecount);

	name = m_Path + name;

	m_dib.SaveJppFile(name);

	m_Number.Format(IDS_pictures, m_filecount);
	UpdateData(FALSE);
}

void CCaptureDlg::SaveAviFile(void)
{
	//全螢幕
	int Width = GetSystemMetrics(SM_CXSCREEN);
	int Height = GetSystemMetrics(SM_CYSCREEN);

	LPCTSTR lpszDrawDeviceName = "DISPLAY";
	CDC dc;
	dc.CreateDC(lpszDrawDeviceName, NULL, NULL, NULL);

	HBITMAP hBackBitmap;
	CBitmap bmp;
	CDC dc2;
	dc2.CreateCompatibleDC(&dc);
	bmp.CreateCompatibleBitmap(&dc, Width, Height);

	CDC tdc;
	tdc.CreateCompatibleDC(&dc);
	CBitmap* pOld = tdc.SelectObject(&bmp);

	tdc.BitBlt(0, 0, Width, Height, &dc, 0, 0, SRCCOPY);
	tdc.SelectObject(pOld);

	hBackBitmap = (HBITMAP)bmp.GetSafeHandle();


	if (m_pAvi == NULL)
	{
		//第1次按下抓圖熱鍵,初始化AVI檔;
		CString name;
		SYSTEMTIME st;
		::GetLocalTime(&st);

		nCount = 1;

		m_filecount++;

		name.Format("%02i%02i%02i_%02i%02i%02i(%d).avi",
			st.wYear,
			st.wMonth,
			st.wDay,
			st.wHour,
			st.wMinute,
			st.wSecond,
			m_filecount);

		name = m_Path + name;

		m_pAvi = new CAviFile(name);

		m_pAvi->AppendNewFrame(hBackBitmap);

		m_Number.Format("自動錄製 Avi (畫格 %d)", nCount);
		UpdateData(FALSE);

		m_nTimerID = SetTimer(1000, 1000, NULL);
	}
	else if (m_WM_Message == WM_TIMER)
	{
		//WM_Timer觸發自動錄製;
		m_pAvi->AppendNewFrame(hBackBitmap);

		nCount++;
		m_Number.Format("自動錄製 Avi (畫格 %d)", nCount);
		UpdateData(FALSE);
	}
	else
	{
		//第2次按下抓圖熱鍵, 結束;
		if (m_nTimerID != NULL)
			this->KillTimer(m_nTimerID);
		m_nTimerID = NULL;

		m_pAvi->AppendNewFrame(hBackBitmap);

		delete (m_pAvi);
		m_pAvi = NULL;

		nCount++;
		m_Number.Format("錄製 Avi 完成 (畫格 %d)", nCount);
		UpdateData(FALSE);
	}

	//delete [] lpData;
}

void CCaptureDlg::SaveWmvFile(void)
{
}

void CCaptureDlg::SaveMovFile(void)
{
}

void CCaptureDlg::OnNMCustomdrawQuality(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此加入控制項告知處理常式程式碼
	*pResult = 0;
	UpdateData(FALSE);
}

void CCaptureDlg::OnTimer(UINT nIDEvent)
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值
	this->m_WM_Message = WM_TIMER;

	if (m_pAvi != NULL)
		SaveAviFile();

	CDialog::OnTimer(nIDEvent);
}

BOOL CCaptureDlg::DestroyWindow()
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
	if (m_nTimerID != NULL)
		this->KillTimer(m_nTimerID);

	if (m_pAvi != NULL)
		delete (m_pAvi);

	return CDialog::DestroyWindow();
}
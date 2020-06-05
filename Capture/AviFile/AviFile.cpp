#include "StdAfx.h"
#include "avifile.h"

#pragma comment(lib,"Vfw32.lib")

CAviFile::CAviFile(LPCTSTR lpszFileName)
{
	::ZeroMemory(&m_AviStreamInfo, sizeof(m_AviStreamInfo));
	::ZeroMemory(&m_AviCompressOptions, sizeof(m_AviCompressOptions));

	AVIFileInit();

	strcpy_s(m_szFileName, lpszFileName);

	m_hHeap = NULL;
	m_hAviDC = NULL;
	m_lpBits = NULL;
	m_pAviFile = NULL;
	m_pAviStream = NULL;
	m_pAviCompressedStream = NULL;

	pAppendFrame[0] = &CAviFile::AppendDummy;
	pAppendFrame[1] = &CAviFile::AppendFrameFirstTime;
	pAppendFrame[2] = &CAviFile::AppendFrameUsual;

	pAppendFrameBits[0] = &CAviFile::AppendDummy2;
	pAppendFrameBits[1] = &CAviFile::AppendFrameFirstTime2;
	pAppendFrameBits[2] = &CAviFile::AppendFrameUsual2;

	nAppendFuncSelector = 1;		//0=Dummy	1=FirstTime	2=Usual
}

CAviFile::~CAviFile(void)
{
	ReleaseMemory();

	AVIFileExit();
}

void CAviFile::ReleaseMemory()
{
	nAppendFuncSelector = 0;		//Point to DummyFunction

	if (m_hAviDC)
	{
		DeleteDC(m_hAviDC);
		m_hAviDC = NULL;
	}
	if (m_pAviCompressedStream)
	{
		AVIStreamRelease(m_pAviCompressedStream);
		m_pAviCompressedStream = NULL;
	}
	if (m_pAviStream)
	{
		AVIStreamRelease(m_pAviStream);
		m_pAviStream = NULL;
	}
	if (m_pAviFile)
	{
		AVIFileRelease(m_pAviFile);
		m_pAviFile = NULL;
	}
	if (m_lpBits)
	{
		HeapFree(m_hHeap, HEAP_NO_SERIALIZE, m_lpBits);
		m_lpBits = NULL;
	}
	if (m_hHeap)
	{
		HeapDestroy(m_hHeap);
		m_hHeap = NULL;
	}
}

HRESULT	CAviFile::AppendFrameFirstTime(HBITMAP	hBitmap)
{
	int	nMaxWidth = GetSystemMetrics(SM_CXSCREEN), nMaxHeight = GetSystemMetrics(SM_CYSCREEN);

	BITMAPINFO	bmpInfo;

	m_hAviDC = CreateCompatibleDC(NULL);
	if (m_hAviDC == NULL)
	{
		//MessageBox(NULL,"Unable to Create Compatible DC","Error",MB_OK|MB_ICONERROR);
		goto TerminateInit;
	}

	ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	GetDIBits(m_hAviDC, hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);

	bmpInfo.bmiHeader.biCompression = BI_RGB;

	if (bmpInfo.bmiHeader.biHeight > nMaxHeight)	nMaxHeight = bmpInfo.bmiHeader.biHeight;
	if (bmpInfo.bmiHeader.biWidth > nMaxWidth)	nMaxWidth = bmpInfo.bmiHeader.biWidth;

	m_hHeap = HeapCreate(HEAP_NO_SERIALIZE, nMaxWidth * nMaxHeight * 4, 0);
	if (m_hHeap == NULL)
	{
		//MessageBox(NULL,"Unable to Allocate Memory","Error",MB_OK);
		goto TerminateInit;
	}

	m_lpBits = HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, nMaxWidth * nMaxHeight * 4);
	if (m_lpBits == NULL)
	{
		//MessageBox(NULL,"Unable to Allocate Memory","Error",MB_OK);
		goto TerminateInit;
	}

	if (FAILED(AVIFileOpen(&m_pAviFile, m_szFileName, OF_CREATE | OF_WRITE, NULL)))
	{
		//MessageBox(NULL,"Unable to Create the Movie File","Error",MB_OK|MB_ICONERROR);
		goto TerminateInit;
	}

	ZeroMemory(&m_AviStreamInfo, sizeof(AVISTREAMINFO));
	m_AviStreamInfo.fccType = streamtypeVIDEO;
	m_AviStreamInfo.fccHandler = mmioFOURCC('M', 'P', 'G', '4');
	m_AviStreamInfo.dwScale = 1;
	m_AviStreamInfo.dwRate = 1;		//15fps
	m_AviStreamInfo.dwQuality = -1;	//Default Quality
	m_AviStreamInfo.dwSuggestedBufferSize = nMaxWidth * nMaxHeight * 4;
	SetRect(&m_AviStreamInfo.rcFrame, 0, 0, bmpInfo.bmiHeader.biWidth, bmpInfo.bmiHeader.biHeight);
	strcpy_s(m_AviStreamInfo.szName, "Video Stream");

	if (FAILED(AVIFileCreateStream(m_pAviFile, &m_pAviStream, &m_AviStreamInfo)))
	{
		//MessageBox(NULL,"Unable to Create Stream","Error",MB_OK|MB_ICONERROR);
		goto TerminateInit;
	}

	ZeroMemory(&m_AviCompressOptions, sizeof(AVICOMPRESSOPTIONS));
	m_AviCompressOptions.fccType = streamtypeVIDEO;
	m_AviCompressOptions.fccHandler = m_AviStreamInfo.fccHandler;
	m_AviCompressOptions.dwFlags = AVICOMPRESSF_KEYFRAMES | AVICOMPRESSF_VALID;//|AVICOMPRESSF_DATARATE;
	m_AviCompressOptions.dwKeyFrameEvery = 15;
	//m_AviCompressOptions.dwBytesPerSecond=1000/8;
	//m_AviCompressOptions.dwQuality=100;

	if (FAILED(AVIMakeCompressedStream(&m_pAviCompressedStream, m_pAviStream, &m_AviCompressOptions, NULL)))
	{
		//MessageBox(NULL,"Unable to Create Compressed Stream","Error",MB_OK);
		goto TerminateInit;
	}

	if (FAILED(AVIStreamSetFormat(m_pAviCompressedStream, 0, (LPVOID)&bmpInfo, bmpInfo.bmiHeader.biSize)))
	{
		//MessageBox(NULL,"Unable to Set Format","Error",MB_OK);
		goto TerminateInit;
	}

	nAppendFuncSelector = 2;		//Point to UsualAppend Function

	return AppendFrameUsual(hBitmap);

TerminateInit:

	ReleaseMemory();	MessageBox(NULL, "Error Occured While Rendering the Movie", "Error", MB_OK | MB_ICONERROR);

	return E_FAIL;
}

HRESULT CAviFile::AppendFrameUsual(HBITMAP hBitmap)
{
	static long	lSample = 0;

	BITMAPINFO	bmpInfo;

	bmpInfo.bmiHeader.biBitCount = 0;
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	GetDIBits(m_hAviDC, hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);

	bmpInfo.bmiHeader.biCompression = BI_RGB;

	GetDIBits(m_hAviDC, hBitmap, 0, bmpInfo.bmiHeader.biHeight, m_lpBits, &bmpInfo, DIB_RGB_COLORS);

	if (FAILED(AVIStreamWrite(m_pAviCompressedStream, lSample++, 1, m_lpBits, bmpInfo.bmiHeader.biSizeImage, 0, NULL, NULL)))
		return E_FAIL;

	return S_OK;
}

HRESULT CAviFile::AppendDummy(HBITMAP)
{
	return E_FAIL;
}

HRESULT CAviFile::AppendNewFrame(HBITMAP hBitmap)
{
	return (this->*pAppendFrame[nAppendFuncSelector])((HBITMAP)hBitmap);
}

HRESULT	CAviFile::AppendNewFrame(int nWidth, int nHeight, LPVOID pBits, int nBitsPerPixel)
{
	return (this->*pAppendFrameBits[nAppendFuncSelector])(nWidth, nHeight, pBits, nBitsPerPixel);
}

HRESULT	CAviFile::AppendFrameFirstTime2(int nWidth, int nHeight, LPVOID pBits, int nBitsPerPixel)
{
	int	nMaxWidth = GetSystemMetrics(SM_CXSCREEN), nMaxHeight = GetSystemMetrics(SM_CYSCREEN);

	BITMAPINFO	bmpInfo;

	m_hAviDC = CreateCompatibleDC(NULL);
	if (m_hAviDC == NULL)
	{
		//MessageBox(NULL,"Unable to Create Compatible DC","Error",MB_OK|MB_ICONERROR);
		goto TerminateInitBits;
	}

	ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biWidth = nWidth;
	bmpInfo.bmiHeader.biHeight = nHeight;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biBitCount = nBitsPerPixel;
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biSizeImage = bmpInfo.bmiHeader.biWidth * bmpInfo.bmiHeader.biHeight * bmpInfo.bmiHeader.biBitCount / 8;

	if (bmpInfo.bmiHeader.biHeight > nMaxHeight)	nMaxHeight = bmpInfo.bmiHeader.biHeight;
	if (bmpInfo.bmiHeader.biWidth > nMaxWidth)	nMaxWidth = bmpInfo.bmiHeader.biWidth;

	m_hHeap = HeapCreate(HEAP_NO_SERIALIZE, nMaxWidth * nMaxHeight * 4, 0);
	if (m_hHeap == NULL)
	{
		//MessageBox(NULL,"Unable to Allocate Memory","Error",MB_OK);
		goto TerminateInitBits;
	}

	m_lpBits = HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, nMaxWidth * nMaxHeight * 4);
	if (m_lpBits == NULL)
	{
		//MessageBox(NULL,"Unable to Allocate Memory","Error",MB_OK);
		goto TerminateInitBits;
	}

	if (FAILED(AVIFileOpen(&m_pAviFile, m_szFileName, OF_CREATE | OF_WRITE, NULL)))
	{
		//MessageBox(NULL,"Unable to Create the Movie File","Error",MB_OK|MB_ICONERROR);
		goto TerminateInitBits;
	}

	ZeroMemory(&m_AviStreamInfo, sizeof(AVISTREAMINFO));
	m_AviStreamInfo.fccType = streamtypeVIDEO;
	m_AviStreamInfo.fccHandler = mmioFOURCC('M', 'P', 'G', '4');
	m_AviStreamInfo.dwScale = 1;
	m_AviStreamInfo.dwRate = 1;		//15fps
	m_AviStreamInfo.dwQuality = -1;	//Default Quality
	m_AviStreamInfo.dwSuggestedBufferSize = nMaxWidth * nMaxHeight * 4;
	SetRect(&m_AviStreamInfo.rcFrame, 0, 0, bmpInfo.bmiHeader.biWidth, bmpInfo.bmiHeader.biHeight);
	strcpy_s(m_AviStreamInfo.szName, "Video Stream");

	if (FAILED(AVIFileCreateStream(m_pAviFile, &m_pAviStream, &m_AviStreamInfo)))
	{
		//MessageBox(NULL,"Unable to Create Stream","Error",MB_OK|MB_ICONERROR);
		goto TerminateInitBits;
	}

	ZeroMemory(&m_AviCompressOptions, sizeof(AVICOMPRESSOPTIONS));
	m_AviCompressOptions.fccType = streamtypeVIDEO;
	m_AviCompressOptions.fccHandler = m_AviStreamInfo.fccHandler;
	m_AviCompressOptions.dwFlags = AVICOMPRESSF_KEYFRAMES | AVICOMPRESSF_VALID;//|AVICOMPRESSF_DATARATE;
	m_AviCompressOptions.dwKeyFrameEvery = 15;
	//m_AviCompressOptions.dwBytesPerSecond=1000/8;
	//m_AviCompressOptions.dwQuality=100;

	if (FAILED(AVIMakeCompressedStream(&m_pAviCompressedStream, m_pAviStream, &m_AviCompressOptions, NULL)))
	{
		//MessageBox(NULL,"Unable to Create Compressed Stream","Error",MB_OK);
		goto TerminateInitBits;
	}

	if (FAILED(AVIStreamSetFormat(m_pAviCompressedStream, 0, (LPVOID)&bmpInfo, bmpInfo.bmiHeader.biSize)))
	{
		//MessageBox(NULL,"Unable to Set Format","Error",MB_OK);
		goto TerminateInitBits;
	}

	nAppendFuncSelector = 2;		//Point to UsualAppend Function

	return AppendFrameUsual2(nWidth, nHeight, pBits, nBitsPerPixel);

TerminateInitBits:

	ReleaseMemory();	MessageBox(NULL, "Error Occured While Rendering the Movie", "Error", MB_OK | MB_ICONERROR);

	return E_FAIL;
}

HRESULT	CAviFile::AppendFrameUsual2(int nWidth, int nHeight, LPVOID pBits, int nBitsPerPixel)
{
	static long	lSample = 0;

	DWORD	dwSize = nWidth * nHeight * nBitsPerPixel / 8;

	if (FAILED(AVIStreamWrite(m_pAviCompressedStream, lSample++, 1, pBits, dwSize, 0, NULL, NULL)))
		return E_FAIL;

	return S_OK;
}

HRESULT	CAviFile::AppendDummy2(int nWidth, int nHeight, LPVOID pBits, int nBitsPerPixel)
{
	return E_FAIL;
}
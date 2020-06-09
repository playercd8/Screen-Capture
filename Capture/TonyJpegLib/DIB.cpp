// DIB.cpp

#include "stdafx.h"
#include "DIB.h"

#include "JpegFile.h"

#include "MiniJpegEnc.h"
#include "MiniJpegDec.h"

#include "TonyJpegEncoder.h"
#include "TonyJpegDecoder.h"

//extern int g_nJpegCodec;//0:IJG Jpeg lib; 1:TonyJpegLib.
static int g_nJpegCodec = 1;//0:IJG Jpeg lib; 1:TonyJpegLib.

///////////////////////////////////////////////////////////////////////////////

CDib::CDib()
{
	m_J2kDll = NULL;
	ImageTranscode = NULL;

	// Set the Dib pointer to
	// NULL so we know if it's
	// been loaded.
	m_pDib = NULL;

	// make tmp file path string
	TCHAR szFile[MAX_PATH];
	::GetModuleFileName(NULL, szFile, sizeof(szFile));
	_tcscpy(_tcsrchr(szFile, TEXT('\\')) + 1, TEXT("tmp.bmp"));
	m_tmpFile = szFile;
}

CDib::~CDib()
{
	// If a Dib has been loaded,
	// delete the memory.
	if (m_pDib != NULL)
		delete[] m_pDib;

	if (m_J2kDll != NULL)
		::FreeLibrary(m_J2kDll);
}

BITMAP CDib::GetBitmap()
{
	BITMAP ret;
	if (m_pDib != NULL)
	{
		ret.bmType= 0;
		ret.bmWidth = m_pBIH->biWidth;
		ret.bmHeight = m_pBIH->biHeight;
		ret.bmWidthBytes = m_pBIH->biWidth * 3;
		if ((ret.bmWidthBytes % 4) != 0) {
			ret.bmWidthBytes = ((ret.bmWidthBytes / 4) + 1) * 4;
		}
		ret.bmPlanes = m_pBIH->biPlanes;
		ret.bmBitsPixel = m_pBIH->biBitCount;
		ret.bmBits = m_pDibBits;
	}
	return ret;
}

void CDib::SetDIBits(PBITMAPFILEHEADER pbfh, PBITMAPINFOHEADER pbih, LPBYTE lpData)
{
	if (m_pDib != NULL)
		delete[] m_pDib;

	m_dwDibSize = pbih->biSizeImage + sizeof(BITMAPINFOHEADER);

	m_pDib = new unsigned char[m_dwDibSize];

	m_nPaletteEntries = 0;

	m_pBIH = (PBITMAPINFOHEADER)m_pDib;
	m_pDibBits = &m_pDib[sizeof(BITMAPINFOHEADER)];

	memcpy(m_pBIH, pbih, sizeof(BITMAPINFOHEADER));
	memcpy(m_pDibBits, lpData, pbih->biSizeImage);

	m_width = pbih->biWidth;
	m_height = pbih->biHeight;

	// Calculate the number of palette entries.
	m_nPaletteEntries = 1 << pbih->biBitCount;
	if (pbih->biBitCount > 8)
		m_nPaletteEntries = 0;
	else if (pbih->biClrUsed != 0)
		m_nPaletteEntries = pbih->biClrUsed;
}

void CDib::LoadJ2kDll(void)
{
	m_J2kDll = ::LoadLibrary("J2kDll.dll");

	if (m_J2kDll != NULL)
	{
		//取得進入點
		ImageTranscode = (LPImageTranscode)GetProcAddress(m_J2kDll, "ImageTranscode");;
	}
}

BOOL CDib::Load(const char* pszFilename)
{
	CFile cf;

	// Attempt to open the Dib file for reading.
	if (!cf.Open(pszFilename, CFile::modeRead))
		return(FALSE);

	// Get the size of the file and store
	// in a local variable. Subtract the
	// size of the BITMAPFILEHEADER structure
	// since we won't keep that in memory.
	DWORD dwDibSize;
	dwDibSize =
		(DWORD)cf.GetLength() - sizeof(BITMAPFILEHEADER);

	// Attempt to allocate the Dib memory.
	unsigned char* pDib;
	pDib = new unsigned char[dwDibSize];
	if (pDib == NULL)
		return(FALSE);

	BITMAPFILEHEADER BFH;

	// Read in the Dib header and data.
	try {
		// Did we read in the entire BITMAPFILEHEADER?
		if (cf.Read(&BFH, sizeof(BITMAPFILEHEADER))
			!= sizeof(BITMAPFILEHEADER) ||

			// Is the type 'MB'?
			BFH.bfType != 'MB' ||

			// Did we read in the remaining data?
			cf.Read(pDib, dwDibSize) != dwDibSize) {
			// Delete the memory if we had any
			// errors and return FALSE.
			delete[] pDib;
			return(FALSE);
		}
	}

	// If we catch an exception, delete the
	// exception, the temporary Dib memory,
	// and return FALSE.
	catch (CFileException* e) {
		e->Delete();
		delete[] pDib;
		return(FALSE);
	}

	// If we got to this point, the Dib has been
	// loaded. If a Dib was already loaded into
	// this class, we must now delete it.
	if (m_pDib != NULL)
		delete m_pDib;

	// Store the local Dib data pointer and
	// Dib size variables in the class member
	// variables.
	m_pDib = pDib;
	m_dwDibSize = dwDibSize;

	// Pointer our BITMAPINFOHEADER and RGBQUAD
	// variables to the correct place in the Dib data.
	m_pBIH = (BITMAPINFOHEADER*)m_pDib;
	m_pPalette =
		(RGBQUAD*)&m_pDib[sizeof(BITMAPINFOHEADER)];

	//	get image width and height
	m_width = m_pBIH->biWidth;
	m_height = m_pBIH->biHeight;

	// Calculate the number of palette entries.
	m_nPaletteEntries = 1 << m_pBIH->biBitCount;
	if (m_pBIH->biBitCount > 8)
		m_nPaletteEntries = 0;
	else if (m_pBIH->biClrUsed != 0)
		m_nPaletteEntries = m_pBIH->biClrUsed;

	// Point m_pDibBits to the actual Dib bits data.
	m_pDibBits =
		&m_pDib[sizeof(BITMAPINFOHEADER) +
		m_nPaletteEntries * sizeof(RGBQUAD)];

	// If we have a valid palette, delete it.
	if (m_Palette.GetSafeHandle() != NULL)
		m_Palette.DeleteObject();

	// If there are palette entries, we'll need
	// to create a LOGPALETTE then create the
	// CPalette palette.
	if (m_nPaletteEntries != 0) {
		// Allocate the LOGPALETTE structure.
		LOGPALETTE* pLogPal = (LOGPALETTE*) new char
			[sizeof(LOGPALETTE) +
			m_nPaletteEntries * sizeof(PALETTEENTRY)];

		if (pLogPal != NULL) {
			// Set the LOGPALETTE to version 0x300
			// and store the number of palette
			// entries.
			pLogPal->palVersion = 0x300;
			pLogPal->palNumEntries = m_nPaletteEntries;

			// Store the RGB values into each
			// PALETTEENTRY element.
			for (int i = 0; i < m_nPaletteEntries; i++) {
				pLogPal->palPalEntry[i].peRed =
					m_pPalette[i].rgbRed;
				pLogPal->palPalEntry[i].peGreen =
					m_pPalette[i].rgbGreen;
				pLogPal->palPalEntry[i].peBlue =
					m_pPalette[i].rgbBlue;
			}

			// Create the CPalette object and
			// delete the LOGPALETTE memory.
			m_Palette.CreatePalette(pLogPal);
			delete[] pLogPal;
		}
	}

	return(TRUE);
}

BOOL CDib::Save(const char* pszFilename)
{
	// If we have no data, we can't save.
	if (m_pDib == NULL)
		return(FALSE);

	CFile cf;

	// Attempt to create the file.
	if (!cf.Open(pszFilename,
		CFile::modeCreate | CFile::modeWrite))
		return(FALSE);

	// Write the data.
	try {
		// First, create a BITMAPFILEHEADER
		// with the correct data.
		BITMAPFILEHEADER BFH;
		memset(&BFH, 0, sizeof(BITMAPFILEHEADER));
		BFH.bfType = 'MB';
		BFH.bfSize = sizeof(BITMAPFILEHEADER) + m_dwDibSize;
		BFH.bfOffBits = sizeof(BITMAPFILEHEADER) +
			sizeof(BITMAPINFOHEADER) +
			m_nPaletteEntries * sizeof(RGBQUAD);

		// Write the BITMAPFILEHEADER and the
		// Dib data.
		cf.Write(&BFH, sizeof(BITMAPFILEHEADER));
		cf.Write(m_pDib, m_dwDibSize);
	}

	// If we get an exception, delete the exception and
	// return FALSE.
	catch (CFileException* e) {
		e->Delete();
		return(FALSE);
	}

	return(TRUE);
}

BOOL CDib::Draw(CDC* pDC, int nX, int nY, int nWidth, int nHeight)
{
	// If we have not data we can't draw.
	if (m_pDib == NULL)
		return(FALSE);

	// Check for the default values of -1
	// in the width and height arguments. If
	// we find -1 in either, we'll set them
	// to the value that's in the BITMAPINFOHEADER.
	if (nWidth == -1)
		nWidth = m_pBIH->biWidth;
	if (nHeight == -1)
		nHeight = m_pBIH->biHeight;

	// Use StretchDIBits to draw the Dib.
	StretchDIBits(pDC->m_hDC, nX, nY,
		nWidth, nHeight,
		0, 0,
		m_pBIH->biWidth, m_pBIH->biHeight,
		m_pDibBits,
		(BITMAPINFO*)m_pBIH,
		BI_RGB, SRCCOPY);

	return(TRUE);
}

BOOL CDib::SetPalette(CDC* pDC)
{
	// If we have not data we
	// won't want to set the palette.
	if (m_pDib == NULL)
		return(FALSE);

	// Check to see if we have a palette
	// handle. For Dibs greater than 8 bits,
	// this will be NULL.
	if (m_Palette.GetSafeHandle() == NULL)
		return(TRUE);

	// Select the palette, realize the palette,
	// then finally restore the old palette.
	CPalette* pOldPalette;
	pOldPalette = pDC->SelectPalette(&m_Palette, FALSE);
	pDC->RealizePalette();
	pDC->SelectPalette(pOldPalette, FALSE);

	return(TRUE);
}

//#include "j2kdll.h"

//////////////////////////////////////////////////////////////////////////////
// for other format images, such as jpc, jp2
// first use transcoder to get one bmp file, then load the bmp;
// Finally clean the bmp
BOOL CDib::LoadFrom(const char* pszFilename)
{
	char source[256], target[256];
	strcpy_s(source, pszFilename);
	strcpy_s(target, (LPSTR)(LPCSTR)m_tmpFile);

	if (ImageTranscode == NULL)
		LoadJ2kDll();
	if (ImageTranscode == NULL)
		return FALSE;

	int ret = ImageTranscode(source, target, 0);
	if (ret != 1)
		return FALSE;

	if (Load((const char*)target) == FALSE)
		return FALSE;

	// TO delete the file
	CFile::Remove(m_tmpFile);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// for other format images, such as jpg, jpc, jp2
// first save the bmp, then use transcoder to compress; Finally clean the bmp
BOOL CDib::SaveAs(const char* pszFilename, int ratio)
{
	char source[256], target[256];
	strcpy_s(target, pszFilename);
	strcpy_s(source, (LPSTR)(LPCSTR)m_tmpFile);

	if (Save((const char*)source) == FALSE)
		return FALSE;

	if (ImageTranscode == NULL)
		LoadJ2kDll();
	if (ImageTranscode == NULL)
		return FALSE;

	int ret = ImageTranscode(source, target, ratio);
	if (ret != 1)
		return FALSE;

	// TO delete the file
	CFile::Remove(m_tmpFile);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

int CDib::SaveJppFile(const char* szPathName)
{
	int nOutputBytes = 0;

	unsigned char* pJpp;
	pJpp = new unsigned char[m_dwDibSize];

	//	DWORD dwTime0 = ::GetTickCount();

	LARGE_INTEGER iLarge;
	QueryPerformanceFrequency(&iLarge);
	double dbFreq = (double)iLarge.QuadPart;

	//	Get starting time
	QueryPerformanceCounter(&iLarge);
	double dbBegin = (double)iLarge.QuadPart;

	/////////////////////////////////////////////////////

	/*
	*	influence quant table:
	*		25:	16->32;	much quantization.
	*		50:	16->16;	moderate quantization.
	*		75:	16->8;	less quantization.
	*/
	CMiniJpegEncoder encoder(50);

	encoder.CompressImage(m_pDibBits, pJpp,
		m_pBIH->biWidth, m_pBIH->biHeight, nOutputBytes);

	//////////////////////////////////////////////////////

//	DWORD dwTime1 = ::GetTickCount();
//	nms = dwTime1 - dwTime0;

	//	Get ending time
	QueryPerformanceCounter(&iLarge);
	double dbEnd = (double)iLarge.QuadPart;

	int nms = (int)((dbEnd - dbBegin) * 1000.0 / dbFreq);

	float ratio = (float)m_dwDibSize / nOutputBytes;
	//CString str;
	//str.Format( "nBytes = %d, Compression Ratio = %f, time = %d ",
	//				nOutputBytes, ratio, nms );
	//AfxMessageBox( str );

	// Attempt to create the file
	CFile cf;
	if (!cf.Open(szPathName, CFile::modeCreate | CFile::modeWrite))
		return(FALSE);

	cf.Write(pJpp, nOutputBytes);

	delete[]pJpp;

	return nOutputBytes;
}

BOOL CDib::ReadJppFile(const char* szPathName)
{
	// Attempt to open the file
	CFile cf;
	if (!cf.Open(szPathName, CFile::modeRead))
		return(FALSE);

	DWORD dwSize;
	dwSize = (DWORD)cf.GetLength();

	unsigned char* pJpp;
	pJpp = new unsigned char[dwSize];

	cf.Read(pJpp, dwSize);
	cf.Close();

	int nWidth, nHeight, nHeadSize;

	//	DWORD dwTime0 = ::GetTickCount();

	LARGE_INTEGER iLarge;
	QueryPerformanceFrequency(&iLarge);
	double dbFreq = (double)iLarge.QuadPart;

	//	Get starting time
	QueryPerformanceCounter(&iLarge);
	double dbBegin = (double)iLarge.QuadPart;

	///////////////////////////////////////////////
	CMiniJpegDecoder decoder;
	decoder.GetImageInfo(pJpp, dwSize, nWidth, nHeight, nHeadSize);

	m_width = nWidth;
	m_height = nHeight;
	///////////////////////////////////////////////

	int nRowBytes = (nWidth * 3 + 3) / 4 * 4;	//	BMP row bytes
	int nDataBytes = nRowBytes * nHeight;
	m_dwDibSize = nDataBytes + sizeof(BITMAPINFOHEADER);
	m_nPaletteEntries = 0;
	m_pPalette = NULL;

	if (m_Palette.GetSafeHandle() != NULL)
		m_Palette.DeleteObject();

	m_pDib = new unsigned char[m_dwDibSize];
	m_pBIH = (BITMAPINFOHEADER*)m_pDib;
	m_pDibBits = &m_pDib[sizeof(BITMAPINFOHEADER)];

	m_pBIH->biSize = sizeof(BITMAPINFOHEADER);
	m_pBIH->biWidth = nWidth;
	m_pBIH->biHeight = nHeight;
	m_pBIH->biPlanes = 1;
	m_pBIH->biBitCount = 24;
	m_pBIH->biCompression = BI_RGB;
	m_pBIH->biSizeImage = 0;
	m_pBIH->biXPelsPerMeter = 0;
	m_pBIH->biYPelsPerMeter = 0;
	m_pBIH->biClrUsed = 0;
	m_pBIH->biClrImportant = 0;

	/////////////////////////////

	decoder.DecompressImage(pJpp + nHeadSize, m_pDibBits);

	/////////////////////////////

	//	Get ending time
	QueryPerformanceCounter(&iLarge);
	double dbEnd = (double)iLarge.QuadPart;

	int ms = (int)((dbEnd - dbBegin) * 1000.0 / dbFreq);

	//CString str;
	//str.Format( "\nDecoding time = %d ", ms );
	//AfxMessageBox( str );

	delete[]pJpp;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

//	switch functions

void CDib::LoadJpg(CString fileName)
{
	if (g_nJpegCodec == 0)
		LoadJpgWithIJGLib(fileName);
	else
		LoadJpgWithTonyLib(fileName);
}

void CDib::SaveJpg(CString filename, BOOL color, int quality)
{
	if (g_nJpegCodec == 0)
		SaveJpgWithIJGLib(filename, color, quality);
	else
		SaveJpgWithTonyLib(filename, color, quality);
}

//////////////////////////////////////////////////////////////////////////////

// jpg load, using IJG code and JpegFile
void CDib::LoadJpgWithIJGLib(CString fileName)
{
	unsigned char* buf;

	// read to buffer tmp; buf is allocated here
	// and read in width and height
	buf = JpegFile::JpegFileToRGB(fileName, &m_width, &m_height);

	//////////////////////
	// set up for display

	// do this before DWORD-alignment!!!
	// this works on packed (not DWORD-aligned) buffers
	// swap red and blue for display
	JpegFile::BGRFromRGB(buf, m_width, m_height);

	// vertical flip for display
	JpegFile::VertFlipBuf(buf, m_width * 3, m_height);

	////////////////////////
	// making bmp here

	//row bytes must be divided by 4; if not, patch zero bytes
	m_rowbytes = (m_width * 3 + 3) / 4 * 4;

	// only the size of memory dib; not include fileheader
	m_dwDibSize = m_rowbytes * m_height + sizeof(BITMAPINFOHEADER);

	// allocate memory
	m_pDib = new unsigned char[m_dwDibSize];

	// make infoheader
	BITMAPINFOHEADER bmiHeader;
	bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader.biWidth = m_width;
	bmiHeader.biHeight = m_height;
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 24;
	bmiHeader.biCompression = BI_RGB;
	bmiHeader.biSizeImage = 0;
	bmiHeader.biXPelsPerMeter = 0;
	bmiHeader.biYPelsPerMeter = 0;
	bmiHeader.biClrUsed = 0;
	bmiHeader.biClrImportant = 0;

	m_pBIH = (BITMAPINFOHEADER*)m_pDib;
	memcpy(m_pBIH, &bmiHeader, sizeof(BITMAPINFOHEADER));

	// no palette
	m_pPalette = (RGBQUAD*)&m_pDib[sizeof(BITMAPINFOHEADER)];
	m_nPaletteEntries = 0;

	// Point m_pDibBits to the actual Dib bits data.
	m_pDibBits = &m_pDib[sizeof(BITMAPINFOHEADER) +
		m_nPaletteEntries * sizeof(RGBQUAD)];

	// get data
	BYTE* pSource, * pTarget;
	pSource = buf;
	pTarget = m_pDibBits;
	for (UINT i = 0; i < m_height; i++)
	{
		memcpy(pTarget, pSource, m_width * 3);
		pSource += m_width * 3;
		pTarget += m_rowbytes;// here patch zero data
	}

	// end making bmp
	////////////////////////

	// free buf
	if (buf != NULL) {
		delete[] buf;
		buf = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////

// jpg save, using IJG code and JpegFile
void CDib::SaveJpgWithIJGLib(CString filename, BOOL color, int quality)
{
	// note, because i'm lazy, most image data in this app
	// is handled as 24-bit images. this makes the DIB
	// conversion easier. 1,4,8, 15/16 and 32 bit DIBs are
	// significantly more difficult to handle.
	m_width = m_pBIH->biWidth;
	m_height = m_pBIH->biHeight;
	m_rowbytes = (m_width * 3 + 3) / 4 * 4;

	unsigned char* buf = new unsigned char[m_width * 3 * m_height];

	// making compact data
	BYTE* pSource, * pTarget;
	pSource = m_pDibBits;
	pTarget = buf;
	for (UINT i = 0; i < m_height; i++)
	{
		memcpy(pTarget, pSource, m_width * 3);
		pSource += m_rowbytes;
		pTarget += m_width * 3;
	}

	// we vertical flip for display. undo that.
	JpegFile::VertFlipBuf(buf, m_width * 3, m_height);

	// we swap red and blue for display, undo that.
	JpegFile::BGRFromRGB(buf, m_width, m_height);

	// save RGB packed buffer to JPG
	BOOL ok = JpegFile::RGBToJpegFile(filename,
		buf,
		m_width,
		m_height,
		color,
		quality);// 75);// quality value 1-100.

// free buf
	if (buf != NULL) {
		delete[] buf;
		buf = NULL;
	}

	if (!ok) {
		AfxMessageBox("Write Error");
	}
}

///////////////////////////////////////////////////////////////////////////////
void CDib::LoadJpgWithTonyLib(CString fileName)
{
	//	AfxMessageBox("LoadJpgWithTonyLib()");

		///////////////////////////////////////////////
		// Step 1: Attempt to open the file
	CFile cf;
	if (!cf.Open(fileName, CFile::modeRead))
	{
		AfxMessageBox("Can not read this jpg file!");
		return;
	}

	DWORD dwSize;
	dwSize = (DWORD)cf.GetLength();

	unsigned char* pJpg;
	pJpg = new unsigned char[dwSize];

	cf.Read(pJpg, dwSize);
	cf.Close();

	int nWidth, nHeight, nHeadSize;

	LARGE_INTEGER iLarge;
	QueryPerformanceFrequency(&iLarge);
	double dbFreq = (double)iLarge.QuadPart;

	//	Get starting time
	QueryPerformanceCounter(&iLarge);
	double dbBegin = (double)iLarge.QuadPart;

	///////////////////////////////////////////////
	// step 2: Read jpg header

	CTonyJpegDecoder decoder;
	if (decoder.ReadJpgHeader(pJpg, dwSize, nWidth, nHeight, nHeadSize) == false)
	{
		AfxMessageBox("Format not supported, sorry!");
		return;
	}

	m_width = nWidth;
	m_height = nHeight;

	///////////////////////////////////////////////
	//	step 3: prepare bmp to receive data

	int nRowBytes = (nWidth * 3 + 3) / 4 * 4;	//	BMP row bytes
	int nDataBytes = nRowBytes * nHeight;
	m_dwDibSize = nDataBytes + sizeof(BITMAPINFOHEADER);
	m_nPaletteEntries = 0;
	m_pPalette = NULL;

	if (m_Palette.GetSafeHandle() != NULL)
		m_Palette.DeleteObject();

	m_pDib = new unsigned char[m_dwDibSize];
	m_pBIH = (BITMAPINFOHEADER*)m_pDib;
	m_pDibBits = &m_pDib[sizeof(BITMAPINFOHEADER)];

	m_pBIH->biSize = sizeof(BITMAPINFOHEADER);
	m_pBIH->biWidth = nWidth;
	m_pBIH->biHeight = nHeight;
	m_pBIH->biPlanes = 1;
	m_pBIH->biBitCount = 24;
	m_pBIH->biCompression = BI_RGB;
	m_pBIH->biSizeImage = 0;
	m_pBIH->biXPelsPerMeter = 0;
	m_pBIH->biYPelsPerMeter = 0;
	m_pBIH->biClrUsed = 0;
	m_pBIH->biClrImportant = 0;

	/////////////////////////////
	// step 4: decompress to bmp buffer
	decoder.DecompressImage(pJpg + nHeadSize, m_pDibBits);

	/////////////////////////////

	//	Get ending time
	QueryPerformanceCounter(&iLarge);
	double dbEnd = (double)iLarge.QuadPart;

	int ms = (int)((dbEnd - dbBegin) * 1000.0 / dbFreq);

	//CString str;
	//str.Format( "\nDecoding time = %d ", ms );
	//AfxMessageBox( str );

	delete[]pJpg;
}

///////////////////////////////////////////////////////////////////////////////
void CDib::SaveJpgWithTonyLib(CString filename, BOOL color, int quality)
{
	//	AfxMessageBox("SaveJpgWithTonyLib()");

	int nOutputBytes = 0;

	unsigned char* pJpg;
	pJpg = new unsigned char[m_dwDibSize];//cannot bigger than bmp

	LARGE_INTEGER iLarge;
	QueryPerformanceFrequency(&iLarge);
	double dbFreq = (double)iLarge.QuadPart;

	//	Get starting time
	QueryPerformanceCounter(&iLarge);
	double dbBegin = (double)iLarge.QuadPart;

	/////////////////////////////////////////////////////

	/*
	*	influence quant table:
	*		25:	16->32;	much quantization.
	*		50:	16->16;	moderate quantization.
	*		75:	16->8;	less quantization.
	*/
	CTonyJpegEncoder encoder(quality);

	encoder.CompressImage(m_pDibBits, pJpg,
		m_pBIH->biWidth, m_pBIH->biHeight, nOutputBytes);

	//////////////////////////////////////////////////////

//	DWORD dwTime1 = ::GetTickCount();
//	nms = dwTime1 - dwTime0;

	//	Get ending time
	QueryPerformanceCounter(&iLarge);
	double dbEnd = (double)iLarge.QuadPart;

	int nms = (int)((dbEnd - dbBegin) * 1000.0 / dbFreq);

	float ratio = (float)m_dwDibSize / nOutputBytes;
	//CString str;
	//str.Format( "nBytes = %d, Compression Ratio = %f, time = %d ",
	//				nOutputBytes, ratio, nms );
	//AfxMessageBox( str );

	// Attempt to create the file
	CFile cf;
	if (!cf.Open(filename, CFile::modeCreate | CFile::modeWrite))
	{
		AfxMessageBox("Can not create jpg file!");
		return;
	}

	cf.Write(pJpg, nOutputBytes);

	delete[]pJpg;
}
///////////////////////////////////////////////////////////////////////////////
// file end //
// DIB.h

#ifndef __DIB_H__
#define __DIB_H__

#include "stdafx.h"
#include <Wingdi.h>

class CDib
{
public:
	CDib();
	~CDib();

	BOOL Load(const char*);
	BOOL Save(const char*);
	BOOL Draw(CDC*, int nX = 0, int nY = 0, int nWidth = -1, int nHeight = -1);
	BOOL SetPalette(CDC*);

	// for other format images, such as jpg, jpc, jp2
	BOOL LoadFrom(const char* pszFilename);
	BOOL SaveAs(const char* pszFilename, int ratio);

	// for jpp
	int SaveJppFile(const char* szFileName);
	BOOL ReadJppFile(const char* szPathName);

	// jpg load and save, using IJG code and JpegFile
	void LoadJpg(CString fileName);
	void SaveJpg(CString filename, BOOL color, int quality);

	void SaveJpgWithTonyLib(CString filename, BOOL color, int quality);
	void SaveJpgWithIJGLib(CString filename, BOOL color, int quality);

	void LoadJpgWithTonyLib(CString fileName);
	void LoadJpgWithIJGLib(CString fileName);

	UINT m_width, m_height, m_rowbytes;

	void SetDIBits(PBITMAPFILEHEADER pbfh, PBITMAPINFOHEADER pbih, LPBYTE lpData);
	inline LPBYTE GetDibBits(void) { return (m_pDibBits); };

private:
	DWORD m_dwDibSize;//not include file header
	unsigned char* m_pDib, * m_pDibBits;
	BITMAPINFOHEADER* m_pBIH;

	CPalette m_Palette;
	RGBQUAD* m_pPalette;
	int m_nPaletteEntries;

	CString m_tmpFile;

	typedef int (WINAPI* LPImageTranscode)(char source[], char target[], int ratio);
	LPImageTranscode ImageTranscode;
	HMODULE m_J2kDll;
	void LoadJ2kDll(void);
};

#endif

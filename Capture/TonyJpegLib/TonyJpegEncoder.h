/****************************************************************************
*	Author:			Dr. Tony Lin											*
*	Email:			lintong@cis.pku.edu.cn									*
*	Release Date:	Dec. 2002												*
*																			*
*	Name:			TonyJpegLib, rewritten from IJG codes					*
*	Source:			IJG v.6a JPEG LIB										*
*	Purpose£º		Support real jpeg file, with readable code				*
*																			*
*	Acknowlegement:	Thanks for great IJG, and Chris Losinger				*
*																			*
*	Legal Issues:	(almost same as IJG with followings)					*
*																			*
*	1. We don't promise that this software works.							*
*	2. You can use this software for whatever you want.						*
*	You don't have to pay.													*
*	3. You may not pretend that you wrote this software. If you use it		*
*	in a program, you must acknowledge somewhere. That is, please			*
*	metion IJG, and Me, Dr. Tony Lin.										*
*																			*
*****************************************************************************/

//	Derived data constructed for each Huffman table
typedef struct tag_HUFFMAN_TABLE {
	unsigned int	code[256];	// code for each symbol
	char			size[256];	// length of code for each symbol
	//If no code has been allocated for a symbol S, size[S] is 0

	/* These two fields directly represent the contents of a JPEG DHT marker */
	unsigned char bits[17];		/* bits[k] = # of symbols with codes of */
	/* length k bits; bits[0] is unused */
	unsigned char huffval[256];		/* The symbols, in order of incr code length */
							/* This field is used only during compression.  It's initialized FALSE when
							* the table is created, and set TRUE when it's been output to the file.
							* You could suppress output of a table by setting this to TRUE.
							* (See jpeg_suppress_tables for an example.)*/
}HUFFMAN_TABLE;

class CTonyJpegEncoder
{
private:

	////////////////////////////////////////////////////////////////////////////
	//	Following data members should be computed in initialization

	unsigned short m_nQuality, m_nScale;

	//	To speed up, we save RGB=>YCbCr color map tables,
	//	with result scaled up by 2^16
	int m_RToY[256], m_GToY[256], m_BToY[256];
	int m_RToCb[256], m_GToCb[256], m_BToCb[256];
	int m_RToCr[256], m_GToCr[256], m_BToCr[256];

	//	To speed up, we precompute two DCT quant tables
	unsigned short m_qtblY[64], m_qtblCbCr[64];

	//	used for write jpeg header
	unsigned char m_dqtY[64], m_dqtCbCr[64];

	HUFFMAN_TABLE m_htblYDC, m_htblYAC, m_htblCbCrDC, m_htblCbCrAC;

	////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////
	//	Following are should be initialized for compressing every image

	unsigned short m_nWidth, m_nHeight;

	//	Three dc records, used for dc differentize for Y/Cb/Cr
	int m_dcY, m_dcCb, m_dcCr;

	//	The size (in bits) and value (in 4 byte buffer) to be written out
	int m_nPutBits, m_nPutVal;

	unsigned char* m_pOutBuf;

	////////////////////////////////////////////////////////////////////////////

private:

	void InitEncoder(void);

	void InitColorTable(void);

	void InitQuantTable(void);

	void ScaleQuantTable(
		unsigned short* tblRst,		//result quant table
		unsigned char* tblStd,		//standard quant table
		unsigned short* tblAan		//scale factor for AAN dct
	);

	void ScaleTable(unsigned char* tbl, int scale, int max);

	void InitHuffmanTable(void);

	void ComputeHuffmanTable(
		unsigned char* pBits,
		unsigned char* pVal,
		HUFFMAN_TABLE* pTbl
	);

	bool CompressOneTile(
		unsigned char* pBgr	//source data, in BGR format
	);

	void BGRToYCbCr(
		unsigned char* pBgr,	//tile source data, in BGR format, 768 bytes
		unsigned char* pY,		//out, Illuminance, 256 bytes
		unsigned char* pCb,	//out, Cb, 256 bytes
		unsigned char* pCr		//out, Cr, 256 bytes
	);

	void BGRToYCbCrEx(
		unsigned char* pBgr,	//in, tile data, in BGR format, 768 bytes
		int* pBlock			//out, Y: 256; Cb: 64; Cr: 64
	);

	void ForwardDct(
		int* data,	//source data, length is 64
		int* coef	//output dct coefficients
	);

	void Quantize(
		int* coef,	//coef is both in and out
		int iBlock	//block id; Y: 0,1,2,3; Cb: 4; Cr: 5
	);

	bool HuffmanEncode(
		int* pCoef,				//	DCT coefficients
		int iBlock				//	0,1,2,3:Y; 4:Cb; 5:Cr;
	);

	bool EmitBits(
		unsigned int code,		//Huffman code
		int size				//Size in bits of the Huffman code
	);

	void EmitLeftBits(void);

	void WriteJpegHeader(void);
	void write_sos(void);
	void write_sof(int code);
	void write_app0(void);
	void write_soi(void);
	void write_dht(int IsCbCr, int IsAc);
	void write_dqt(int index);

public:

	CTonyJpegEncoder();			//default quality is 50

	CTonyJpegEncoder(int nQuality);

	~CTonyJpegEncoder();

	bool CompressImage(
		unsigned char* pInBuf,	//source data, bgr format, 3 bytes per pixel
		unsigned char* pOutBuf,	//destination buffer, in jpg format
		int nWidthPix,			//image width in pixels
		int nHeight,			//height
		int& nOutputBytes		//return number of bytes being written
	);
};

/****************************************************************************
*	Author:			Dr. Tony Lin											*
*	Email:			lintong@cis.pku.edu.cn									*
*	Release Date:	Dec. 2002												*
*																			*
*	Name:			mini JPEG class, rewritten from IJG codes				*
*	Source:			IJG v.6a JPEG LIB										*
*	Purpose£º		1. Readable, so reusable								*
*					2. Customized Jpeg format, with smallest overhead		*
*					3. Standard c++ types, for easily understood			*
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

class CMiniJpegEncoder
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

	//	Derived data constructed for each Huffman table
	typedef struct tag_HUFFMAN_TABLE {
		unsigned int	code[256];	// code for each symbol
		char			size[256];	// length of code for each symbol
		//If no code has been allocated for a symbol S, size[S] is 0
	}HUFFMAN_TABLE;
	HUFFMAN_TABLE m_htblYDC, m_htblYAC, m_htblCbCrDC, m_htblCbCrAC;

	////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////
	//	Following are should be initialized for compressing every image

	unsigned short m_nWidth, m_nHeight;

	//	Three dc records, used for dc differentize for Y/Cb/Cr
	int m_dcY, m_dcCb, m_dcCr;

	//	The size (in bits) and value (in 4 byte buffer) to be written out
	int m_nPutBits, m_nPutVal;

	////////////////////////////////////////////////////////////////////////////

private:

	void InitEncoder(void);

	void InitColorTable(void);

	void InitQuantTable(void);

	void ScaleQuantTable(
		unsigned short* tblRst,		//result quant table
		unsigned short* tblStd,		//standard quant table
		unsigned short* tblAan		//scale factor for AAN dct
	);

	void InitHuffmanTable(void);

	void ComputeHuffmanTable(
		unsigned char* pBits,
		unsigned char* pVal,
		HUFFMAN_TABLE* pTbl
	);

	bool CompressOneTile(
		unsigned char* pBgr,	//source data, in BGR format
		unsigned char* pJpg,	//destination, in jpg format
		int& nTileBytes			//return value, the length of compressed data
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
		unsigned char* pOut,	//	Output byte stream
		int iBlock,				//	0,1,2,3:Y; 4:Cb; 5:Cr;
		int& nBytes				//	Out, Byte number of Output stream
	);

	bool EmitBits(
		unsigned char* pOut,	//Output byte stream
		unsigned int code,		//Huffman code
		int size,				//Size in bits of the Huffman code
		int& nBytes				//Out, bytes length
	);

	void EmitLeftBits(
		unsigned char* pOut,	//Output byte stream
		int& nBytes				//Out, bytes length
	);

public:

	CMiniJpegEncoder();			//default quality is 50

	CMiniJpegEncoder(int nQuality);

	~CMiniJpegEncoder();

	bool CompressImage(
		unsigned char* pInBuf,	//source data, bgr format, 3 bytes per pixel
		unsigned char* pOutBuf,	//destination buffer, in jpg format
		int nWidthPix,			//image width in pixels
		int nHeight,			//height
		int& nOutputBytes		//return number of bytes being written
	);
};

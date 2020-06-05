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

////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TonyJpegEncoder.h"

////////////////////////////////////////////////////////////////////////////////
//JPEG marker codes
typedef enum {
	M_SOF0 = 0xc0,
	M_SOF1 = 0xc1,
	M_SOF2 = 0xc2,
	M_SOF3 = 0xc3,

	M_SOF5 = 0xc5,
	M_SOF6 = 0xc6,
	M_SOF7 = 0xc7,

	M_JPG = 0xc8,
	M_SOF9 = 0xc9,
	M_SOF10 = 0xca,
	M_SOF11 = 0xcb,

	M_SOF13 = 0xcd,
	M_SOF14 = 0xce,
	M_SOF15 = 0xcf,

	M_DHT = 0xc4,

	M_DAC = 0xcc,

	M_RST0 = 0xd0,
	M_RST1 = 0xd1,
	M_RST2 = 0xd2,
	M_RST3 = 0xd3,
	M_RST4 = 0xd4,
	M_RST5 = 0xd5,
	M_RST6 = 0xd6,
	M_RST7 = 0xd7,

	M_SOI = 0xd8,
	M_EOI = 0xd9,
	M_SOS = 0xda,
	M_DQT = 0xdb,
	M_DNL = 0xdc,
	M_DRI = 0xdd,
	M_DHP = 0xde,
	M_EXP = 0xdf,

	M_APP0 = 0xe0,
	M_APP1 = 0xe1,
	M_APP2 = 0xe2,
	M_APP3 = 0xe3,
	M_APP4 = 0xe4,
	M_APP5 = 0xe5,
	M_APP6 = 0xe6,
	M_APP7 = 0xe7,
	M_APP8 = 0xe8,
	M_APP9 = 0xe9,
	M_APP10 = 0xea,
	M_APP11 = 0xeb,
	M_APP12 = 0xec,
	M_APP13 = 0xed,
	M_APP14 = 0xee,
	M_APP15 = 0xef,

	M_JPG0 = 0xf0,
	M_JPG13 = 0xfd,
	M_COM = 0xfe,

	M_TEM = 0x01,

	M_ERROR = 0x100
} JPEG_MARKER;

/*
* jpeg_natural_order[i] is the natural-order position of the i'th
* element of zigzag order.
*
* When reading corrupted data, the Huffman decoders could attempt
* to reference an entry beyond the end of this array (if the decoded
* zero run length reaches past the end of the block).  To prevent
* wild stores without adding an inner-loop test, we put some extra
* "63"s after the real entries.  This will cause the extra coefficient
* to be stored in location 63 of the block, not somewhere random.
* The worst case would be a run-length of 15, which means we need 16
* fake entries.
*/
static const int jpeg_natural_order[64 + 16] = {
		0,  1,  8, 16,  9,  2,  3, 10,
		17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63,
		63, 63, 63, 63, 63, 63, 63, 63,//extra entries for safety
		63, 63, 63, 63, 63, 63, 63, 63
};

// These are the sample quantization tables given in JPEG spec section K.1.
// The spec says that the values given produce "good" quality, and
// when divided by 2, "very good" quality.

unsigned char std_luminance_quant_tbl[64] =
{
		16,  11,  10,  16,  24,  40,  51,  61,
		12,  12,  14,  19,  26,  58,  60,  55,
		14,  13,  16,  24,  40,  57,  69,  56,
		14,  17,  22,  29,  51,  87,  80,  62,
		18,  22,  37,  56,  68, 109, 103,  77,
		24,  35,  55,  64,  81, 104, 113,  92,
		49,  64,  78,  87, 103, 121, 120, 101,
		72,  92,  95,  98, 112, 100, 103,  99
};
unsigned char std_chrominance_quant_tbl[64] =
{
		17,  18,  24,  47,  99,  99,  99,  99,
		18,  21,  26,  66,  99,  99,  99,  99,
		24,  26,  56,  99,  99,  99,  99,  99,
		47,  66,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99
};

////////////////////////////////////////////////////////////////////////////////

#define emit_byte(val)	*m_pOutBuf++=(unsigned char)(val);

#define emit_2bytes(val)			\
*m_pOutBuf=(unsigned char)(((val)>>8)&0xFF);\
*(m_pOutBuf+1)=(unsigned char)((val)&0xFF);\
m_pOutBuf+=2;

#define emit_marker(val)			\
*m_pOutBuf=0xFF;\
*(m_pOutBuf+1)=(unsigned char)(val);\
m_pOutBuf+=2;

////////////////////////////////////////////////////////////////////////////////

CTonyJpegEncoder::CTonyJpegEncoder()
{
	m_nQuality = 50;
	InitEncoder();
}

CTonyJpegEncoder::CTonyJpegEncoder(int nQuality)
{
	m_nQuality = nQuality;
	InitEncoder();
}

CTonyJpegEncoder::~CTonyJpegEncoder()
{
}

////////////////////////////////////////////////////////////////////////////////
//	Prepare for all the tables needed,
//	eg. quantization tables, huff tables, color convert tables
//	1 <= nQuality <= 100, is used for quantization scaling
//	Computing once, and reuse them again and again !!!!!!!

void CTonyJpegEncoder::InitEncoder()
{
	//	prepare color convert table, from bgr to ycbcr
	InitColorTable();

	//	prepare two quant tables, one for Y, and another for CbCr
	InitQuantTable();

	//	prepare four huffman tables:
	InitHuffmanTable();
}

////////////////////////////////////////////////////////////////////////////////
//	Name:	CTonyJpegEncoder::InitColorTable()
//  Purpose:
//			Save RGB->YCC colorspace conversion for reuse, only computing once
//			so dont need multiply in color conversion later

/* Notes:
 *
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0 .. 255 rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *
 *	Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
 *	Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B  + 128
 *	Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B  + 128
 *
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 */

void CTonyJpegEncoder::InitColorTable(void)
{
	int i;
	int nScale = 1L << 16;		//equal to power(2,16)
	int CBCR_OFFSET = 128 << 16;
	/*
	*	nHalf is for (y, cb, cr) rounding, equal to (1L<<16)*0.5
	*	If (R,G,B)=(0,0,1), then Cb = 128.5, should round to 129
	*	Using these tables will produce 129 too:
	*	Cb	= (int)((RToCb[0] + GToCb[0] + BToCb[1]) >> 16)
	*		= (int)(( 0 + 0 + 1L<<15 + 1L<<15 + 128 * 1L<<16 ) >> 16)
	*		= (int)(( 1L<<16 + 128 * 1L<<16 ) >> 16 )
	*		= 129
	*/
	int nHalf = nScale >> 1;

	for (i = 0; i < 256; i++)
	{
		m_RToY[i] = (int)(0.29900 * nScale + 0.5) * i;
		m_GToY[i] = (int)(0.58700 * nScale + 0.5) * i;
		m_BToY[i] = (int)(0.11400 * nScale + 0.5) * i + nHalf;

		m_RToCb[i] = (int)(0.16874 * nScale + 0.5) * (-i);
		m_GToCb[i] = (int)(0.33126 * nScale + 0.5) * (-i);
		m_BToCb[i] = (int)(0.50000 * nScale + 0.5) * i +
			CBCR_OFFSET + nHalf - 1;

		m_RToCr[i] = m_BToCb[i];
		m_GToCr[i] = (int)(0.41869 * nScale + 0.5) * (-i);
		m_BToCr[i] = (int)(0.08131 * nScale + 0.5) * (-i);
	}
}

////////////////////////////////////////////////////////////////////////////////
//	InitQuantTable will produce customized quantization table into:
//		m_tblYQuant[0..63] and m_tblCbCrQuant[0..63]

void CTonyJpegEncoder::InitQuantTable(void)
{
	/*  For AA&N IDCT method, divisors are equal to quantization
	*	coefficients scaled by scalefactor[row]*scalefactor[col], where
	*		scalefactor[0] = 1
	*		scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	*	We apply a further scale factor of 8.
	*/
	static unsigned short aanscales[64] = {
		/* precomputed values scaled up by 14 bits */
		16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
		22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
		21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
		19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
		16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
		12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
		 8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
		 4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
	};

	// Safety checking. Convert 0 to 1 to avoid zero divide.
	m_nScale = m_nQuality;

	if (m_nScale <= 0)
		m_nScale = 1;
	if (m_nScale > 100)
		m_nScale = 100;

	//	Non-linear map: 1->5000, 10->500, 25->200, 50->100, 75->50, 100->0
	if (m_nScale < 50)
		m_nScale = 5000 / m_nScale;
	else
		m_nScale = 200 - m_nScale * 2;

	// use std to initialize
	memcpy(m_dqtY, std_luminance_quant_tbl, 64);
	memcpy(m_dqtCbCr, std_chrominance_quant_tbl, 64);

	//	scale dqt for writing jpeg header
	ScaleTable(m_dqtY, m_nScale, 100);
	ScaleTable(m_dqtCbCr, m_nScale, 100);

	//	Scale the Y and CbCr quant table, respectively
	ScaleQuantTable(m_qtblY, &std_luminance_quant_tbl[0], aanscales);
	ScaleQuantTable(m_qtblCbCr, &std_chrominance_quant_tbl[0], aanscales);
}

////////////////////////////////////////////////////////////////////////////////
void CTonyJpegEncoder::ScaleTable(unsigned char* tbl, int scale, int max)
{
	int i, temp, half = max / 2;

	for (i = 0; i < 64; i++)
	{
		// (1) user scale up
		temp = (int)((m_nScale * tbl[i] + half) / max);

		// limit to baseline range
		if (temp <= 0)
			temp = 1;
		if (temp > 255)
			temp = 255;

		// (2) scaling needed for AA&N algorithm
		tbl[i] = (unsigned char)temp;
	}
}

////////////////////////////////////////////////////////////////////////////////

void CTonyJpegEncoder::ScaleQuantTable(
	unsigned short* tblRst,		//result quant table
	unsigned char* tblStd,		//standard quant table
	unsigned short* tblAan		//scale factor for AAN dct
)
{
	int i, temp, half = 1 << 10;
	for (i = 0; i < 64; i++)
	{
		// (1) user scale up
		temp = (int)((m_nScale * tblStd[i] + 50) / 100);

		// limit to baseline range
		if (temp <= 0)
			temp = 1;
		if (temp > 255)
			temp = 255;

		// (2) scaling needed for AA&N algorithm
		tblRst[i] = (unsigned short)((temp * tblAan[i] + half) >> 11);
	}
}

////////////////////////////////////////////////////////////////////////////////
//	Prepare four Huffman tables:
//		HUFFMAN_TABLE m_htblYDC, m_htblYAC, m_htblCbCrDC, m_htblCbCrAC;

void CTonyJpegEncoder::InitHuffmanTable(void)
{
	//	Y dc component
	static unsigned char bitsYDC[17] =
	{ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
	static unsigned char valYDC[] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

	//	CbCr dc
	static unsigned char bitsCbCrDC[17] =
	{ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
	static unsigned char valCbCrDC[] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

	//	Y ac component
	static unsigned char bitsYAC[17] =
	{ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
	static unsigned char valYAC[] =
	{ 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa };

	//	CbCr ac
	static unsigned char bitsCbCrAC[17] =
	{ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
	static unsigned char valCbCrAC[] =
	{ 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa };

	//	Compute four derived Huffman tables
	ComputeHuffmanTable(bitsYDC, valYDC, &m_htblYDC);
	ComputeHuffmanTable(bitsYAC, valYAC, &m_htblYAC);

	ComputeHuffmanTable(bitsCbCrDC, valCbCrDC, &m_htblCbCrDC);
	ComputeHuffmanTable(bitsCbCrAC, valCbCrAC, &m_htblCbCrAC);
}

////////////////////////////////////////////////////////////////////////////////

//	Compute the derived values for a Huffman table.
//	also, add bits[] and huffval[] to Hufftable for writing jpeg file header

void CTonyJpegEncoder::ComputeHuffmanTable(
	unsigned char* pBits,
	unsigned char* pVal,
	HUFFMAN_TABLE* pTbl)
{
	int p, i, l, lastp, si;
	char huffsize[257];
	unsigned int huffcode[257];
	unsigned int code;

	// First we copy bits and huffval
	memcpy(pTbl->bits, pBits, sizeof(pTbl->bits));
	memcpy(pTbl->huffval, pVal, sizeof(pTbl->huffval));

	/* Figure C.1: make table of Huffman code length for each symbol */
	/* Note that this is in code-length order. */

	p = 0;
	for (l = 1; l <= 16; l++) {
		for (i = 1; i <= (int)pBits[l]; i++)
			huffsize[p++] = (char)l;
	}
	huffsize[p] = 0;
	lastp = p;

	/* Figure C.2: generate the codes themselves */
	/* Note that this is in code-length order. */

	code = 0;
	si = huffsize[0];
	p = 0;
	while (huffsize[p]) {
		while (((int)huffsize[p]) == si) {
			huffcode[p++] = code;
			code++;
		}
		code <<= 1;
		si++;
	}

	/* Figure C.3: generate encoding tables */
	/* These are code and size indexed by symbol value */

	/* Set any codeless symbols to have code length 0;
	* this allows EmitBits to detect any attempt to emit such symbols.
	*/
	memset(pTbl->size, 0, sizeof(pTbl->size));

	for (p = 0; p < lastp; p++) {
		pTbl->code[pVal[p]] = huffcode[p];
		pTbl->size[pVal[p]] = huffsize[p];
	}
}

///////////////////////////////////////////////////////////////////////////////

//write soi, app0, Y_dqt, CbCr_dqt, sof, 4 * dht, sos.
void CTonyJpegEncoder::WriteJpegHeader(void)
{
	write_soi();

	write_app0();

	write_dqt(0);//Y

	write_dqt(1);//cbcr

	write_sof(M_SOF0);

	write_dht(0, 0);//m_htblYDC
	write_dht(0, 1);//m_htblYAC
	write_dht(1, 0);//m_htblCbCrDC
	write_dht(1, 1);//m_htblCbCrAC

	write_sos();
}

///////////////////////////////////////////////////////////////////////////////

void CTonyJpegEncoder::write_soi()
{
	emit_marker(M_SOI);
}

void CTonyJpegEncoder::write_app0()
{
	/*
	 * Length of APP0 block	(2 bytes)
	 * Block ID			(4 bytes - ASCII "JFIF")
	 * Zero byte			(1 byte to terminate the ID string)
	 * Version Major, Minor	(2 bytes - 0x01, 0x01)
	 * Units			(1 byte - 0x00 = none, 0x01 = inch, 0x02 = cm)
	 * Xdpu			(2 bytes - dots per unit horizontal)
	 * Ydpu			(2 bytes - dots per unit vertical)
	 * Thumbnail X size		(1 byte)
	 * Thumbnail Y size		(1 byte)
	 */

	emit_marker(M_APP0);

	emit_2bytes(2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1); /* length */

	emit_byte(0x4A);	/* Identifier: ASCII "JFIF" */
	emit_byte(0x46);
	emit_byte(0x49);
	emit_byte(0x46);
	emit_byte(0);

	/* We currently emit version code 1.01 since we use no 1.02 features.
	 * This may avoid complaints from some older decoders.
	 */
	emit_byte(1);		/* Major version */
	emit_byte(1);		/* Minor version */
	emit_byte(1); /* Pixel size information */
	emit_2bytes(300);
	emit_2bytes(300);
	emit_byte(0);		/* No thumbnail image */
	emit_byte(0);
}

void CTonyJpegEncoder::write_dqt(int index)//0:Y;1:CbCr
{
	unsigned char* dqt;
	if (index == 0)
		dqt = &m_dqtY[0];//changed from std with quality
	else
		dqt = &m_dqtCbCr[0];

	//only allow prec = 0;

	emit_marker(M_DQT);
	emit_2bytes(67);//length
	emit_byte(index);

	int i;
	unsigned char qval;
	for (i = 0; i < 64; i++)
	{
		qval = (unsigned char)(dqt[jpeg_natural_order[i]]);
		emit_byte(qval);
	}
}

//currently support M_SOF0 baseline implementation
void CTonyJpegEncoder::write_sof(int code)
{
	emit_marker(code);
	emit_2bytes(17); //length

	emit_byte(8);//cinfo->data_precision);
	emit_2bytes(m_nHeight);
	emit_2bytes(m_nWidth);
	emit_byte(3);//cinfo->num_components);

	//for Y
	emit_byte(1);//compptr->component_id);
	emit_byte(34);//(compptr->h_samp_factor << 4) + compptr->v_samp_factor);
	emit_byte(0);//quant_tbl_no

	//for Cb
	emit_byte(2);//compptr->component_id);
	emit_byte(17);//(compptr->h_samp_factor << 4) + compptr->v_samp_factor);
	emit_byte(1);//quant_tbl_no

	//for Cr
	emit_byte(3);//compptr->component_id);
	emit_byte(17);//(compptr->h_samp_factor << 4) + compptr->v_samp_factor);
	emit_byte(1);//quant_tbl_no
}

void CTonyJpegEncoder::write_dht(int IsCbCr, int IsAc)
{
	HUFFMAN_TABLE* htbl;
	int index;
	if (IsCbCr)
	{
		if (IsAc)
		{
			htbl = &m_htblCbCrAC;
			index = 17;
		}
		else
		{
			htbl = &m_htblCbCrDC;
			index = 1;
		}
	}
	else
	{
		if (IsAc)
		{
			htbl = &m_htblYAC;
			index = 16;
		}
		else
		{
			htbl = &m_htblYDC;
			index = 0;
		}
	}

	emit_marker(M_DHT);

	int i, length = 0;
	for (i = 1; i <= 16; i++)
		length += htbl->bits[i];

	emit_2bytes(length + 2 + 1 + 16);

	emit_byte(index);

	for (i = 1; i <= 16; i++)
		emit_byte(htbl->bits[i]);

	for (i = 0; i < length; i++)//varible-length
		emit_byte(htbl->huffval[i]);
}

void CTonyJpegEncoder::write_sos()
{
	emit_marker(M_SOS);

	int length = 2 * 3 + 2 + 1 + 3;
	emit_2bytes(length);

	emit_byte(3);//cinfo->comps_in_scan

	//Y
	emit_byte(1);//index
	emit_byte(0);//dc and ac tbl use 0-th tbl

	//Cb
	emit_byte(2);//index
	emit_byte(0x11);//dc and ac tbl use 1-th tbl

	//Cr
	emit_byte(3);//index
	emit_byte(0x11);//dc and ac tbl use 1-th tbl

	emit_byte(0);//Ss
	emit_byte(0x3F);//Se
	emit_byte(0);//  Ah/Al
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//	CTonyJpegEncoder::CompressImage(), the main function in this class !!
//	Don't ask me its purpose, parameter lists, and return value,       d:-)

bool CTonyJpegEncoder::CompressImage(
	unsigned char* pInBuf,	//source data, bgr format, 3 bytes per pixel
	unsigned char* pOutBuf,//destination buffer, in jpg format
	int nWidthPix,			//image width in pixels
	int nHeight,			//height
	int& nOutputBytes		//return number of bytes being written
)
{
	//	Error handling
	if ((pInBuf == 0) || (pOutBuf == 0))
		return false;

	m_nWidth = nWidthPix;
	m_nHeight = nHeight;
	m_pOutBuf = pOutBuf;

	//write soi, app0, Y_dqt, CbCr_dqt, sof, 4 * dht, sos.
	WriteJpegHeader();

	//	let pOutBuf rewind to the first byte of output buffer
	int nHeadBytes = m_pOutBuf - pOutBuf;//here being equal to header size

	// bmp row bytes
	int nRowBytes = (m_nWidth * 3 + 3) / 4 * 4;

	// exception
	if ((nWidthPix <= 0) || (nRowBytes <= 0) || (nHeight <= 0))
		return false;

	//	declares
	int xPixel, yPixel, xTile, yTile, cxTile, cyTile, cxBlock, cyBlock;
	int x, y, nTrueRows, nTrueCols, nTileBytes;
	unsigned char byTile[768], * pTileRow, * pLastPixel, * pHolePixel;

	//	horizontal and vertical count of tile, macroblocks,
	//	or MCU(Minimum Coded Unit), in 16*16 pixels
	cxTile = (nWidthPix + 15) >> 4;
	cyTile = (nHeight + 15) >> 4;

	//	horizontal and vertical count of block, in 8*8 pixels
	cxBlock = cxTile << 1;
	cyBlock = cyTile << 1;

	//	first set output bytes as zero
	nTileBytes = 0;

	//	three dc values set to zero, needed for compressing one new image
	m_dcY = m_dcCb = m_dcCr = 0;

	//	Initialize size (in bits) and value to be written out
	m_nPutBits = 0;
	m_nPutVal = 0;

	//	Run all the tiles, or macroblocks, or MCUs
	//	Vertically flip image data
	for (yTile = 0; yTile < cyTile; yTile++)
	{
		for (xTile = 0; xTile < cxTile; xTile++)
		{
			//	Get tile starting pixel position
			xPixel = xTile << 4;
			yPixel = yTile << 4;

			//	Get the true number of tile columns and rows
			nTrueRows = 16;
			nTrueCols = 16;
			if (yPixel + nTrueRows > nHeight)
				nTrueRows = nHeight - yPixel;
			if (xPixel + nTrueCols > nWidthPix)
				nTrueCols = nWidthPix - xPixel;

			//	Prepare pointer to one row of this tile
			//	Vert-flip here !!!
			pTileRow = pInBuf + (m_nHeight - yPixel) * nRowBytes + xPixel * 3;

			//	Get tile data from pInBuf into byTile. If not full, padding
			//	byTile with the bottom row and rightest pixel
			for (y = 0; y < 16; y++)
			{
				if (y < nTrueRows)
				{
					//	Get data of one row
					pTileRow -= nRowBytes;
					memcpy(byTile + y * 16 * 3, pTileRow, nTrueCols * 3);

					//	padding to full tile with the rightest pixel
					if (nTrueCols < 16)
					{
						pLastPixel = pTileRow + (nTrueCols - 1) * 3;
						pHolePixel = byTile + y * 16 * 3 + nTrueCols * 3;
						for (x = nTrueCols; x < 16; x++)
						{
							memcpy(pHolePixel, pLastPixel, 3);
							pHolePixel += 3;
						}
					}
				}
				else
				{
					//	padding the hole rows with the bottom row
					memcpy(byTile + y * 16 * 3,
						byTile + (nTrueRows - 1) * 16 * 3,
						16 * 3);
				}
			}

			//	Compress this full tile with jpeg algorithm here !!!!!
			//	The compressed data length for this tile is return by nTileBytes
			if (!CompressOneTile(byTile))
				return false;
		}
	}

	//	Maybe there are some bits left, send them here
	if (m_nPutBits > 0)
	{
		EmitLeftBits();
	}

	// write EOI; end of Image
	emit_marker(M_EOI);
	nOutputBytes = m_pOutBuf - pOutBuf;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//	function Purpose:	compress one 16*16 pixels with jpeg
//	destination is m_pOutBuf, in jpg format

bool CTonyJpegEncoder::CompressOneTile(
	unsigned char* pBgr	//source data, in BGR format
)
{
	//	Three color components, 256 + 64 + 64 elements
	int pYCbCr[384];

	//	The DCT outputs are returned scaled up by a factor of 8;
	//	they therefore have a range of +-8K for 8-bit source data
	int coef[64];

	//	Color conversion and subsampling
	//	pY data is in block order, e.g.
	//	block 0 is from pY[0] to pY[63], block 1 is from pY[64] to pY[127]
	BGRToYCbCrEx(pBgr, pYCbCr);

	//	Do Y/Cb/Cr components, Y: 4 blocks; Cb: 1 block; Cr: 1 block
	int i;
	for (i = 0; i < 6; i++)
	{
		ForwardDct(pYCbCr + i * 64, coef);

		Quantize(coef, i);	//coef is both in and out

		HuffmanEncode(coef, i);//output into m_pOutBuf
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
//	Color convertion from bgr to ycbcr for one tile, 16*16 pixels
//	Actually being not used for efficiency !!!!! Please use BGRToYCbCrEx()

void CTonyJpegEncoder::BGRToYCbCr(
	unsigned char* pBgr,	//tile source data, in BGR format, 768 bytes
	unsigned char* pY,		//out, Illuminance, 256 bytes
	unsigned char* pCb,	//out, Cb, 256 bytes
	unsigned char* pCr		//out, Cr, 256 bytes
)
{
	int i;
	unsigned char r, g, b, * pByte = pBgr, * py = pY, * pcb = pCb, * pcr = pCr;

	for (i = 0; i < 256; i++)
	{
		b = *(pByte++);
		g = *(pByte++);
		r = *(pByte++);

		*(py++) = (unsigned char)((m_RToY[r] + m_GToY[g] + m_BToY[b]) >> 16);
		*(pcb++) = (unsigned char)((m_RToCb[r] + m_GToCb[g] + m_BToCb[b]) >> 16);
		*(pcr++) = (unsigned char)((m_RToCr[r] + m_GToCr[g] + m_BToCr[b]) >> 16);
	}
}

////////////////////////////////////////////////////////////////////////////////

//	(1) Color convertion from bgr to ycbcr for one tile, 16*16 pixels;
//	(2) Y has 4 blocks, with block 0 from pY[0] to pY[63],
//		block 1 from pY[64] to pY[127], block 2 from pY[128] to pY[191], ...
//	(3) With Cb/Cr subsampling, i.e. 2*2 pixels get one Cb and one Cr
//		IJG use average for better performance; we just pick one from four
//	(4) Do unsigned->signed conversion, i.e. substract 128

void CTonyJpegEncoder::BGRToYCbCrEx(
	unsigned char* pBgr,	//in, tile data, in BGR format, 768 bytes
	int* pBlock	//out, Y: 256; Cb: 64; Cr: 64
)
{
	int x, y, * py[4], * pcb, * pcr;
	unsigned char r, g, b, * pByte;

	pByte = pBgr;
	for (x = 0; x < 4; x++)
		py[x] = pBlock + 64 * x;
	pcb = pBlock + 256;
	pcr = pBlock + 320;

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			b = *(pByte++);
			g = *(pByte++);
			r = *(pByte++);

			//	block number is ((y/8) * 2 + x/8): 0, 1, 2, 3
			*(py[((y >> 3) << 1) + (x >> 3)] ++) =
				((m_RToY[r] + m_GToY[g] + m_BToY[b]) >> 16) - 128;

			//	Equal to: (( x%2 == 0 )&&( y%2 == 0 ))
			if ((!(y & 1L)) && (!(x & 1L)))
			{
				*(pcb++) =
					((m_RToCb[r] + m_GToCb[g] + m_BToCb[b]) >> 16) - 128;
				*(pcr++) =
					((m_RToCr[r] + m_GToCr[g] + m_BToCr[b]) >> 16) - 128;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

/**************************************************************************
 * (1)	Direct dct algorithms:
 *	are also available, but they are much more complex and seem not to
 *  be any faster when reduced to code.
 *
 *************************************************************************
 * (2)  LL&M dct algorithm:
 *	This implementation is based on an algorithm described in
 *  C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast 1-D DCT
 *  Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
 *  Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
 *	The primary algorithm described there uses 11 multiplies and 29 adds.
 *	We use their alternate method with 12 multiplies and 32 adds.
 *
 ***************************************************************************
 * (3)	AA&N DCT algorithm:
 * This implementation is based on Arai, Agui, and Nakajima's algorithm for
 * scaled DCT.  Their original paper (Trans. IEICE E-71(11):1095) is in
 * Japanese, but the algorithm is described in the Pennebaker & Mitchell
 * JPEG textbook (see REFERENCES section in file README).  The following
 * code is based directly on figure 4-8 in P&M.
 *
 * The AA&N method needs only 5 multiplies and 29 adds.
 *
 * The primary disadvantage of this method is that with fixed-point math,
 * accuracy is lost due to imprecise representation of the scaled
 * quantization values.  The smaller the quantization table entry, the less
 * precise the scaled value, so this implementation does worse with high-
 * quality-setting files than with low-quality ones.
 ***************************************************************************
 */

 //	AA&N DCT algorithm implemention

void CTonyJpegEncoder::ForwardDct(
	int* data,	//source data, length is 64
	int* coef	//output dct coefficients
)
{
	////////////////////////////////////////////////////////////////////////////
	//	define some macroes

	//	Scale up the float with 1<<8; so (int)(0.382683433 * 1<<8 ) = 98
#define FIX_0_382683433  ((int)98)		/* FIX(0.382683433) */
#define FIX_0_541196100  ((int)139)		/* FIX(0.541196100) */
#define FIX_0_707106781  ((int)181)		/* FIX(0.707106781) */
#define FIX_1_306562965  ((int)334)		/* FIX(1.306562965) */

//	This macro changes float multiply into int multiply and right-shift
//	MULTIPLY(a, FIX_0_707106781) = (short)( 0.707106781 * a )
#define MULTIPLY(var,cons)  (int)(((cons) * (var)) >> 8 )

////////////////////////////////////////////////////////////////////////////

	static const int DCTSIZE = 8;
	int x, y;
	int* dataptr;
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	int tmp10, tmp11, tmp12, tmp13;
	int z1, z2, z3, z4, z5, z11, z13, * coefptr;

	/* Pass 1: process rows. */

	dataptr = data;		//input
	coefptr = coef;		//output
	for (y = 0; y < 8; y++)
	{
		tmp0 = dataptr[0] + dataptr[7];
		tmp7 = dataptr[0] - dataptr[7];
		tmp1 = dataptr[1] + dataptr[6];
		tmp6 = dataptr[1] - dataptr[6];
		tmp2 = dataptr[2] + dataptr[5];
		tmp5 = dataptr[2] - dataptr[5];
		tmp3 = dataptr[3] + dataptr[4];
		tmp4 = dataptr[3] - dataptr[4];

		/* Even part */

		tmp10 = tmp0 + tmp3;	/* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		coefptr[0] = tmp10 + tmp11; /* phase 3 */
		coefptr[4] = tmp10 - tmp11;

		z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); /* c4 */
		coefptr[2] = tmp13 + z1;	/* phase 5 */
		coefptr[6] = tmp13 - z1;

		/* Odd part */

		tmp10 = tmp4 + tmp5;	/* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433);	/* c6 */
		z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5;		/* c2-c6 */
		z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5;		/* c2+c6 */
		z3 = MULTIPLY(tmp11, FIX_0_707106781);			/* c4 */

		z11 = tmp7 + z3;		/* phase 5 */
		z13 = tmp7 - z3;

		coefptr[5] = z13 + z2;	/* phase 6 */
		coefptr[3] = z13 - z2;
		coefptr[1] = z11 + z4;
		coefptr[7] = z11 - z4;

		dataptr += 8;		/* advance pointer to next row */
		coefptr += 8;
	}

	/* Pass 2: process columns. */

	coefptr = coef;		//both input and output
	for (x = 0; x < 8; x++)
	{
		tmp0 = coefptr[DCTSIZE * 0] + coefptr[DCTSIZE * 7];
		tmp7 = coefptr[DCTSIZE * 0] - coefptr[DCTSIZE * 7];
		tmp1 = coefptr[DCTSIZE * 1] + coefptr[DCTSIZE * 6];
		tmp6 = coefptr[DCTSIZE * 1] - coefptr[DCTSIZE * 6];
		tmp2 = coefptr[DCTSIZE * 2] + coefptr[DCTSIZE * 5];
		tmp5 = coefptr[DCTSIZE * 2] - coefptr[DCTSIZE * 5];
		tmp3 = coefptr[DCTSIZE * 3] + coefptr[DCTSIZE * 4];
		tmp4 = coefptr[DCTSIZE * 3] - coefptr[DCTSIZE * 4];

		/* Even part */

		tmp10 = tmp0 + tmp3;	/* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		coefptr[DCTSIZE * 0] = tmp10 + tmp11; /* phase 3 */
		coefptr[DCTSIZE * 4] = tmp10 - tmp11;

		z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781); /* c4 */
		coefptr[DCTSIZE * 2] = tmp13 + z1; /* phase 5 */
		coefptr[DCTSIZE * 6] = tmp13 - z1;

		/* Odd part */

		tmp10 = tmp4 + tmp5;	/* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;

		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433); /* c6 */
		z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5; /* c2-c6 */
		z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5; /* c2+c6 */
		z3 = MULTIPLY(tmp11, FIX_0_707106781); /* c4 */

		z11 = tmp7 + z3;		/* phase 5 */
		z13 = tmp7 - z3;

		coefptr[DCTSIZE * 5] = z13 + z2; /* phase 6 */
		coefptr[DCTSIZE * 3] = z13 - z2;
		coefptr[DCTSIZE * 1] = z11 + z4;
		coefptr[DCTSIZE * 7] = z11 - z4;

		coefptr++;			/* advance pointer to next column */
	}
}

////////////////////////////////////////////////////////////////////////////////

void CTonyJpegEncoder::Quantize(
	int* coef,	//coef is both in and out
	int iBlock	//block id; Y: 0,1,2,3; Cb: 4; Cr: 5
)
{
	int temp;
	unsigned short qval, * pQuant;

	if (iBlock < 4)
		pQuant = m_qtblY;
	else
		pQuant = m_qtblCbCr;

	for (int i = 0; i < 64; i++)
	{
		qval = pQuant[i];
		temp = coef[i];

		/* Divide the coefficient value by qval, ensuring proper rounding.
		* Since C does not specify the direction of rounding for negative
		* quotients, we have to force the dividend positive for portability.
		*
		* In most files, at least half of the output values will be zero
		* (at default quantization settings, more like three-quarters...)
		* so we should ensure that this case is fast.  On many machines,
		* a comparison is enough cheaper than a divide to make a special test
		* a win.  Since both inputs will be nonnegative, we need only test
		* for a < b to discover whether a/b is 0.
		* If your machine's division is fast enough, define FAST_DIVIDE.
		*/

		// Notes: Actually we use the second expression !!
/*
#ifdef FAST_DIVIDE
#define DIVIDE_BY(a,b)	a /= b
#else
*/
#define DIVIDE_BY(a,b)	if (a >= b) a /= b; else a = 0
//#endif

		if (temp < 0)
		{
			temp = -temp;
			temp += qval >> 1;	/* for rounding */
			DIVIDE_BY(temp, qval);
			temp = -temp;
		}
		else
		{
			temp += qval >> 1;	/* for rounding */
			DIVIDE_BY(temp, qval);
		}

		coef[i] = temp;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool CTonyJpegEncoder::HuffmanEncode(
	int* pCoef,				//	DCT coefficients
	int iBlock				//	0,1,2,3:Y; 4:Cb; 5:Cr;
)
{
	/*
	* jpeg_natural_order[i] is the natural-order position of the i'th element
	* of zigzag order.
	*
	* When reading corrupted data, the Huffman decoders could attempt
	* to reference an entry beyond the end of this array (if the decoded
	* zero run length reaches past the end of the block).  To prevent
	* wild stores without adding an inner-loop test, we put some extra
	* "63"s after the real entries.  This will cause the extra coefficient
	* to be stored in location 63 of the block, not somewhere random.
	* The worst case would be a run-length of 15, which means we need 16
	* fake entries.
	*/
	static const int jpeg_natural_order[64 + 16] = {
			 0,  1,  8, 16,  9,  2,  3, 10,
			17, 24, 32, 25, 18, 11,  4,  5,
			12, 19, 26, 33, 40, 48, 41, 34,
			27, 20, 13,  6,  7, 14, 21, 28,
			35, 42, 49, 56, 57, 50, 43, 36,
			29, 22, 15, 23, 30, 37, 44, 51,
			58, 59, 52, 45, 38, 31, 39, 46,
			53, 60, 61, 54, 47, 55, 62, 63,
			63, 63, 63, 63, 63, 63, 63, 63,//extra entries for safety
			63, 63, 63, 63, 63, 63, 63, 63
	};

	int temp, temp2, nbits, k, r, i;
	int* block = pCoef;
	int* pLastDc = &m_dcY;
	HUFFMAN_TABLE* dctbl, * actbl;

	if (iBlock < 4)
	{
		dctbl = &m_htblYDC;
		actbl = &m_htblYAC;
		//		pLastDc = &m_dcY;
	}
	else
	{
		dctbl = &m_htblCbCrDC;
		actbl = &m_htblCbCrAC;

		if (iBlock == 4)
			pLastDc = &m_dcCb;
		else
			pLastDc = &m_dcCr;
	}

	/* Encode the DC coefficient difference per section F.1.2.1 */

	temp = temp2 = block[0] - (*pLastDc);
	*pLastDc = block[0];

	if (temp < 0) {
		temp = -temp;		/* temp is abs value of input */
		/* For a negative input, want temp2 = bitwise complement of abs(input) */
		/* This code assumes we are on a two's complement machine */
		temp2--;
	}

	/* Find the number of bits needed for the magnitude of the coefficient */
	nbits = 0;
	while (temp) {
		nbits++;
		temp >>= 1;
	}

	//	Write category number
	if (!EmitBits(dctbl->code[nbits], dctbl->size[nbits]))
		return FALSE;

	//	Write category offset
	if (nbits)			/* EmitBits rejects calls with size 0 */
	{
		if (!EmitBits((unsigned int)temp2, nbits))
			return FALSE;
	}

	////////////////////////////////////////////////////////////////////////////
	/* Encode the AC coefficients per section F.1.2.2 */

	r = 0;			/* r = run length of zeros */

	for (k = 1; k < 64; k++)
	{
		if ((temp = block[jpeg_natural_order[k]]) == 0)
		{
			r++;
		}
		else
		{
			/* if run length > 15, must emit special run-length-16 codes (0xF0) */
			while (r > 15) {
				if (!EmitBits(actbl->code[0xF0], actbl->size[0xF0]))
					return FALSE;
				r -= 16;
			}

			temp2 = temp;
			if (temp < 0) {
				temp = -temp;		/* temp is abs value of input */
				/* This code assumes we are on a two's complement machine */
				temp2--;
			}

			/* Find the number of bits needed for the magnitude of the coefficient */
			nbits = 1;		/* there must be at least one 1 bit */
			while ((temp >>= 1))
				nbits++;

			/* Emit Huffman symbol for run length / number of bits */
			i = (r << 4) + nbits;
			if (!EmitBits(actbl->code[i], actbl->size[i]))
				return FALSE;

			//	Write Category offset
			if (!EmitBits((unsigned int)temp2, nbits))
				return FALSE;

			r = 0;
		}
	}

	//If all the left coefs were zero, emit an end-of-block code
	if (r > 0)
	{
		if (!EmitBits(actbl->code[0], actbl->size[0]))
			return FALSE;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

/* Outputting bits to the file */

/* Only the right 24 bits of put_buffer are used; the valid bits are
 * left-justified in this part.  At most 16 bits can be passed to EmitBits
 * in one call, and we never retain more than 7 bits in put_buffer
 * between calls, so 24 bits are sufficient.
 */

inline bool CTonyJpegEncoder::EmitBits(
	unsigned int code,		//Huffman code
	int size				//Size in bits of the Huffman code
)
{
	/* This routine is heavily used, so it's worth coding tightly. */
	int put_buffer = (int)code;
	int put_bits = m_nPutBits;

	/* if size is 0, caller used an invalid Huffman table entry */
	if (size == 0)
		return false;

	put_buffer &= (((int)1) << size) - 1; /* mask off any extra bits in code */

	put_bits += size;					/* new number of bits in buffer */

	put_buffer <<= 24 - put_bits;		/* align incoming bits */

	put_buffer |= m_nPutVal;			/* and merge with old buffer contents */

	//	If there are more than 8 bits, write it out
	unsigned char uc;
	while (put_bits >= 8)
	{
		//	Write one byte out !!!!
		uc = (unsigned char)((put_buffer >> 16) & 0xFF);
		emit_byte(uc);

		if (uc == 0xFF) {		//need to stuff a zero byte?
			emit_byte(0);	//	Write one byte out !!!!
		}

		put_buffer <<= 8;
		put_bits -= 8;
	}

	m_nPutVal = put_buffer; /* update state variables */
	m_nPutBits = put_bits;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

inline void CTonyJpegEncoder::EmitLeftBits(void)
{
	if (!EmitBits(0x7F, 7)) /* fill 7 bits with ones */
		return;
	/*
		unsigned char uc = (unsigned char) ((m_nPutVal >> 16) & 0xFF);
		emit_byte(uc);		//	Write one byte out !!!!
	*/
	m_nPutVal = 0;
	m_nPutBits = 0;
}

////////////////////////////////////////////////////////////////////////////////
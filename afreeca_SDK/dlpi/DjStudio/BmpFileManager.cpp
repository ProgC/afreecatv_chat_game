//
// 아프리카 개발팀 - 2005년 7월 -
//

#include "stdafx.h"
#include "BmpFileManager.h"

/*
 * macro function
 */
#define GET_BLOCK(handle)	((PBFM_BLOCK)handle)

/*
 * definition
 */
#define SIG_BITMAP			'MB'

/*
 * bitmap format
 */
#pragma pack(push, 1)

typedef struct _BITMAP_FILE_HEADER {
	USHORT					signature;
	ULONG					filesize;
	USHORT					resv1;
	USHORT					resv2;
	ULONG					bits_offset;
} BITMAP_FILE_HEADER, *PBITMAP_FILE_HEADER;

typedef struct _BITMAP_INFO_HEADER {
	ULONG					size;
	ULONG					width;
	ULONG					height;
	USHORT					planes;
	USHORT					bit_count;
	ULONG					is_compressed;
	ULONG					image_size;
	ULONG					x_pels_per_meter;
	ULONG					y_pels_per_meter;
	ULONG					color_used;
	ULONG					color_important;
} BITMAP_INFO_HEADER, *PBITMAP_INFO_HEADER;

typedef struct _BITMAP_HEADER {
	BITMAP_FILE_HEADER		file_hdr;
	BITMAP_INFO_HEADER		info_hdr;
} BITMAP_HEADER, *PBITMAP_HEADER;

#pragma pack(pop)

/*
 * structure
 */
typedef struct _BFM_BLOCK {
	// file
	FILE					*fp_bmp;

	// info
	BITMAP_HEADER			bmp_header;
} BFM_BLOCK, *PBFM_BLOCK;


////////////////////////////////////////////////////////////////////////////////
static
BOOL
BfmpAnalyzeBitmapFile(
	IN	PBFM_BLOCK				pBfmBlock
	)
{
	ULONG				read_size;

	// check parameter
	if( !pBfmBlock || !pBfmBlock->fp_bmp )
	{
		return FALSE;
	}

	// set file pointer to initial pos
	fseek(pBfmBlock->fp_bmp, 0, SEEK_SET);

	// read header
	read_size = fread(
		&pBfmBlock->bmp_header,
		1,
		sizeof(BITMAP_HEADER),
		pBfmBlock->fp_bmp
		);
	if( read_size != sizeof(BITMAP_HEADER) ) { return FALSE; }

	// check bitmap signature
	if( pBfmBlock->bmp_header.file_hdr.signature != SIG_BITMAP )
	{
		return FALSE;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
BOOL
BfmGetBitmapFormat(
	IN	HANDLE					BitmapHandle,
	OUT	PSIMPLE_BITMAP_FORMAT	pBitmapFormat
	)
{
	PBFM_BLOCK				pt_block = NULL;
	PBITMAP_INFO_HEADER		pt_format = NULL;

	// check parameter
	if( !BitmapHandle || !pBitmapFormat )
	{
		return FALSE;
	}

	// get block
	pt_block = GET_BLOCK(BitmapHandle);

	// get format ptr
	pt_format = (PBITMAP_INFO_HEADER)&pt_block->bmp_header.info_hdr;

	// set info
	pBitmapFormat->width			= pt_format->width;
	pBitmapFormat->height			= pt_format->height;
	pBitmapFormat->bit_count		= pt_format->bit_count;
	pBitmapFormat->is_compressed	= pt_format->is_compressed;
	pBitmapFormat->image_size
		= (pt_format->width * pt_format->height * (pt_format->bit_count/8));

	return TRUE;
}

BOOL
BfmReadBitmapData(
	IN	HANDLE					BitmapHandle,
	IN	ULONG					BufferSize,
	IN	PUCHAR					pReadBuffer,
	OUT	PULONG					pBytesRead
	)
{
	PBFM_BLOCK				pt_block = NULL;
	PBITMAP_INFO_HEADER		pt_format = NULL;
	ULONG					read_bytes;
	ULONG					image_size;
	ULONG					dummy_bytes;

	// check parameter
	if( !BitmapHandle || !BufferSize || !pReadBuffer || !pBytesRead )
	{
		return FALSE;
	}

	// get block
	pt_block = GET_BLOCK(BitmapHandle);

	// check file handle
	if( !pt_block->fp_bmp )
	{
		return FALSE;
	}

	// init
	pt_format	= (PBITMAP_INFO_HEADER)&pt_block->bmp_header.info_hdr;
	image_size	= pt_format->width * pt_format->height * (pt_format->bit_count/8);

	// get dummy bytes
	dummy_bytes = 0;
	if( pt_format->bit_count != 32 )
	{
		dummy_bytes	= (pt_format->image_size - image_size) / pt_format->height;
	}

	// read!
	{
		ULONG		h;
		ULONG		line_size;
		UCHAR		dummy_buf[4];

		// get line size
		line_size = pt_format->width * (pt_format->bit_count/8);

		// set file ptr
		fseek( pt_block->fp_bmp, pt_block->bmp_header.file_hdr.bits_offset, SEEK_SET );

		// read!
		for(h=0; h<pt_format->height; h++)
		{
			// image data
			read_bytes = fread(
				pReadBuffer + (line_size*h),
				1,
				line_size,
				pt_block->fp_bmp
				);
			if( (ULONG)read_bytes != line_size )
			{
				return FALSE;
			}

			// dummy
			fread( dummy_buf, 1, dummy_bytes, pt_block->fp_bmp );
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
VOID
BfmCloseBmpFile(
	IN	HANDLE					BitmapHandle
	)
{
	PBFM_BLOCK		pt_block = NULL;

	// check parameter
	if( !BitmapHandle )
	{
		return;
	}

	// get block
	pt_block = GET_BLOCK(BitmapHandle);

	// close file
	if( pt_block->fp_bmp )
	{
		fclose(pt_block->fp_bmp); pt_block->fp_bmp = NULL;
	}

	// free!
	free(pt_block); pt_block = NULL;
}

HANDLE
BfmOpenBmpFile(
	IN	PSTR					pFilePath
	)
{
	BOOL			result;
	PBFM_BLOCK		pt_block = NULL;

	// check parameter
	if( !pFilePath )
	{
		return FALSE;
	}

	// alloc new block
	pt_block = (PBFM_BLOCK)malloc( sizeof(BFM_BLOCK) );
	if( !pt_block )
	{
		return FALSE;
	}
	memset( pt_block, 0, sizeof(BFM_BLOCK) );

	// open file
	pt_block->fp_bmp = fopen( pFilePath, "rb" );
	if( !pt_block->fp_bmp )
	{
		goto $error_occurred;
	}

	// analyze
	result = BfmpAnalyzeBitmapFile( pt_block );
	if( !result )
	{
		goto $error_occurred;
	}

	return (HANDLE)pt_block;

	//
$error_occurred:
	BfmCloseBmpFile( (HANDLE)pt_block );
	if( pt_block )
	{
		free(pt_block); pt_block = NULL;
	}

	return NULL;
}





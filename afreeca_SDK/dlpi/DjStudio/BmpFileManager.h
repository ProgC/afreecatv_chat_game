//
// 아프리카 개발팀 - 2005년 7월 -
//

#ifndef _BITMAP_FILE_MANAGER_HEADER_FILE_
#define _BITMAP_FILE_MANAGER_HEADER_FILE_


/*
 * definition
 */
#ifndef IN
# define IN
#endif

#ifndef OUT
# define OUT
#endif


/*
 * structures
 */
typedef struct _SIMPLE_BITMAP_FORMAT {
	ULONG					is_compressed;
	ULONG					image_size;

	ULONG					width;
	ULONG					height;
	USHORT					bit_count;
} SIMPLE_BITMAP_FORMAT, *PSIMPLE_BITMAP_FORMAT;


/*
 * interfaces
 */
HANDLE
BfmOpenBmpFile(
	IN	PSTR					pFilePath
	);

VOID
BfmCloseBmpFile(
	IN	HANDLE					BitmapHandle
	);

//
BOOL
BfmGetBitmapFormat(
	IN	HANDLE					BitmapHandle,
	OUT	PSIMPLE_BITMAP_FORMAT	pBitmapFormat
	);

BOOL
BfmReadBitmapData(
	IN	HANDLE					BitmapHandle,
	IN	ULONG					BufferSize,
	IN	PUCHAR					pReadBuffer,
	OUT	PULONG					pBytesRead
	);


#endif /* #ifndef _BITMAP_FILE_MANAGER_HEADER_FILE_ */
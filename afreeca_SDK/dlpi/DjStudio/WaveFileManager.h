//
// 아프리카 개발팀 - 2005년 7월 -
//

#ifndef _WAVE_FILE_MANAGER_HEADER_FILE_
#define _WAVE_FILE_MANAGER_HEADER_FILE_


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
typedef struct _SIMPLE_WAVE_FORMAT {
	USHORT				format_tag;
	USHORT				channels;
	USHORT				bits_per_sample;
	ULONG				samples_per_sec;
	ULONG				bytes_per_sec;
} SIMPLE_WAVE_FORMAT, *PSIMPLE_WAVE_FORMAT;


/*
 * interfaces
 */
HANDLE
WfmOpenWaveFile(
	IN	PSTR					pFilePath
	);

VOID
WfmCloseWaveFile(
	IN	HANDLE					WaveHandle
	);

//
BOOL
WfmGetWaveFormat(
	IN	HANDLE					WaveHandle,
	OUT	PSIMPLE_WAVE_FORMAT		pWaveFormat
	);

BOOL
WfmReadWaveData(
	IN	HANDLE					WaveHandle,
	IN	ULONG					BytesToRead,
	IN	PUCHAR					pReadBuffer,
	OUT	PULONG					pBytesRead
	);

BOOL
WfmResetWaveDataPosition(
	IN	HANDLE					WaveHandle
	);


#endif /* #ifndef _WAVE_FILE_MANAGER_HEADER_FILE_ */
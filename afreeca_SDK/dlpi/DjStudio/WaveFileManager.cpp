//
// 아프리카 개발팀 - 2005년 7월 -
//

#include "stdafx.h"
#include "WaveFileManager.h"

/*
 * macro function
 */
#define GET_BLOCK(handle)	((PWFM_BLOCK)handle)

/*
 * definition
 */
#define SIG_RIFF			'FFIR' // RIFF
#define SIG_FORMAT			' tmf' // fmt 
#define SIG_DATA			'atad' // data

/*
 * wave format
 */
#pragma pack(push, 1)

typedef struct _CHUNK_HEADER {
	ULONG				signature;
	ULONG				chunk_size;
} CHUNK_HEADER, *PCHUNK_HEADER;

typedef struct _WAVE_FORMAT_CHUNK {
	USHORT				format_tag;
	USHORT				channels;
	ULONG				samples_per_sec;
	ULONG				bytes_per_sec;		// (samples_per_sec * bytes_per_sample)
	USHORT				bytes_per_sample;	// (bits_per_sample / 8)
	USHORT				bits_per_sample;
} WAVE_FORMAT_CHUNK, *PWAVE_FORMAT_CHUNK;

#pragma pack(pop)

/*
 * structure
 */
typedef struct _WFM_BLOCK {
	// file
	FILE					*fp_wave;

	// info
	ULONG					wave_data_start;
	ULONG					wave_data_size;
	WAVE_FORMAT_CHUNK		wave_format;
} WFM_BLOCK, *PWFM_BLOCK;


////////////////////////////////////////////////////////////////////////////////
static
BOOL
WfmpAnalyzeWaveFile(
	IN	PWFM_BLOCK				pWfmBlock
	)
{
	ULONG				read_size;
	CHUNK_HEADER		chunk_hdr;

	// check parameter
	if( !pWfmBlock || !pWfmBlock->fp_wave )
	{
		return FALSE;
	}

	// set file pointer to initial pos
	fseek(pWfmBlock->fp_wave, 0, SEEK_SET);

	// analyze
	while(1)
	{
		// read chunk header
		read_size = fread( &chunk_hdr, 1, sizeof(CHUNK_HEADER), pWfmBlock->fp_wave );
		if( read_size != sizeof(CHUNK_HEADER) ) { return FALSE; }

		// do!
		switch( chunk_hdr.signature )
		{
		case SIG_RIFF:
			{
				// skip 'WAVE' signature
				fseek( pWfmBlock->fp_wave, 4, SEEK_CUR );
			}
			break;

		case SIG_FORMAT:
			{
				// check wave format size
				if( chunk_hdr.chunk_size < sizeof(WAVE_FORMAT_CHUNK) )
				{
					return FALSE;
				}

				// read wave format
				read_size = fread(
					&pWfmBlock->wave_format,
					1,
					sizeof(WAVE_FORMAT_CHUNK),
					pWfmBlock->fp_wave
					);
				if( read_size != sizeof(WAVE_FORMAT_CHUNK) ) { return FALSE; }

				// adjust filepos
				fseek(
					pWfmBlock->fp_wave,
					(chunk_hdr.chunk_size - sizeof(WAVE_FORMAT_CHUNK)),
					SEEK_CUR
					);
			}
			break;

		case SIG_DATA:
			{
				// store info
				pWfmBlock->wave_data_size  = chunk_hdr.chunk_size;
				pWfmBlock->wave_data_start = (ULONG)ftell(pWfmBlock->fp_wave);

				// done!
				goto $done;
			}
			break;

		default:
			{
				// skip
				fseek(pWfmBlock->fp_wave, chunk_hdr.chunk_size, SEEK_CUR);
			}
			break;
		}
	}

	//
$done:
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
BOOL
WfmGetWaveFormat(
	IN	HANDLE					WaveHandle,
	OUT	PSIMPLE_WAVE_FORMAT		pWaveFormat
	)
{
	PWFM_BLOCK		pt_block = NULL;

	// check parameter
	if( !WaveHandle || !pWaveFormat )
	{
		return FALSE;
	}

	// get block
	pt_block = GET_BLOCK(WaveHandle);

	// set info
	pWaveFormat->format_tag			= pt_block->wave_format.format_tag;
	pWaveFormat->channels			= pt_block->wave_format.channels;
	pWaveFormat->bits_per_sample	= pt_block->wave_format.bits_per_sample;
	pWaveFormat->samples_per_sec	= pt_block->wave_format.samples_per_sec;
	pWaveFormat->bytes_per_sec		= pt_block->wave_format.bytes_per_sec;

	return TRUE;
}

BOOL
WfmReadWaveData(
	IN	HANDLE					WaveHandle,
	IN	ULONG					BytesToRead,
	IN	PUCHAR					pReadBuffer,
	OUT	PULONG					pBytesRead
	)
{
	PWFM_BLOCK		pt_block = NULL;
	ULONG			total_read_size;
	ULONG			readable_bytes;

	// check parameter
	if( !WaveHandle || !BytesToRead || !pReadBuffer || !pBytesRead )
	{
		return FALSE;
	}

	// get block
	pt_block = GET_BLOCK(WaveHandle);

	// check file pointer
	if( !pt_block->fp_wave )
	{
		return FALSE;
	}

	// get readable bytes
	total_read_size	= (ULONG)ftell(pt_block->fp_wave) - pt_block->wave_data_start;
	readable_bytes	= pt_block->wave_data_size - total_read_size;

	// check requsted size
	if( BytesToRead > readable_bytes )
	{
		BytesToRead = readable_bytes;
	}

	// read!
	*pBytesRead = (ULONG)fread( pReadBuffer, 1, BytesToRead, pt_block->fp_wave );

	return TRUE;
}

BOOL
WfmResetWaveDataPosition(
	IN	HANDLE					WaveHandle
	)
{
	PWFM_BLOCK		pt_block = NULL;

	// check parameter
	if( !WaveHandle )
	{
		return FALSE;
	}

	// get block
	pt_block = GET_BLOCK(WaveHandle);

	// check file pointer
	if( !pt_block->fp_wave )
	{
		return FALSE;
	}

	// reset
	fseek( pt_block->fp_wave, pt_block->wave_data_start, SEEK_SET );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
VOID
WfmCloseWaveFile(
	IN	HANDLE					WaveHandle
	)
{
	PWFM_BLOCK		pt_block = NULL;

	// check parameter
	if( !WaveHandle )
	{
		return;
	}

	// get block
	pt_block = GET_BLOCK(WaveHandle);

	// close file
	if( pt_block->fp_wave )
	{
		fclose(pt_block->fp_wave); pt_block->fp_wave = NULL;
	}

	// free!
	free(pt_block); pt_block = NULL;
}

HANDLE
WfmOpenWaveFile(
	IN	PSTR					pFilePath
	)
{
	BOOL			result;
	PWFM_BLOCK		pt_block = NULL;

	// check parameter
	if( !pFilePath )
	{
		return FALSE;
	}

	// alloc new block
	pt_block = (PWFM_BLOCK)malloc( sizeof(WFM_BLOCK) );
	if( !pt_block )
	{
		return FALSE;
	}
	memset( pt_block, 0, sizeof(WFM_BLOCK) );

	// open file
	pt_block->fp_wave = fopen( pFilePath, "rb" );
	if( !pt_block->fp_wave )
	{
		goto $error_occurred;
	}

	// analyze
	result = WfmpAnalyzeWaveFile( pt_block );
	if( !result )
	{
		goto $error_occurred;
	}

	return (HANDLE)pt_block;

	//
$error_occurred:
	WfmCloseWaveFile( (HANDLE)pt_block );
	if( pt_block )
	{
		free(pt_block); pt_block = NULL;
	}

	return NULL;
}




//
// 아프리카 개발팀 - 2005년 7월 -
//

#include "stdafx.h"
#include "DjStudio.h"
#include "BmpFileManager.h"
#include "WaveFileManager.h"

/*
 * plugin guid
 */
static
const
GUID
DJSTUDIOLPI_GUID = { // {C46F0032-9224-4a4d-A735-7841FC7E1467}
	0xc46f0032,
	0x9224,
	0x4a4d,
	{ 0xa7, 0x35, 0x78, 0x41, 0xfc, 0x7e, 0x14, 0x67 }
};

/*
 * definition
 */
#define MYLPI_NAME					"DJ Studio 플러그인"
#define MYLPI_DEVELOPER				"아프리카 개발팀"
#define MYLPI_COMMENTS				"아프리카 예제 플러그인"
#define MYLPI_VER					MAKE_LVERSION(1,0,0,0)

#define VIDEO_WIDTH					320
#define VIDEO_HEIGHT				240
#define VIDEO_DEPTH					32
#define VIDEO_FRAMERATE				15
#define VIDEO_COLORSPACE			LCS_BGRA

#define AUDIO_DATA_SIZE				(44100*2*2/2) // 0.5초 동안의 데이터 크기
#define VIDEO_DATA_SIZE				(VIDEO_WIDTH*VIDEO_HEIGHT*(VIDEO_DEPTH/8))

/*
 * macro function
 */
#define GET_BLOCK(handle)			((PMY_LPI_BLOCK)handle)
#define INSTANCE_HANDLE(hwnd)		((HINSTANCE)::GetWindowLong( hwnd, GWL_HINSTANCE ))
#define CHECK_LPI_HANDLE(lpi)		{ if((u32_t)lpi != (u32_t)&m_MyLpiBlock) return FALSE; }

#define SENDCMD2MGR(cmd, pt_param)	\
	m_MyLpiBlock.pt_mgr_dispatch( m_MyLpiBlock.pt_mgr_context, (lpi_handle_t)&m_MyLpiBlock, cmd, pt_param )


/*
 * structure
 */
typedef struct _MY_LPI_BLOCK {
	BOOL							is_loaded;

	// player
	HWND							player_hwnd;
	HWND							video_hwnd;

	// lpi manager dispatch routine
	PLPIMGR_DISPATCH				pt_mgr_dispatch;
	PVOID							pt_mgr_context;

	// lpi
	HWND							studio_hwnd;

	// media data
	UCHAR							audio_data[AUDIO_DATA_SIZE];
	UCHAR							video_data[VIDEO_DATA_SIZE];
	UCHAR							video_temp_data[VIDEO_DATA_SIZE];
	ULONG							start_tick;

	// dc & bitmap
	HDC								wnd_dc;
	HDC								mem_dc;
	HBITMAP							mem_bitmap;
	HBITMAP							mem_old_bitmap;

	// wave
	HANDLE							wave_handle;
	SIMPLE_WAVE_FORMAT				wave_format;
} MY_LPI_BLOCK, *PMY_LPI_BLOCK;

/*
 * global variables
 */
static HINSTANCE			m_hInstance = NULL;
static MY_LPI_BLOCK			m_MyLpiBlock;


////////////////////////////////////////////////////////////////////////////////
VOID
CALLBACK
AudioTimerProc(
	HWND		hwnd,
    UINT		uMsg,
    UINT_PTR	idEvent,
    DWORD		dwTime
	)
{
	bool_t							lresult;

	LONGLONG						time_value;
	DPORT_PACKET_PROCESS_MEDIA_DATA	media_data;
	ULONG							read_size = AUDIO_DATA_SIZE;

	// init
	RtlZeroMemory( m_MyLpiBlock.audio_data, AUDIO_DATA_SIZE );

	// read
	if( m_MyLpiBlock.wave_handle )
	{
		ULONG		request_size;

		// set request_size
		request_size = (m_MyLpiBlock.wave_format.bytes_per_sec / 2);

		// read wave data
		WfmReadWaveData(
			m_MyLpiBlock.wave_handle,
			request_size,
			m_MyLpiBlock.audio_data,
			&read_size
			);

		// reset wave data position?
		if( read_size != request_size )
		{
			WfmResetWaveDataPosition(m_MyLpiBlock.wave_handle);
		}
	}

	// get time value
	time_value = (LONGLONG)(GetTickCount() - m_MyLpiBlock.start_tick) * 10000;

	// buildup media_data block
	memset( &media_data, 0, sizeof(media_data) );
	media_data.is_audio_data					= TRUE;
	media_data.pt_data							= (void*)m_MyLpiBlock.audio_data;
	media_data.audio_data_desc.channels			= m_MyLpiBlock.wave_format.channels;
	media_data.audio_data_desc.bits_per_sample	= m_MyLpiBlock.wave_format.bits_per_sample;
	media_data.audio_data_desc.samples_per_sec	= m_MyLpiBlock.wave_format.samples_per_sec;
	media_data.audio_data_desc.data_size		= read_size;
	media_data.audio_data_desc.start_time		= time_value;
	media_data.audio_data_desc.end_time			= (time_value + 5000000);

	// send!
	lresult = SENDCMD2MGR( DPORT_CMD_PROCESS_MEDIA_DATA, &media_data );
	if( !lresult )
	{
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////
static
VOID
MakeMemoryBitmap(VOID)
{
	BITMAPINFO	bi;

	// make bitmap info
	memset( &bi, 0, sizeof(BITMAPINFO) );
	bi.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth		= VIDEO_WIDTH;
	bi.bmiHeader.biHeight		= VIDEO_HEIGHT;
	bi.bmiHeader.biPlanes		= 1;
	bi.bmiHeader.biBitCount		= VIDEO_DEPTH;
	bi.bmiHeader.biCompression	= BI_RGB;
	bi.bmiHeader.biSizeImage	= VIDEO_DATA_SIZE;

	// set dib bits
	SetDIBits(
		m_MyLpiBlock.mem_dc,
		m_MyLpiBlock.mem_bitmap,
		0,
		VIDEO_HEIGHT,
		m_MyLpiBlock.video_data,
		&bi,
		DIB_RGB_COLORS
		);
}

VOID
CALLBACK
VideoTimerProc(
	HWND		hwnd,
    UINT		uMsg,
    UINT_PTR	idEvent,
    DWORD		dwTime
	)
{
	bool_t							lresult;
	DPORT_PACKET_PROCESS_MEDIA_DATA	media_data;
	LONGLONG						time_value;

	// get time value
	time_value = (LONGLONG)(GetTickCount() - m_MyLpiBlock.start_tick) * 10000;

	// copy
	CopyMemory( m_MyLpiBlock.video_temp_data, m_MyLpiBlock.video_data, VIDEO_DATA_SIZE );

	// buildup media_data block
	memset( &media_data, 0, sizeof(media_data) );
	media_data.is_audio_data					= FALSE;
	media_data.pt_data							= (void*)m_MyLpiBlock.video_data;
	media_data.video_data_desc.width			= VIDEO_WIDTH;
	media_data.video_data_desc.height			= VIDEO_HEIGHT;
	media_data.video_data_desc.bits_per_pixel	= VIDEO_DEPTH;
	media_data.video_data_desc.colorspace		= VIDEO_COLORSPACE;
	media_data.video_data_desc.framerate		= VIDEO_FRAMERATE;
	media_data.video_data_desc.data_size		= VIDEO_DATA_SIZE;
	media_data.video_data_desc.start_time		= time_value;
	media_data.video_data_desc.end_time			= (time_value + (10000000/VIDEO_FRAMERATE));

	// send!
	lresult = SENDCMD2MGR( DPORT_CMD_PROCESS_MEDIA_DATA, &media_data );
	if( !lresult )
	{
		return;
	}

	// update
	MakeMemoryBitmap();

	// refresh!
	if( m_MyLpiBlock.mem_dc )
	{
		RECT	rect;

		GetClientRect( m_MyLpiBlock.video_hwnd, &rect );
		StretchBlt(
			m_MyLpiBlock.wnd_dc,
			0,
			0,
			rect.right - rect.left,
			rect.bottom - rect.top,
			m_MyLpiBlock.mem_dc,
			0,
			0,
			VIDEO_WIDTH,
			VIDEO_HEIGHT,
			SRCCOPY
			);
	}

	// copy
	CopyMemory( m_MyLpiBlock.video_data, m_MyLpiBlock.video_temp_data, VIDEO_DATA_SIZE );
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
SdpOnBitmapFindButton(
	IN	HWND				hDlg
	)
{
	OPENFILENAME	ofn;
	char			filepath[MAX_PATH] = "";

	// check parameter
	if( !hDlg )
	{
		return FALSE;
	}

	// make find block
	memset( &ofn, 0, sizeof(OPENFILENAME) );
	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= hDlg;
	ofn.lpstrFilter			= "비트맵 파일(*.bmp)\0*.bmp\0모든파일(*.*)\0*.*\0";
	ofn.nFilterIndex		= 1;
	ofn.lpstrFile			= filepath;
	ofn.nMaxFile			= sizeof(filepath);
	ofn.lpstrTitle			= "320x240x32의 비트맵 파일을 선택 하세요.";
	ofn.Flags				= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if( GetOpenFileName(&ofn) != 0 )
	{
		SetDlgItemText( hDlg, IDC_BITMAP_PATH_EDIT, filepath );
	}

	return TRUE;
}

static
BOOL
SdpOnWaveFindButton(
	IN	HWND				hDlg
	)
{
	OPENFILENAME	ofn;
	char			filepath[MAX_PATH] = "";

	// check parameter
	if( !hDlg )
	{
		return FALSE;
	}

	// make find block
	memset( &ofn, 0, sizeof(OPENFILENAME) );
	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= hDlg;
	ofn.lpstrFilter			= "WAVE 파일(*.wav)\0*.wav\0모든파일(*.*)\0*.*\0";
	ofn.nFilterIndex		= 1;
	ofn.lpstrFile			= filepath;
	ofn.nMaxFile			= sizeof(filepath);
	ofn.lpstrTitle			= "WAVE 파일을 선택 하세요!";
	ofn.Flags				= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if( GetOpenFileName(&ofn) != 0 )
	{
		SetDlgItemText( hDlg, IDC_WAVE_PATH_EDIT, filepath );
	}

	return TRUE;
}

static
BOOL
SdpOnReadyButton(
	IN	HWND				hDlg
	)
{
	bool_t							lresult;
	DPORT_PACKET_SET_MEDIA_FORMAT	media_format;
	char							filepath[MAX_PATH];

	// check parameter
	if( !hDlg )
	{
		return FALSE;
	}

	// bitmap
	{
		HANDLE					bmp_handle = NULL;
		SIMPLE_BITMAP_FORMAT	bmp_format;
		ULONG					read_size;

		// get bitmap path
		GetDlgItemText(
			hDlg,
			IDC_BITMAP_PATH_EDIT,
			filepath,
			sizeof(filepath)
			);
		if( strlen(filepath) == 0 )
		{
			MessageBox(hDlg, "비트맵 파일을 선택 하세요.", "에러!", MB_OK);
			return FALSE;
		}

		// open
		bmp_handle = BfmOpenBmpFile( filepath );
		if( !bmp_handle )
		{
			MessageBox(hDlg, "올바르지 않은 비트맵 파일 입니다.", "에러!", MB_OK);
			return FALSE;
		}

		// get bmp format
		BfmGetBitmapFormat( bmp_handle, &bmp_format );

		// check format
		if( bmp_format.bit_count != VIDEO_DEPTH ||
			bmp_format.width != VIDEO_WIDTH ||
			bmp_format.height != VIDEO_HEIGHT ||
			bmp_format.is_compressed )
		{
			MessageBox(hDlg, "인식할 수 있는 비트맵 파일이 아닙니다.", "에러!", MB_OK);
			return FALSE;
		}

		// read!
		BfmReadBitmapData(
			bmp_handle,
			VIDEO_DATA_SIZE,
			m_MyLpiBlock.video_data,
			&read_size
			);

		// make memory bitmap
		MakeMemoryBitmap();

		// close bmp file
		BfmCloseBmpFile( bmp_handle ); bmp_handle = NULL;
	}

	// wave
	{
		char		temp[100];

		// get bitmap path
		GetDlgItemText(
			hDlg,
			IDC_WAVE_PATH_EDIT,
			filepath,
			sizeof(filepath)
			);
		if( strlen(filepath) == 0 )
		{
			MessageBox(hDlg, "WAVE 파일을 선택 하세요.", "에러!", MB_OK);
			return FALSE;
		}

		// already exist?
		if( m_MyLpiBlock.wave_handle )
		{
			WfmCloseWaveFile( m_MyLpiBlock.wave_handle );
			m_MyLpiBlock.wave_handle = NULL;
		}

		// open file
		m_MyLpiBlock.wave_handle = WfmOpenWaveFile( filepath );
		if( !m_MyLpiBlock.wave_handle )
		{
			MessageBox(hDlg, "WAVE파일을 읽을수 없습니다.", "에러!", MB_OK);
			return FALSE;
		}

		// read wave header
		WfmGetWaveFormat( m_MyLpiBlock.wave_handle, &m_MyLpiBlock.wave_format );

		// is wave file right?
		if(	m_MyLpiBlock.wave_format.channels != 2 ||
			m_MyLpiBlock.wave_format.samples_per_sec != 44100 ||
			m_MyLpiBlock.wave_format.bits_per_sample != 16 ||
			m_MyLpiBlock.wave_format.format_tag != 0x01 )
		{
			// close wave
			WfmCloseWaveFile( m_MyLpiBlock.wave_handle );
			m_MyLpiBlock.wave_handle = NULL;

			// error message
			MessageBox(hDlg, "지원하지 않는 WAVE 포맷 입니다.", "에러!", MB_OK);
			return FALSE;
		}

		// set info
		sprintf(temp, "%d", m_MyLpiBlock.wave_format.channels);
		SetDlgItemText(hDlg, IDC_CHANNEL_STATIC, temp);
		sprintf(temp, "%d", m_MyLpiBlock.wave_format.samples_per_sec);
		SetDlgItemText(hDlg, IDC_FREQUENCY_STATIC, temp);
	}

	// set media format
	media_format.audio_format.channels			= m_MyLpiBlock.wave_format.channels;
	media_format.audio_format.bits_per_sample	= m_MyLpiBlock.wave_format.bits_per_sample;
	media_format.audio_format.samples_per_sec	= m_MyLpiBlock.wave_format.samples_per_sec;

	media_format.video_format.width				= VIDEO_WIDTH;
	media_format.video_format.height			= VIDEO_HEIGHT;
	media_format.video_format.bits_per_pixel	= VIDEO_DEPTH;
	media_format.video_format.colorspace		= VIDEO_COLORSPACE;
	media_format.video_format.framerate			= (float)VIDEO_FRAMERATE;
	lresult = SENDCMD2MGR( DPORT_CMD_SET_MEDIA_FORMAT, &media_format );
	if( !lresult )
	{
		MessageBox(hDlg, "지원하지 않는 미디어 포맷 입니다.", "에러!", MB_OK);
		return FALSE;
	}

	// ready to start
	lresult = SENDCMD2MGR( DPORT_CMD_READY_TO_START, NULL );
	if( !lresult )
	{
		MessageBox(hDlg, "방송 준비를 완료 할 수 없습니다.", "에러!", MB_OK);
		return FALSE;
	}

	// complete
	MessageBox(hDlg, "방송 준비 완료!! 방송 시작 버튼을 누르세요!", "완료!", MB_OK);

	return TRUE;
}

static
BOOL
SdpOnChangeButton(
	IN	HWND				hDlg
	)
{
	bool_t								lresult;
	char								filepath[MAX_PATH];
	DPORT_PACKET_SET_MEDIA_FORMAT		media_format;

	HANDLE								bmp_handle = NULL;
	HANDLE								wave_handle = NULL;
	SIMPLE_BITMAP_FORMAT				bmp_format;
	SIMPLE_WAVE_FORMAT					wave_format;

	char		temp[100];
	ULONG		read_size;

	// check parameter
	if( !hDlg )
	{
		return FALSE;
	}

	// kill timer
	KillTimer( m_MyLpiBlock.player_hwnd, 1001 );
	KillTimer( m_MyLpiBlock.player_hwnd, 1002 );

	// bitmap
	{
		// get bitmap path
		GetDlgItemText(
			hDlg,
			IDC_BITMAP_PATH_EDIT,
			filepath,
			sizeof(filepath)
			);
		if( strlen(filepath) == 0 )
		{
			MessageBox(hDlg, "비트맵 파일을 선택 하세요.", "에러!", MB_OK);
			goto $error_occurred;
		}

		// open
		bmp_handle = BfmOpenBmpFile( filepath );
		if( !bmp_handle )
		{
			MessageBox(hDlg, "올바르지 않은 비트맵 파일 입니다.", "에러!", MB_OK);
			goto $error_occurred;
		}

		// get bmp format
		BfmGetBitmapFormat( bmp_handle, &bmp_format );

		// check format
		if( bmp_format.bit_count != VIDEO_DEPTH ||
			bmp_format.width != VIDEO_WIDTH ||
			bmp_format.height != VIDEO_HEIGHT ||
			bmp_format.is_compressed )
		{
			MessageBox(hDlg, "인식할 수 있는 비트맵 파일이 아닙니다.", "에러!", MB_OK);
			goto $error_occurred;
		}
	}

	// wave
	{
		// get bitmap path
		GetDlgItemText(
			hDlg,
			IDC_WAVE_PATH_EDIT,
			filepath,
			sizeof(filepath)
			);
		if( strlen(filepath) == 0 )
		{
			MessageBox(hDlg, "WAVE 파일을 선택 하세요.", "에러!", MB_OK);
			goto $error_occurred;
		}

		// open file
		wave_handle = WfmOpenWaveFile( filepath );
		if( !wave_handle )
		{
			MessageBox(hDlg, "WAVE파일을 읽을수 없습니다.", "에러!", MB_OK);
			goto $error_occurred;
		}

		// read wave header
		WfmGetWaveFormat( wave_handle, &wave_format );

		// is wave file right?
		if(	wave_format.channels > 2 ||
			wave_format.samples_per_sec > 44100 ||
			wave_format.format_tag != 0x01 )
		{
			// error message
			MessageBox(hDlg, "지원하지 않는 WAVE 포맷 입니다.", "에러!", MB_OK);
			goto $error_occurred;
		}
	}

	// set media format
	media_format.audio_format.channels			= wave_format.channels;
	media_format.audio_format.bits_per_sample	= wave_format.bits_per_sample;
	media_format.audio_format.samples_per_sec	= wave_format.samples_per_sec;
	media_format.video_format.width				= VIDEO_WIDTH;
	media_format.video_format.height			= VIDEO_HEIGHT;
	media_format.video_format.bits_per_pixel	= VIDEO_DEPTH;
	media_format.video_format.colorspace		= LCS_BGRA;
	media_format.video_format.framerate			= (float)VIDEO_FRAMERATE;
	lresult = SENDCMD2MGR( DPORT_CMD_CANCEL_MEDIA_FORMAT, NULL );
	if( !lresult ) { goto $change_error; }
	lresult = SENDCMD2MGR( DPORT_CMD_SET_MEDIA_FORMAT, &media_format );
	if( !lresult )
	{
$change_error:
		MessageBox(hDlg, "미디어 포맷 변경 중 에러가 발생 하였습니다.", "에러!", MB_OK);
		goto $error_occurred;
	}

	// read!
	BfmReadBitmapData(
		bmp_handle,
		VIDEO_DATA_SIZE,
		m_MyLpiBlock.video_data,
		&read_size
		);

	// make memory bitmap
	MakeMemoryBitmap();

	// set info
	sprintf(temp, "%d", wave_format.channels);
	SetDlgItemText(hDlg, IDC_CHANNEL_STATIC, temp);
	sprintf(temp, "%d", wave_format.samples_per_sec);
	SetDlgItemText(hDlg, IDC_FREQUENCY_STATIC, temp);

	// close bmp handle
	BfmCloseBmpFile( bmp_handle ); bmp_handle = NULL;

	// clear previous wave handle
	WfmCloseWaveFile( m_MyLpiBlock.wave_handle );
	m_MyLpiBlock.wave_handle = NULL;

	// save wave handle
	m_MyLpiBlock.wave_handle = wave_handle; wave_handle = NULL;
	memcpy( &m_MyLpiBlock.wave_format, &wave_format, sizeof(wave_format) );

	// resume timer
	SetTimer( m_MyLpiBlock.player_hwnd, 1001, 500, AudioTimerProc );
	SetTimer( m_MyLpiBlock.player_hwnd, 1002, (1000/VIDEO_FRAMERATE), VideoTimerProc );

	// complete
	MessageBox(hDlg, "방송 변경 적용 완료!!", "완료!", MB_OK);

	return TRUE;

	//
$error_occurred:
	// close all handles
	BfmCloseBmpFile( bmp_handle ); bmp_handle = NULL;
	WfmCloseWaveFile( wave_handle ); wave_handle = NULL;

	// resume timer
	SetTimer( m_MyLpiBlock.player_hwnd, 1001, 500, AudioTimerProc );
	SetTimer( m_MyLpiBlock.player_hwnd, 1002, (1000/VIDEO_FRAMERATE), VideoTimerProc );

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
CALLBACK
StudioDlgProc(
	IN	HWND				hDlg,
	IN	UINT				iMessage,
	IN	WPARAM				wParam,
	IN	LPARAM				lParam
	)
{
	// dispatch
	switch(iMessage)
	{
	case WM_INITDIALOG:
		{
			;
		}
		break;

	case WM_COMMAND:
		{
			switch(wParam)
			{
			case IDC_BITMAP_FIND_BUTTON:
				{
					SdpOnBitmapFindButton(hDlg);
				}
				return TRUE;

			case IDC_WAVE_FIND_BUTTON:
				{
					SdpOnWaveFindButton(hDlg);
				}
				return TRUE;

			case IDC_READY_BUTTON:
				{
					SdpOnReadyButton(hDlg);
				}
				return TRUE;

			case IDC_CHANGE_BUTTON:
				{
					SdpOnChangeButton(hDlg);
				}
				return TRUE;
			}
		}
		break;

	case WM_CLOSE:
		{
			ShowWindow(hDlg, SW_HIDE);
		}
		return TRUE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
DispatchNotifyInitComplete(VOID)
{
	bool_t							lresult;
	PORT_PACKET_GET_AFREECA_INFO	afreeca_info;

	// get hwnd
	lresult = SENDCMD2MGR( PORT_CMD_GET_AFREECA_INFO, &afreeca_info );
	if( !lresult )
	{
		return FALSE;
	}

	// set hwnd
	m_MyLpiBlock.player_hwnd	= afreeca_info.main_hwnd;
	m_MyLpiBlock.video_hwnd		= afreeca_info.video_hwnd;

	// set dc
	m_MyLpiBlock.wnd_dc = GetDC( m_MyLpiBlock.video_hwnd );
	m_MyLpiBlock.mem_dc = CreateCompatibleDC( m_MyLpiBlock.wnd_dc );
	m_MyLpiBlock.mem_bitmap
		= CreateCompatibleBitmap( m_MyLpiBlock.wnd_dc, VIDEO_WIDTH, VIDEO_HEIGHT );
	m_MyLpiBlock.mem_old_bitmap = (HBITMAP)SelectObject(
		m_MyLpiBlock.mem_dc,
		m_MyLpiBlock.mem_bitmap
		);

	// create studio wnd
	m_MyLpiBlock.studio_hwnd = CreateDialog(
		m_hInstance,
		MAKEINTRESOURCE(IDD_STUDIO_DIALOG),
		m_MyLpiBlock.player_hwnd,
		StudioDlgProc
		);
	if( !m_MyLpiBlock.studio_hwnd )
	{
		return FALSE;
	}
	ShowWindow( m_MyLpiBlock.studio_hwnd, SW_HIDE );

	return TRUE;
}

static
BOOL
DispatchGetUpdateInfo(
	OUT	PLPI_PACKET_GET_UPDATE_INFO		pUpdateInfo
	)
{
	// check parameter
	if( !pUpdateInfo )
	{
		return FALSE;
	}

	// no updated ver
	pUpdateInfo->has_updated_ver = FALSE;

	return TRUE;
}

static
BOOL
DispatchGetUninstallerInfo(
	OUT	PLPI_PACKET_GET_UNINSTALLER_INFO	pUninstallerInfo
	)
{
	// check parameter
	if( !pUninstallerInfo )
	{
		return FALSE;
	}

	// no uninstaller
	pUninstallerInfo->has_uninstaller = FALSE;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
DispatchShowStudio(VOID)
{
	ShowWindow( m_MyLpiBlock.studio_hwnd, SW_SHOW );

	return TRUE;
}

static
BOOL
DispatchHideStudio(VOID)
{
	ShowWindow( m_MyLpiBlock.studio_hwnd, SW_HIDE );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
DispatchStart(VOID)
{
	char filepath[MAX_PATH];

	// not loaded?
	if( !m_MyLpiBlock.is_loaded )
	{
		return FALSE;
	}

	// disable 'ready' button
	EnableWindow(
		GetDlgItem(m_MyLpiBlock.studio_hwnd, IDC_READY_BUTTON),
		FALSE
		);

	// clear time value
	m_MyLpiBlock.start_tick = GetTickCount();

	// kill timer
	KillTimer( m_MyLpiBlock.player_hwnd, 1001 );
	KillTimer( m_MyLpiBlock.player_hwnd, 1002 );

	// set timer
	SetTimer( m_MyLpiBlock.player_hwnd, 1001, 500, AudioTimerProc );
	SetTimer( m_MyLpiBlock.player_hwnd, 1002, (1000/VIDEO_FRAMERATE), VideoTimerProc );

	// play sound
	do
	{
		GetDlgItemText(
			m_MyLpiBlock.studio_hwnd,
			IDC_WAVE_PATH_EDIT,
			filepath,
			sizeof(filepath)
			);
		if( !strlen(filepath) )
		{
			break;
		}

		sndPlaySound(filepath, SND_ASYNC | SND_LOOP);
	} while(FALSE);

	return TRUE;
}

static
BOOL
DispatchPause(VOID)
{
	return TRUE;
}

static
BOOL
DispatchStop(VOID)
{
	// enable 'ready' button
	EnableWindow(
		GetDlgItem(m_MyLpiBlock.studio_hwnd, IDC_READY_BUTTON),
		TRUE
		);

	// kill timer
	KillTimer( m_MyLpiBlock.player_hwnd, 1001 );
	KillTimer( m_MyLpiBlock.player_hwnd, 1002 );

	// stop playing
	sndPlaySound(NULL, SND_ASYNC | SND_LOOP);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
DispatchSetVolume(
	OUT	PDLPI_PACKET_SET_VOLUME			pVolumeInfo
	)
{
	return TRUE;
}

static
BOOL
DispatchGetVolume(
	IN	PDLPI_PACKET_GET_VOLUME			pVolumeInfo
	)
{
	return TRUE;
}

static
BOOL
DispatchUpdateVideoSize(VOID)
{
	return TRUE;
}

static
BOOL
DispatchSetFullscreen(
	IN	PDLPI_PACKET_SET_FULLSCREEN		pFullscreenInfo
	)
{
	return TRUE;
}

static
BOOL
DispatchQueryFullscreen(
	IN	PDLPI_PACKET_QUERY_FULLSCREEN	pFullscreenInfo
	)
{
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
DJSTUDIO_API
bool_t
LPI_Dispatch(
	IN		lpi_handle_t				LpiHandle,
	IN		u32_t						Command,
	IN OUT	void						*pContext
	)
{
	BOOL	result;

	// check lpi handle
	CHECK_LPI_HANDLE(LpiHandle);

	// not loaded?
	if( m_MyLpiBlock.is_loaded == FALSE )
	{
		return FALSE;
	}

	// dispatch commands
	switch(Command)
	{
	case LPI_CMD_NOTIFY_INIT_COMPLETE:
		{
			result = DispatchNotifyInitComplete();
		}
		break;

	case LPI_CMD_GET_UPDATE_INFO:
		{
			result = DispatchGetUpdateInfo(
				(PLPI_PACKET_GET_UPDATE_INFO)pContext
				);
		}
		break;

	case LPI_CMD_GET_UNINSTALLER_INFO:
		{
			result = DispatchGetUninstallerInfo(
				(PLPI_PACKET_GET_UNINSTALLER_INFO)pContext
				);
		}
		break;

	case LPI_CMD_SHOW_STUDIO:
		{
			result = DispatchShowStudio();
		}
		break;

	case LPI_CMD_HIDE_STUDIO:
		{
			result = DispatchHideStudio();
		}
		break;

		//
	case DLPI_CMD_START:
		{
			result = DispatchStart();
		}
		break;

	case DLPI_CMD_PAUSE:
		{
			result = DispatchPause();
		}
		break;

	case DLPI_CMD_STOP:
		{
			result = DispatchStop();
		}
		break;

		//
	case DLPI_CMD_SET_VOLUME:
		{
			result = DispatchSetVolume(
				(PDLPI_PACKET_SET_VOLUME)pContext
				);
		}
		break;

	case DLPI_CMD_GET_VOLUME:
		{
			result = DispatchGetVolume(
				(PDLPI_PACKET_GET_VOLUME)pContext
				);
		}
		break;

	case DLPI_CMD_UPDATE_VIDEO_SIZE:
		{
			result = DispatchUpdateVideoSize();
		}
		break;

	case DLPI_CMD_SET_FULLSCREEN:
		{
			result = DispatchSetFullscreen(
				(PDLPI_PACKET_SET_FULLSCREEN)pContext
				);
		}
		break;

	case DLPI_CMD_QUERY_FULLSCREEN:
		{
			result = DispatchQueryFullscreen(
				(PDLPI_PACKET_QUERY_FULLSCREEN)pContext
				);
		}
		break;

	default:
		result = FALSE;
		break;
	}

	return (bool_t)result;
}

////////////////////////////////////////////////////////////////////////////////
DJSTUDIO_API
bool_t
LPI_Close(
	IN		lpi_handle_t				LpiHandle
	)
{
	// check lpi handle
	CHECK_LPI_HANDLE(LpiHandle);

	// not loaded?
	if( m_MyLpiBlock.is_loaded == FALSE )
	{
		return FALSE;
	}

	// stop!
	DispatchStop();

	// has mem dc?
	if( m_MyLpiBlock.mem_dc )
	{
		SelectObject(m_MyLpiBlock.mem_dc, m_MyLpiBlock.mem_old_bitmap);
		DeleteObject(m_MyLpiBlock.mem_bitmap);
		DeleteDC(m_MyLpiBlock.mem_dc);
	}

	// check wave file handle
	if( m_MyLpiBlock.wave_handle )
	{
		WfmCloseWaveFile( m_MyLpiBlock.wave_handle );
		m_MyLpiBlock.wave_handle = NULL;
	}

	// check studio hwnd
	if( m_MyLpiBlock.studio_hwnd )
	{
		// destory window
		DestroyWindow( m_MyLpiBlock.studio_hwnd );
		m_MyLpiBlock.studio_hwnd = NULL;
	}

	// nullify
	memset( &m_MyLpiBlock, 0, sizeof(m_MyLpiBlock) );
	m_MyLpiBlock.is_loaded = FALSE;

	return TRUE;
}

DJSTUDIO_API
lpi_handle_t
LPI_Create(
	IN		PLPIMGR_DISPATCH			pLpiMgrDispatch,
	IN		void						*pLpiMgrContext
	)
{
	// check parameter
	if( !pLpiMgrDispatch )
	{
		return FALSE;
	}

	// already loaded?
	if( m_MyLpiBlock.is_loaded )
	{
		return FALSE;
	}

	// set
	memset( &m_MyLpiBlock, 0, sizeof(m_MyLpiBlock) );
	m_MyLpiBlock.is_loaded			= TRUE;
	m_MyLpiBlock.studio_hwnd		= NULL;
	m_MyLpiBlock.player_hwnd		= NULL;
	m_MyLpiBlock.video_hwnd			= NULL;
	m_MyLpiBlock.pt_mgr_dispatch	= pLpiMgrDispatch;
	m_MyLpiBlock.pt_mgr_context		= pLpiMgrContext;

	return (lpi_handle_t)&m_MyLpiBlock;
}

////////////////////////////////////////////////////////////////////////////////
DJSTUDIO_API
bool_t
LPI_Identify(
	OUT		PLPI_IDENITIFICATION_INFO	pLpiID
	)
{
	// check parameter
	if( !pLpiID )
	{
		return FALSE;
	}

	// init
	memset( pLpiID, 0, sizeof(LPI_IDENITIFICATION_INFO) );
	pLpiID->lib_ver			= LPI_LIB_VER;
	pLpiID->guid			= DJSTUDIOLPI_GUID;
	strcpy( pLpiID->name, MYLPI_NAME );
	strcpy( pLpiID->developer, MYLPI_DEVELOPER );
	pLpiID->ver				= MYLPI_VER;
	pLpiID->type			= LPI_TYPE_DLPI;
	strcpy( pLpiID->comments, MYLPI_COMMENTS );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
BOOL
APIENTRY
DllMain(
	IN	HINSTANCE			hModule, 
	IN	DWORD				ul_reason_for_call, 
	IN	LPVOID				lpReserved
	)
{
	// is first time?
	if( ul_reason_for_call == DLL_PROCESS_ATTACH )
	{
		// save info
		m_hInstance = hModule;

		// nullify
		memset( &m_MyLpiBlock, 0, sizeof(m_MyLpiBlock) );
		m_MyLpiBlock.is_loaded = FALSE;
	}

    return TRUE;
}

















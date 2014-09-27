//
// 아프리카 개발팀 : 2006년 8월 10일
//


#include "stdafx.h"
#include "Ball.h"


/*
 * plugin guid
 */
static
const
GUID
BALLLPI_GUID = { // {FB4F0AC6-5B1A-4da4-BEB0-CC0B45A738E2}
	0xfb4f0ac6, 
	0x5b1a, 
	0x4da4, 
	{ 0xbe, 0xb0, 0xcc, 0xb, 0x45, 0xa7, 0x38, 0xe2 } 
};


/*
 * definition
 */
#define MYLPI_NAME					"Ball 플러그인"
#define MYLPI_DEVELOPER				"아프리카 개발팀"
#define MYLPI_COMMENTS				"예제 플러그인"
#define MYLPI_VER					MAKE_LVERSION(1,0,0,0)

#define VIDEO_DEPTH					32

#define BALL_CX						20
#define BALL_CY						20

#define BALL_COUNT					10


/*
 * macro function
 */
#define GET_BLOCK(handle)			((PMY_LPI_BLOCK)handle)
#define CHECK_LPI_HANDLE(lpi)		{ if((u32_t)lpi != (u32_t)&m_MyLpiBlock) return FALSE; }

#define SENDCMD2MGR(cmd, pt_param)	\
	m_MyLpiBlock.pt_mgr_dispatch( m_MyLpiBlock.pt_mgr_context, (lpi_handle_t)&m_MyLpiBlock, cmd, pt_param )


/*
 * structure
 */
typedef struct _BALL_DESC {
	POINT							pos;
	LONG							x_vel;
	LONG							y_vel;
	COLORREF						color;
} BALL_DESC, *PBALL_DESC;

typedef struct _MY_LPI_BLOCK {
	BOOL							is_loaded;

	// lpi manager dispatch routine
	PLPIMGR_DISPATCH				pt_mgr_dispatch;
	PVOID							pt_mgr_context;

	// ball info
	BALL_DESC						ball[BALL_COUNT];
} MY_LPI_BLOCK, *PMY_LPI_BLOCK;


/*
 * global variables
 */
static HINSTANCE			m_hInstance = NULL;
static MY_LPI_BLOCK			m_MyLpiBlock;



////////////////////////////////////////////////////////////////////////////////
static
BOOL
DispatchNotifyInitComplete(VOID)
{
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
	MessageBox(NULL, "지원하지 않는 기능입니다.", "Ball", MB_OK);

	return TRUE;
}

static
BOOL
DispatchHideStudio(VOID)
{
	MessageBox(NULL, "지원하지 않는 기능입니다.", "Ball", MB_OK);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
DispatchNotifyMediaFormatChange(VOID)
{
	return TRUE;
}

static
BOOL
DispatchNotifyStart(VOID)
{
	bool_t				result;
	ULONG				i;
	LPI_MEDIA_FORMAT	mf;

	// get media format
	result = SENDCMD2MGR( EPORT_CMD_QUERY_MEDIA_FORMAT, &mf );
	if( !result ) { mf.video_format.width = 320; mf.video_format.height = 240; }

	// init balls
	for(i=0; i<BALL_COUNT; i++)
	{
		// set seed
		srand( (unsigned)time( NULL ) );

		// set ball data
		m_MyLpiBlock.ball[i].pos.x	= (rand() % (mf.video_format.width  - BALL_CX)) + (BALL_CX/2);
		m_MyLpiBlock.ball[i].pos.y	= (rand() % (mf.video_format.height - BALL_CY)) + (BALL_CY/2);
		m_MyLpiBlock.ball[i].x_vel	= ((rand() % 4) + 2) * ((-1)*(i+1));
		m_MyLpiBlock.ball[i].y_vel	= ((rand() % 4) + 2) * ((-1)*(i+2));
		m_MyLpiBlock.ball[i].color	= 0xff << ((rand() % 3)*8);
	}

	return TRUE;
}

static
BOOL
DispatchNotifyPause(VOID)
{
	return TRUE;
}

static
BOOL
DispatchNotifyStop(VOID)
{
	return TRUE;
}

static
BOOL
DispatchQueryReady(
	IN	PELPI_PACKET_QUERY_READY			pInfo
	)
{
	// check parameter
	if( !pInfo ) { return FALSE; }

	// set flag
	pInfo->ready = TRUE;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
static
VOID
MoveBall(
	IN	HDC									hDC,
	IN	LONG								Width,
	IN	LONG								Height,
	IN	PBALL_DESC							pBallDesc
	)
{
	HBRUSH		hbr = NULL;
	HBRUSH		holdbr = NULL;

	RECT		ball_rect;
	RECT		screen_rect;
	RECT		subs_rect;

	// init
	SetRect( 
		&ball_rect, 
		pBallDesc->pos.x,
		pBallDesc->pos.y,
		pBallDesc->pos.x + BALL_CX,
		pBallDesc->pos.y + BALL_CY
		);
	OffsetRect( &ball_rect, -(BALL_CX/2), -(BALL_CY/2) );
	SetRect( &screen_rect, 0, 0, Width, Height );

	// is out of range?
	SubtractRect( &subs_rect, &ball_rect, &screen_rect );
	if( !IsRectEmpty( &subs_rect ) )
	{
		// reset vel
		if( (subs_rect.left  < 0     && pBallDesc->x_vel < 0) ||
			(subs_rect.right > Width && pBallDesc->x_vel > 0) )
		{
			pBallDesc->x_vel = -pBallDesc->x_vel;
		}
		if( (subs_rect.top    < 0      && pBallDesc->y_vel < 0 ) ||
			(subs_rect.bottom > Height && pBallDesc->y_vel > 0 ) )
		{
			pBallDesc->y_vel = -pBallDesc->y_vel;
		}

		// change color
		pBallDesc->color <<= 8;
		pBallDesc->color &= 0x00ffffff;
		if( !pBallDesc->color ) { pBallDesc->color = 0xff; };
	}

	// move ball
	OffsetRect( &ball_rect, pBallDesc->x_vel, pBallDesc->y_vel );

	// create a brush
	hbr = CreateSolidBrush( pBallDesc->color );
	holdbr = (HBRUSH)SelectObject( hDC, hbr );

	// draw
	Ellipse( hDC, ball_rect.left, ball_rect.top, ball_rect.right, ball_rect.bottom );

	// save pos
	OffsetRect( &ball_rect, (BALL_CX/2), (BALL_CY/2) );
	pBallDesc->pos.x = ball_rect.left;
	pBallDesc->pos.y = ball_rect.top;

	// rollup
	SelectObject( hDC, holdbr );	holdbr = NULL;
	DeleteObject( hbr );			hbr = NULL;
}

static
BOOL
DispatchProcessMediaData(
	IN	PELPI_PACKET_PROCESS_MEDIA_DATA		pMediaData
	)
{
	HDC			hdc = NULL;
	HDC			hmemdc = NULL;
	HBITMAP		hbmp = NULL;
	HBITMAP		holdbmp = NULL;
	BITMAPINFO	bi;
	ULONG		i;

	// check parameter
	if( !pMediaData ) { return FALSE; }

	// is audio data?
	if( pMediaData->is_audio_data ) { return TRUE; }

	// set dc
	hdc		= GetDC( GetDesktopWindow() );
	hmemdc	= CreateCompatibleDC( hdc );
	hbmp	= CreateCompatibleBitmap( 
		hdc, 
		pMediaData->video_data_desc.width, 
		pMediaData->video_data_desc.height
		);
	holdbmp	= (HBITMAP)SelectObject( hmemdc, hbmp );

	// make bitmap info
	memset( &bi, 0, sizeof(BITMAPINFO) );
	bi.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth		= pMediaData->video_data_desc.width;
	bi.bmiHeader.biHeight		= pMediaData->video_data_desc.height;
	bi.bmiHeader.biPlanes		= 1;
	bi.bmiHeader.biBitCount		= VIDEO_DEPTH;
	bi.bmiHeader.biCompression	= BI_RGB;
	bi.bmiHeader.biSizeImage	= (
		pMediaData->video_data_desc.width  * 
		pMediaData->video_data_desc.height * 
		(VIDEO_DEPTH/8)
		);

	// set dib bits
	SetDIBits(
		hmemdc,
		hbmp,
		0,
		pMediaData->video_data_desc.height,
		pMediaData->pt_data,
		&bi,
		DIB_RGB_COLORS
		);

	// print!
	for(i=0; i<BALL_COUNT; i++)
	{
		MoveBall( 
			hmemdc, 
			pMediaData->video_data_desc.width, 
			pMediaData->video_data_desc.height,
			m_MyLpiBlock.ball + i
			);
	}

	// copy
	BitBlt(
		hdc,
		0,
		0,
		pMediaData->video_data_desc.width,
		pMediaData->video_data_desc.height,
		hmemdc,
		0,
		0,
		SRCCOPY
		);

	// get dib bits
	GetDIBits(
		hmemdc, 
		hbmp, 
		0, 
		pMediaData->video_data_desc.height, 
		pMediaData->pt_data, 
		&bi, 
		DIB_RGB_COLORS 
		);

	// rolling up
	SelectObject( hmemdc, holdbmp );
	DeleteObject( hbmp );
	DeleteDC( hmemdc );
	ReleaseDC( GetDesktopWindow(), hdc );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
BALL_API
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
		//
		// -- basic commands --
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
		// -- ELPI Specific commands --
	case ELPI_CMD_NOTIFY_MEDIA_FORMAT_CHANGE:
		{
			result = DispatchNotifyMediaFormatChange();
		}
		break;

	case ELPI_CMD_NOTIFY_START:
		{
			result = DispatchNotifyStart();
		}
		break;

	case ELPI_CMD_NOTIFY_PAUSE:
		{
			result = DispatchNotifyPause();
		}
		break;

	case ELPI_CMD_NOTIFY_STOP:
		{
			result = DispatchNotifyStop();
		}
		break;

	case ELPI_CMD_QUERY_READY:
		{
			result = DispatchQueryReady(
				(PELPI_PACKET_QUERY_READY)pContext
				);
		}
		break;

	case ELPI_CMD_PROCESS_MEDIA_DATA:
		{
			result = DispatchProcessMediaData(
				(PELPI_PACKET_PROCESS_MEDIA_DATA)pContext
				);
		}
		break;

		//
		// -- invalid commands --
	default:
		result = FALSE;
		break;
	}

	return (bool_t)result;
}

////////////////////////////////////////////////////////////////////////////////
BALL_API
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

	// nullify
	memset( &m_MyLpiBlock, 0, sizeof(m_MyLpiBlock) );
	m_MyLpiBlock.is_loaded = FALSE;

	return TRUE;
}

BALL_API
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
	m_MyLpiBlock.pt_mgr_dispatch	= pLpiMgrDispatch;
	m_MyLpiBlock.pt_mgr_context		= pLpiMgrContext;

	return (lpi_handle_t)&m_MyLpiBlock;
}

////////////////////////////////////////////////////////////////////////////////
BALL_API
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
	pLpiID->guid			= BALLLPI_GUID;
	strcpy( pLpiID->name, MYLPI_NAME );
	strcpy( pLpiID->developer, MYLPI_DEVELOPER );
	pLpiID->ver				= MYLPI_VER;
	pLpiID->type			= LPI_TYPE_ELPI;
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
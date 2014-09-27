//
// 아프리카 개발팀 : 2006년 9월 8일
// ---
// 9월14일 - 귓속말 처리 루틴 추가
//

//
// (1) BJ가 일반 메시지를 보냈을 경우 flow
// ---------------------------------------
// CLPI_TCM_SEND -> CLPI_TCM_RECV
//
// (2) BJ가 귓속말을 보냈을 경우 flow
// ----------------------------------
// CLPI_TCM_SEND -> CLPI_TCM_RECV_WHISPER
//
// (3) CLPI가 일반 메시지를 보냈을 경우 flow
// -----------------------------------------
// CPORT_CMD_SEND_CHAT_MSG -> CLPI_TCM_SEND -> CLPI_TCM_RECV
//
// (4) CLPI가 귓속말을 보냈을 경우 flow
// ------------------------------------
// CPORT_CMD_SEND_CHAT_MSG -> CLPI_TCM_SEND -> CLPI_TCM_RECV_WHISPER
//


#include "stdafx.h"
#include "Greeting.h"

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

WSADATA wsaData;

/*
 * plugin guid
 */

static
const
GUID
/*GREETINGLPI_GUID = { // {0975C3BE-FD60-4978-861E-F003018B2B84}
	0x975c3be, 
	0xfd60, 
	0x4978, 
	{ 0x86, 0x1e, 0xf0, 0x3, 0x1, 0x8b, 0x2b, 0x84 }
};*/

GREETINGLPI_GUID = { 
	0x80cb3326, 0x4d1c, 0x43c6, 0x9e, 0x56, 0xaa, 0xbc, 0x2f, 0xbc, 0xeb, 0xb4
};

/*
 * definition
 */
#define MYLPI_NAME					"Greeting 플러그인"
#define MYLPI_DEVELOPER				"아프리카 개발팀"
#define MYLPI_COMMENTS				"예제 플러그인"
#define MYLPI_VER					MAKE_LVERSION(1,0,0,0)
#define DEFAULT_BUFLEN 512

#define DEFAULT_PORT "27015"
SOCKET ConnectSocket = INVALID_SOCKET;

/*
 * macro function
 */
#define GET_BLOCK(handle)			((PMY_LPI_BLOCK)handle)
#define CHECK_LPI_HANDLE(lpi)		{ if((u32_t)lpi != (u32_t)&m_MyLpiBlock) return FALSE; }

#define SENDCMD2MGR(cmd, pt_param)	\
	m_MyLpiBlock.pt_mgr_dispatch( m_MyLpiBlock.pt_mgr_context, (lpi_handle_t)&m_MyLpiBlock, cmd, pt_param )

bool InitWinsock();


/*
 * structure
 */
typedef struct _MY_LPI_BLOCK {
	BOOL							is_loaded;

	// lpi manager dispatch routine
	PLPIMGR_DISPATCH				pt_mgr_dispatch;
	PVOID							pt_mgr_context;

	// bj id
	char							bj_id[32];
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
	MessageBox(NULL, "지원하지 않는 기능입니다.", "Greeting", MB_OK);

	return TRUE;
}

static
BOOL
DispatchHideStudio(VOID)
{
	MessageBox(NULL, "지원하지 않는 기능입니다.", "Greeting", MB_OK);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
DispatchNotifyStart(VOID)
{
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
	IN	PCLPI_PACKET_QUERY_READY			pInfo
	)
{
	// check parameter
	if( !pInfo ) { return FALSE; }

	// set flag
	pInfo->ready = TRUE;

	return TRUE;
}

bool SendTest();

void ProcessChatMsg(const char* msg)
{
	// parse chat message.
	
	// commands
	// ! => input
	//      something

	//SendTest();
	// command start 
	// !input c
	//if ( msg[0] == '!' )
	{		
		int recvbuflen = DEFAULT_BUFLEN;

		char *sendbuf = "this is a test";
		
		int iResult;

		// Send an initial buffer
		iResult = send(ConnectSocket, msg, (int) strlen(msg), 0);

		//MessageBox( NULL, msg, "데이터 보냄...", MB_OK );
		if (iResult == SOCKET_ERROR) 
		{
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();			
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
static
BOOL
DispatchProcessChatMsg(
	IN	PCLPI_PACKET_PROCESS_CHAT_MSG	pChatMsg
	)
{
	CPORT_PACKET_SEND_CHAT_MSG	msg;

	// dispatch
	// 
	// 아프리카 스튜디오가 채팅 서버로부터 전달받은 채팅메시지를 
	// 처리하는 부분. 처리 규칙은 다음과 같다.
	//
	// (1) 서버로 부터 전달받은 메세지인지
	// (2) BJ가 입력한 채팅메시지가 서버로부터 에코(echo)되어 되돌아
	//     오는것이 아닌지
	//
	// 위의 2가지 조건이 맞아야만 다음 메시지를 처리한다.
	//if( !stricmp(m_MyLpiBlock.bj_id, pChatMsg->id) ) { return TRUE; }	// <-- (2)
	if( pChatMsg->type == CLPI_TCM_RECV ) // <-- (1)
	{
		// echo test.
		// it works!!
		/*memset( &msg, 0, sizeof(msg) );
		strcpy( msg.msg, pChatMsg->pt_msg );
		SENDCMD2MGR( CPORT_CMD_SEND_CHAT_MSG, &msg );*/
		
		//MessageBox(NULL, pChatMsg->pt_msg, "dd",MB_OK );
		ProcessChatMsg(pChatMsg->pt_msg);

		//
		// 시청자가 인사말을 건냈을 경우 자동 인사말 건네기
		//
		/*if( strstr( pChatMsg->pt_msg, "안녕하세요" ) )
		{
			// build msg
			memset( &msg, 0, sizeof(msg) );
			sprintf(
				msg.msg,
				"네. 안녕하세요 %s님. ^^",
				pChatMsg->nick
				);

			// send
			SENDCMD2MGR( CPORT_CMD_SEND_CHAT_MSG, &msg );
		}*/
		//
		// 시청자가 욕을 하였을 경우 자동 강퇴 시키기
		//
		/*else if( strstr( pChatMsg->pt_msg, "시발" ) )
		{
			// build msg
			memset( &msg, 0, sizeof(msg) );
			sprintf(
				msg.msg,
				"/k %s",
				pChatMsg->id
				);

			// send
			SENDCMD2MGR( CPORT_CMD_SEND_CHAT_MSG, &msg );
		}*/
		//
		// 시청자가 염장을 지를경우 채팅창에 표시하지 않기
		//
		/*else if( strstr( pChatMsg->pt_msg, "여친" ) )
		{
			// skip
			pChatMsg->skip = TRUE;
		}*/
	}
	else if( pChatMsg->type == CLPI_TCM_RECV_WHISPER )
	{
		//
		// 시청자가 귓속말을 건냈을 경우... 자제 요청 하기
		//

		// build msg
		memset( &msg, 0, sizeof(msg) );
		sprintf(
			msg.msg,
			"/to %s 저 귓속말 안받겄든요. 자제해 주세요!!",
			pChatMsg->id
			);

		// send
		SENDCMD2MGR( CPORT_CMD_SEND_CHAT_MSG, &msg );
	}

	return TRUE;
}

static
BOOL
DispatchNotifyChatChange(
	IN	PCLPI_PACKET_NOTIFY_CHAT_CHANGE	pChatChange
	)
{
	CPORT_PACKET_SEND_CHAT_MSG	msg;

	// dispatch
	if( pChatChange->type == CLPI_TCN_CREATE )
	{
		//
		// 채팅방이 Create되면 BJ의 아이디를 미리 저장해 놓는다.
		//
		strcpy( m_MyLpiBlock.bj_id, pChatChange->id );
	}
	else if( pChatChange->type == CLPI_TCN_INCOMING )
	{
		//
		// 새로운 시청자가 들어오면 인사말을 건넨다.
		//

		// build msg
		/*memset( &msg, 0, sizeof(msg) );
		sprintf(
			msg.msg,
			"%s님 어서오세요. ^^",
			pChatChange->nick
			);
			
		// send
		SENDCMD2MGR( CPORT_CMD_SEND_CHAT_MSG, &msg );*/
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
GREETING_API
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
		// -- CLPI Specific commands --
	case CLPI_CMD_NOTIFY_START:
		{
			result = DispatchNotifyStart();
		}
		break;

	case CLPI_CMD_NOTIFY_PAUSE:
		{
			result = DispatchNotifyPause();
		}
		break;

	case CLPI_CMD_NOTIFY_STOP:
		{
			result = DispatchNotifyStop();
		}
		break;

	case CLPI_CMD_QUERY_READY:
		{
			result = DispatchQueryReady(
				(PCLPI_PACKET_QUERY_READY)pContext
				);
		}
		break;

	case CLPI_CMD_PROCESS_CHAT_MSG:
		{
			result = DispatchProcessChatMsg(
				(PCLPI_PACKET_PROCESS_CHAT_MSG)pContext
				);
		}
		break;

	case CLPI_CMD_NOTIFY_CHAT_CHANGE:
		{
			result = DispatchNotifyChatChange(
				(PCLPI_PACKET_NOTIFY_CHAT_CHANGE)pContext
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
GREETING_API
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

GREETING_API
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

	if ( InitWinsock() )
		{		
			SendTest();

			Sleep(1000);
			SendTest();
		}

	return (lpi_handle_t)&m_MyLpiBlock;
}

////////////////////////////////////////////////////////////////////////////////
GREETING_API
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
	pLpiID->guid			= GREETINGLPI_GUID;
	strcpy( pLpiID->name, MYLPI_NAME );
	strcpy( pLpiID->developer, MYLPI_DEVELOPER );
	pLpiID->ver				= MYLPI_VER;
	pLpiID->type			= LPI_TYPE_CLPI;
	strcpy( pLpiID->comments, MYLPI_COMMENTS );

	return TRUE;
}



bool InitWinsock()
{
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) 
	{
		//printf("WSAStartup failed: %d\n", iResult);
		return FALSE;
	}

	struct addrinfo *result = NULL,
            *ptr = NULL,
            hints;

	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	

	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return false;
	}
		
	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr=result;

	// Create a SOCKET for connecting to server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
		ptr->ai_protocol);
	
	if (ConnectSocket == INVALID_SOCKET) 
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}


	// Connect to server.
	iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return false;
	}
	



	return true;
}



bool SendTest()
{
	int recvbuflen = DEFAULT_BUFLEN;

	char *sendbuf = "this is a test";
	char recvbuf[DEFAULT_BUFLEN];

	int iResult;

	// Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int) strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return false;
	}
	
	//printf("Bytes Sent: %ld\n", iResult);

	// shutdown the connection for sending since no more data will be sent
	// the client can still use the ConnectSocket for receiving data
	/*iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return false;
	}*/

	return true;
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


		// Initialize winsock.	
		/*if ( InitWinsock() )
		{		
			SendTest();

			Sleep(1000);
			SendTest();
		}*/
	}

    return TRUE;
}
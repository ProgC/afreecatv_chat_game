//
// ������ī ������ : 2006�� 9�� 8��
// ---
// 9��14�� - �ӼӸ� ó�� ��ƾ �߰�
//

//
// (1) BJ�� �Ϲ� �޽����� ������ ��� flow
// ---------------------------------------
// CLPI_TCM_SEND -> CLPI_TCM_RECV
//
// (2) BJ�� �ӼӸ��� ������ ��� flow
// ----------------------------------
// CLPI_TCM_SEND -> CLPI_TCM_RECV_WHISPER
//
// (3) CLPI�� �Ϲ� �޽����� ������ ��� flow
// -----------------------------------------
// CPORT_CMD_SEND_CHAT_MSG -> CLPI_TCM_SEND -> CLPI_TCM_RECV
//
// (4) CLPI�� �ӼӸ��� ������ ��� flow
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
#define MYLPI_NAME					"Greeting �÷�����"
#define MYLPI_DEVELOPER				"������ī ������"
#define MYLPI_COMMENTS				"���� �÷�����"
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
	MessageBox(NULL, "�������� �ʴ� ����Դϴ�.", "Greeting", MB_OK);

	return TRUE;
}

static
BOOL
DispatchHideStudio(VOID)
{
	MessageBox(NULL, "�������� �ʴ� ����Դϴ�.", "Greeting", MB_OK);

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

		//MessageBox( NULL, msg, "������ ����...", MB_OK );
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
	// ������ī ��Ʃ����� ä�� �����κ��� ���޹��� ä�ø޽����� 
	// ó���ϴ� �κ�. ó�� ��Ģ�� ������ ����.
	//
	// (1) ������ ���� ���޹��� �޼�������
	// (2) BJ�� �Է��� ä�ø޽����� �����κ��� ����(echo)�Ǿ� �ǵ���
	//     ���°��� �ƴ���
	//
	// ���� 2���� ������ �¾ƾ߸� ���� �޽����� ó���Ѵ�.
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
		// ��û�ڰ� �λ縻�� �ǳ��� ��� �ڵ� �λ縻 �ǳױ�
		//
		/*if( strstr( pChatMsg->pt_msg, "�ȳ��ϼ���" ) )
		{
			// build msg
			memset( &msg, 0, sizeof(msg) );
			sprintf(
				msg.msg,
				"��. �ȳ��ϼ��� %s��. ^^",
				pChatMsg->nick
				);

			// send
			SENDCMD2MGR( CPORT_CMD_SEND_CHAT_MSG, &msg );
		}*/
		//
		// ��û�ڰ� ���� �Ͽ��� ��� �ڵ� ���� ��Ű��
		//
		/*else if( strstr( pChatMsg->pt_msg, "�ù�" ) )
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
		// ��û�ڰ� ������ ������� ä��â�� ǥ������ �ʱ�
		//
		/*else if( strstr( pChatMsg->pt_msg, "��ģ" ) )
		{
			// skip
			pChatMsg->skip = TRUE;
		}*/
	}
	else if( pChatMsg->type == CLPI_TCM_RECV_WHISPER )
	{
		//
		// ��û�ڰ� �ӼӸ��� �ǳ��� ���... ���� ��û �ϱ�
		//

		// build msg
		memset( &msg, 0, sizeof(msg) );
		sprintf(
			msg.msg,
			"/to %s �� �ӼӸ� �ȹްε��. ������ �ּ���!!",
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
		// ä�ù��� Create�Ǹ� BJ�� ���̵� �̸� ������ ���´�.
		//
		strcpy( m_MyLpiBlock.bj_id, pChatChange->id );
	}
	else if( pChatChange->type == CLPI_TCN_INCOMING )
	{
		//
		// ���ο� ��û�ڰ� ������ �λ縻�� �ǳٴ�.
		//

		// build msg
		/*memset( &msg, 0, sizeof(msg) );
		sprintf(
			msg.msg,
			"%s�� �������. ^^",
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
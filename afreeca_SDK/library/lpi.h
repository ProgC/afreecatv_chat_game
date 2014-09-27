#ifndef _LIVEPLUGIN_SUPPORT_LIBRARY_HEADER_FILE_
#define _LIVEPLUGIN_SUPPORT_LIBRARY_HEADER_FILE_


/*
 * compile option
 */
#pragma pack(push, 4)


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
 * types
 */
typedef char						s8_t;
typedef unsigned char				u8_t;
typedef short						s16_t;
typedef unsigned short				u16_t;
typedef long						s32_t;
typedef unsigned long				u32_t;
typedef __int64						s64_t;
typedef unsigned __int64			u64_t;

typedef s32_t						lpi_handle_t;
typedef s32_t						bool_t;


/*
 * macro function
 */
#define MAKE_LVERSION(a,b,c,d)		\
	( ((a&0xff)<<24) | ((b&0xff)<<16) | ((c&0xff)<<8) | (d&0xff) )


/*
 * version info
 */
#define LPI_LIB_VER					MAKE_LVERSION(1, 1, 1, 0)


/*
 * plugin type
 */
typedef unsigned long				LPI_TYPE;
#define LPI_TYPE_DLPI				0x01000000	// device lpi
#define LPI_TYPE_ELPI				0x02000000	// effect lpi
#define LPI_TYPE_CLPI				0x04000000	// chatting lpi


/*
 * function prototype
 */
typedef bool_t (__cdecl *PLPIMGR_DISPATCH)(		// lpi -> mgr
	IN	void						*pLpiMgrCtx,
	IN	lpi_handle_t				LpiHandle,
	IN	u32_t						Command,
	IN	void						*pContext
	);


/*
 * structure
 */
typedef struct _LPI_IDENITIFICATION_INFO {
	// library info
	OUT	u32_t						lib_ver;

	// plugin info
	OUT	GUID						guid;
	OUT	char						name[64];
	OUT	char						developer[64];
	OUT	u32_t						ver;
	OUT	LPI_TYPE					type;
	OUT	char						comments[1024];
} LPI_IDENITIFICATION_INFO, *PLPI_IDENITIFICATION_INFO;


/*
 * lpi entry points
 */
#define LPIFNN_IDENTIFY			"LPI_Identify"
#define LPIFNN_CREATE			"LPI_Create"
#define LPIFNN_DISPATCH			"LPI_Dispatch"
#define LPIFNN_CLOSE			"LPI_Close"

typedef bool_t (__cdecl *PLPI_IDENTIFY)(
	OUT		PLPI_IDENITIFICATION_INFO	pLpiID
	);
typedef lpi_handle_t (__cdecl *PLPI_CREATE)(
	IN		PLPIMGR_DISPATCH			pLpiMgrDispatch,
	IN		void						*pLpiMgrCtx
	);
typedef bool_t (__cdecl *PLPI_DISPATCH)(
	IN		lpi_handle_t				LpiHandle,
	IN		u32_t						Command,
	IN OUT	void						*pContext
	);
typedef bool_t (__cdecl *PLPI_CLOSE)(
	IN		lpi_handle_t				LpiHandle
	);




////////////////////////////////////////////////////////////////////////////////
//                    LPI SUPPORT LIBRARY COMMON DEFINITION                   //
////////////////////////////////////////////////////////////////////////////////
// colorspace
typedef unsigned long				LPI_COLORSPACE;
typedef LPI_COLORSPACE				*PLPI_COLORSPACE;
//
#define LCS_I420					(1<< 1)
#define LCS_YV12					(1<< 2)
#define LCS_YUY2					(1<< 3)
#define LCS_UYVY					(1<< 4)
#define LCS_BGRA					(1<< 6) // rgb32
#define LCS_BGR						(1<< 9) // rgb24
#define LCS_RGB555					(1<<10)
#define LCS_UNKNOWN					0xffffffff


// media format structures
typedef struct _LPI_AUDIO_FORMAT {
	u16_t							channels;
	u16_t							bits_per_sample;
	u32_t							samples_per_sec;
} LPI_AUDIO_FORMAT, *PLPI_AUDIO_FORMAT;

typedef struct _LPI_VIDEO_FORMAT {
	s16_t							width;
	s16_t							height;
	u16_t							bits_per_pixel;
	LPI_COLORSPACE					colorspace;
	float							framerate;
} LPI_VIDEO_FORMAT, *PLPI_VIDEO_FORMAT;

typedef struct _LPI_MEDIA_FORMAT {
	LPI_AUDIO_FORMAT				audio_format;
	LPI_VIDEO_FORMAT				video_format;
} LPI_MEDIA_FORMAT, *PLPI_MEDIA_FORMAT;

typedef struct _NLB_RESOLUTION_PAIR {
	u16_t							width;
	u16_t							height;
} LPI_RESOLUTION_PAIR, *PLPI_RESOLUTION_PAIR;


// data descriptors
typedef struct _LPI_AUDIO_DATA_DESC {
	// basic audio info
	u16_t							channels;
	u16_t							bits_per_sample;
	u32_t							samples_per_sec;

	// data size
	u32_t							data_size;

	// time info
	s64_t							start_time;
	s64_t							end_time;
} LPI_AUDIO_DATA_DESC, *PLPI_AUDIO_DATA_DESC;

typedef struct _LPI_VIDEO_DATA_DESC {
	// basic video info
	s16_t							width;
	s16_t							height;
	u16_t							bits_per_pixel;
	LPI_COLORSPACE					colorspace;
	float							framerate;

	// data size
	u32_t							data_size;

	// time info
	s64_t							start_time;
	s64_t							end_time;
} LPI_VIDEO_DATA_DESC, *PLPI_VIDEO_DATA_DESC;

//
typedef struct _LPI_MEDIA_DATA {
	IN	bool_t						is_audio_data;
	union
	{
		IN	LPI_AUDIO_DATA_DESC		audio_data_desc;
		IN	LPI_VIDEO_DATA_DESC		video_data_desc;
	};
	IN	void						*pt_data; // in/out
} LPI_MEDIA_DATA, *PLPI_MEDIA_DATA;




////////////////////////////////////////////////////////////////////////////////
//                         LPI COMMON COMMAND DEFINITION                      //
////////////////////////////////////////////////////////////////////////////////
// common interfaces for lpi
#define LPI_CMD_NOTIFY_INIT_COMPLETE	0		// param : NULL
#define LPI_CMD_GET_UPDATE_INFO			10
#define LPI_CMD_GET_UNINSTALLER_INFO	11

#define LPI_CMD_SHOW_STUDIO				20		// param : NULL
#define LPI_CMD_HIDE_STUDIO				21		// param : NULL


// structures for lpi
typedef struct _LPI_PACKET_GET_UPDATE_INFO {
	OUT	bool_t						has_updated_ver;

	OUT	bool_t						is_url;
	union
	{
		OUT	char					url_path[1024]; // fullpath
		OUT	char					exe_path[1024]; // fullpath
	};
} LPI_PACKET_GET_UPDATE_INFO, *PLPI_PACKET_GET_UPDATE_INFO;

typedef struct _LPI_PACKET_GET_UNINSTALLER_INFO {
	OUT	bool_t						has_uninstaller;
	OUT	char						exe_path[1024];
} LPI_PACKET_GET_UNINSTALLER_INFO, *PLPI_PACKET_GET_UNINSTALLER_INFO;




////////////////////////////////////////////////////////////////////////////////
//                            DLPI COMMAND DEFINITION                         //
////////////////////////////////////////////////////////////////////////////////
// interfaces for dlpi
#define DLPI_CMD_START					(LPI_TYPE_DLPI + 0)		// param : NULL
#define DLPI_CMD_PAUSE					(LPI_TYPE_DLPI + 1)		// param : NULL
#define DLPI_CMD_STOP					(LPI_TYPE_DLPI + 2)		// param : NULL

#define DLPI_CMD_SET_VOLUME				(LPI_TYPE_DLPI + 10)
#define DLPI_CMD_GET_VOLUME				(LPI_TYPE_DLPI + 11)
#define DLPI_CMD_UPDATE_VIDEO_SIZE		(LPI_TYPE_DLPI + 12)	// param : NULL
#define DLPI_CMD_SET_FULLSCREEN			(LPI_TYPE_DLPI + 13)
#define DLPI_CMD_QUERY_FULLSCREEN		(LPI_TYPE_DLPI + 14)


// structures for dlpi
typedef struct _DLPI_PACKET_SET_VOLUME {
	IN	u32_t						volume;	// 0~100
} DLPI_PACKET_SET_VOLUME, *PDLPI_PACKET_SET_VOLUME;

typedef struct _DLPI_PACKET_GET_VOLUME {
	OUT	u32_t						volume;	// 0~100
} DLPI_PACKET_GET_VOLUME, *PDLPI_PACKET_GET_VOLUME;

typedef struct _DLPI_PACKET_SET_FULLSCREEN {
	IN	bool_t						fullscreen_mode;
} DLPI_PACKET_SET_FULLSCREEN, *PDLPI_PACKET_SET_FULLSCREEN;

typedef struct _DLPI_PACKET_QUERY_FULLSCREEN {
	OUT	bool_t						fullscreen_mode;
} DLPI_PACKET_QUERY_FULLSCREEN, *PDLPI_PACKET_QUERY_FULLSCREEN;




////////////////////////////////////////////////////////////////////////////////
//                            ELPI COMMAND DEFINITION                         //
////////////////////////////////////////////////////////////////////////////////
// interfaces for elpi
#define ELPI_CMD_NOTIFY_MEDIA_FORMAT_CHANGE		(LPI_TYPE_ELPI + 0)

#define ELPI_CMD_NOTIFY_START					(LPI_TYPE_ELPI + 1)
#define ELPI_CMD_NOTIFY_PAUSE					(LPI_TYPE_ELPI + 2)
#define ELPI_CMD_NOTIFY_STOP					(LPI_TYPE_ELPI + 3)

#define ELPI_CMD_QUERY_READY					(LPI_TYPE_ELPI + 10)
#define ELPI_CMD_PROCESS_MEDIA_DATA				(LPI_TYPE_ELPI + 20)


// structures for elpi
typedef LPI_MEDIA_FORMAT			ELPI_PACKET_NOTIFY_MEDIA_FORMAT_CHANGE;
typedef LPI_MEDIA_FORMAT			*PELPI_PACKET_NOTIFY_MEDIA_FORMAT_CHANGE;

typedef struct _ELPI_PACKET_QUERY_READY {
	OUT	bool_t						ready;
} ELPI_PACKET_QUERY_READY, *PELPI_PACKET_QUERY_READY;

typedef LPI_MEDIA_DATA				ELPI_PACKET_PROCESS_MEDIA_DATA;
typedef LPI_MEDIA_DATA				*PELPI_PACKET_PROCESS_MEDIA_DATA;




////////////////////////////////////////////////////////////////////////////////
//                            CLPI COMMAND DEFINITION                         //
////////////////////////////////////////////////////////////////////////////////
// interfaces for clpi
#define CLPI_CMD_NOTIFY_START					(LPI_TYPE_CLPI + 0)
#define CLPI_CMD_NOTIFY_PAUSE					(LPI_TYPE_CLPI + 1)
#define CLPI_CMD_NOTIFY_STOP					(LPI_TYPE_CLPI + 2)

#define CLPI_CMD_QUERY_READY					(LPI_TYPE_CLPI + 10)

#define CLPI_CMD_PROCESS_CHAT_MSG				(LPI_TYPE_CLPI + 20)
#define CLPI_CMD_NOTIFY_CHAT_CHANGE				(LPI_TYPE_CLPI + 21)


// type chat msg(TCM)
#define CLPI_TCM_SEND							(0x0000 + 0) // studio -> server
#define CLPI_TCM_RECV							(0x0000 + 1) // server -> studio
#define CLPI_TCM_RECV_WHISPER					(0x0000 + 2) // server -> studio

// type chat notify(TCN)
#define CLPI_TCN_CREATE							(0x0100 + 0)
#define CLPI_TCN_INCOMING						(0x0100 + 1)
#define CLPI_TCN_OUTGOING						(0x0100 + 2)
#define CLPI_TCN_DUMB							(0x0100 + 3)
#define CLPI_TCN_BAN							(0x0100 + 4)
#define CLPI_TCN_DUMB_BAN						(0x0100 + 5)
#define CLPI_TCN_OVERCHAT_BAN					(0x0100 + 6)
#define CLPI_TCN_MANAGER_APPOINT				(0x0100 + 7)
#define CLPI_TCN_MANAGER_DISMISS				(0x0100 + 8)
#define CLPI_TCN_NICK_CHANGE					(0x0100 + 9)


// structures for clpi
typedef struct _CLPI_PACKET_QUERY_READY {
	OUT	bool_t						ready;
} CLPI_PACKET_QUERY_READY, *PCLPI_PACKET_QUERY_READY;

typedef struct _CLPI_PACKET_PROCESS_CHAT_MSG {
	IN		u32_t					type;
	IN		char					id[32];
	IN		char					nick[32];
	IN	OUT	char					*pt_msg;
	//
	OUT		bool_t					skip;
} CLPI_PACKET_PROCESS_CHAT_MSG, *PCLPI_PACKET_PROCESS_CHAT_MSG;

typedef struct _CLPI_PACKET_NOTIFY_CHAT_CHANGE {
	IN		u32_t					type;
	IN		char					id[32];
	IN		char					nick[32];
} CLPI_PACKET_NOTIFY_CHAT_CHANGE, *PCLPI_PACKET_NOTIFY_CHAT_CHANGE;




////////////////////////////////////////////////////////////////////////////////
//                       LPI MANAGER COMMAND DEFINITION                       //
////////////////////////////////////////////////////////////////////////////////
// common interfaces for lpi
#define PORT_CMD_GET_AFREECA_INFO			0


/*
 * common cmd packet structures
 */
typedef u32_t								AFREECA_TYPE;
#define AFREECA_TYPE_STUDIO					0
#define AFREECA_TYPE_PLAYER					1

typedef struct _PORT_PACKET_GET_AFREECA_INFO {
	// version
	OUT	AFREECA_TYPE				type;
	OUT	u32_t						ver;

	// window handles
	OUT	HWND						main_hwnd;
	OUT	HWND						video_hwnd;

	// path
	OUT	char						path[1024];	// c:\afreeca
} PORT_PACKET_GET_AFREECA_INFO, *PPORT_PACKET_GET_AFREECA_INFO;




////////////////////////////////////////////////////////////////////////////////
//                       DLPI MANAGER COMMAND DEFINITION                      //
////////////////////////////////////////////////////////////////////////////////
// specific interfaces for dlpi
#define DPORT_CMD_QUERY_MEDIA_FORMAT_LIST		(LPI_TYPE_DLPI + 0)
#define DPORT_CMD_SET_MEDIA_FORMAT				(LPI_TYPE_DLPI + 1)
#define DPORT_CMD_QUERY_MEDIA_FORMAT			(LPI_TYPE_DLPI + 2)
#define DPORT_CMD_CANCEL_MEDIA_FORMAT			(LPI_TYPE_DLPI + 3)		// param : NULL

#define DPORT_CMD_READY_TO_START				(LPI_TYPE_DLPI + 10)	// param : NULL
#define DPORT_CMD_CANCEL_READY					(LPI_TYPE_DLPI + 11)	// param : NULL

#define DPORT_CMD_PROCESS_MEDIA_DATA			(LPI_TYPE_DLPI + 20)


/*
 * packet structures for dlpi
 */
typedef struct _DPORT_PACKET_QUERY_MEDIA_FORMAT_LIST {
	// lpimgr allocate
	// 0 = end of list
	struct
	{
		OUT	u16_t					*pt_ch_list;  // channel
		OUT	u16_t					*pt_bps_list; // bits per sample
		OUT	u16_t					*pt_sps_list; // samples per sec
	} audio;

	struct
	{
		OUT	PLPI_RESOLUTION_PAIR	pt_res_list;  // resolution
		OUT	PLPI_COLORSPACE			pt_cs_list;   // colorspace
		OUT	float					*pt_fr_list;  // framerate
	} video;
} DPORT_PACKET_QUERY_MEDIA_FORMAT_LIST, *PDPORT_PACKET_QUERY_MEDIA_FORMAT_LIST;

typedef LPI_MEDIA_FORMAT			DPORT_PACKET_SET_MEDIA_FORMAT;
typedef LPI_MEDIA_FORMAT			*PDPORT_PACKET_SET_MEDIA_FORMAT;

typedef LPI_MEDIA_FORMAT			DPORT_PACKET_QUERY_MEDIA_FORMAT;
typedef LPI_MEDIA_FORMAT			*PDPORT_PACKET_QUERY_MEDIA_FORMAT;

typedef LPI_MEDIA_DATA				DPORT_PACKET_PROCESS_MEDIA_DATA;
typedef LPI_MEDIA_DATA				*PDPORT_PACKET_PROCESS_MEDIA_DATA;




////////////////////////////////////////////////////////////////////////////////
//                       ELPI MANAGER COMMAND DEFINITION                      //
////////////////////////////////////////////////////////////////////////////////
// specific interfaces for elpi
#define EPORT_CMD_QUERY_MEDIA_FORMAT		(LPI_TYPE_ELPI + 0)


/*
 * packet structures for elpi
 */
typedef LPI_MEDIA_FORMAT			EPORT_PACKET_QUERY_MEDIA_FORMAT;
typedef LPI_MEDIA_FORMAT			*PEPORT_PACKET_QUERY_MEDIA_FORMAT;




////////////////////////////////////////////////////////////////////////////////
//                       CLPI MANAGER COMMAND DEFINITION                      //
////////////////////////////////////////////////////////////////////////////////
// specific interfaces for clpi
#define CPORT_CMD_SEND_CHAT_MSG				(LPI_TYPE_CLPI + 0)


/*
 * packet structures for clpi
 */
typedef struct _CPORT_PACKET_SEND_CHAT_MSG {
	IN	char						msg[299+1];
} CPORT_PACKET_SEND_CHAT_MSG, *PCPORT_PACKET_SEND_CHAT_MSG;



////////////////////////////////////////////////////////////////////////////////
/*
 * compile option
 */
#pragma pack(pop)


#endif /* #ifndef _LIVEPLUGIN_SUPPORT_LIBRARY_HEADER_FILE_ */








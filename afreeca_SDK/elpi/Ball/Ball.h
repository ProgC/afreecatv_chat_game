#ifndef _BALL_LPI_HEADER_FILE_
#define _BALL_LPI_HEADER_FILE_


/*
 * definition
 */
#ifdef BALL_EXPORTS
# define BALL_API		__declspec(dllexport)
#else
# define BALL_API		__declspec(dllimport)
#endif


//
#ifdef __cplusplus
extern "C" {
#endif


/*
 * plugin intefaces
 */
BALL_API
bool_t
LPI_Identify(
	OUT		PLPI_IDENITIFICATION_INFO	pLpiID
	);

BALL_API
lpi_handle_t
LPI_Create(
	IN		PLPIMGR_DISPATCH			pLpiMgrDispatch,
	IN		void						*pLpiMgrContext
	);

BALL_API
bool_t
LPI_Dispatch(
	IN		lpi_handle_t				LpiHandle,
	IN		u32_t						Command,
	IN OUT	void						*pContext
	);

BALL_API
bool_t
LPI_Close(
	IN		lpi_handle_t				LpiHandle
	);


//
#ifdef __cplusplus
}
#endif

#endif /* #ifndef _BALL_LPI_HEADER_FILE_ */
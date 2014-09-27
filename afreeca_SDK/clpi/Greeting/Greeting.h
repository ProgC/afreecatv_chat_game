#ifndef _GREETING_LPI_HEADER_FILE_
#define _GREETING_LPI_HEADER_FILE_


/*
 * definition
 */
#ifdef GREETING_EXPORTS
# define GREETING_API		__declspec(dllexport)
#else
# define GREETING_API		__declspec(dllimport)
#endif


//
#ifdef __cplusplus
extern "C" {
#endif


/*
 * plugin intefaces
 */
GREETING_API
bool_t
LPI_Identify(
	OUT		PLPI_IDENITIFICATION_INFO	pLpiID
	);

GREETING_API
lpi_handle_t
LPI_Create(
	IN		PLPIMGR_DISPATCH			pLpiMgrDispatch,
	IN		void						*pLpiMgrContext
	);

GREETING_API
bool_t
LPI_Dispatch(
	IN		lpi_handle_t				LpiHandle,
	IN		u32_t						Command,
	IN OUT	void						*pContext
	);

GREETING_API
bool_t
LPI_Close(
	IN		lpi_handle_t				LpiHandle
	);


//
#ifdef __cplusplus
}
#endif

#endif /* #ifndef _GREETING_LPI_HEADER_FILE_ */
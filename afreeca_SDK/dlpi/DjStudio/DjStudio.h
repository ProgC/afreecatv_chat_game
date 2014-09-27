#ifndef _DJSTUDIO_LPI_HEADER_FILE_
#define _DJSTUDIO_LPI_HEADER_FILE_


/*
 * definition
 */
#ifdef DJSTUDIO_EXPORTS
# define DJSTUDIO_API __declspec(dllexport)
#else
# define DJSTUDIO_API __declspec(dllimport)
#endif


//
#ifdef __cplusplus
extern "C" {
#endif


/*
 * plugin intefaces
 */
DJSTUDIO_API
bool_t
LPI_Identify(
	OUT		PLPI_IDENITIFICATION_INFO	pLpiID
	);

DJSTUDIO_API
lpi_handle_t
LPI_Create(
	IN		PLPIMGR_DISPATCH			pLpiMgrDispatch,
	IN		void						*pLpiMgrContext
	);

DJSTUDIO_API
bool_t
LPI_Dispatch(
	IN		lpi_handle_t				LpiHandle,
	IN		u32_t						Command,
	IN OUT	void						*pContext
	);

DJSTUDIO_API
bool_t
LPI_Close(
	IN		lpi_handle_t				LpiHandle
	);


//
#ifdef __cplusplus
}
#endif


#endif /* #ifndef _DJSTUDIO_LPI_HEADER_FILE_ */



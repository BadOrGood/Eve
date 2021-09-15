
#define OPT
#define NON
#define VMEM
#define HMEM

#include <stdio.h>
#include <Windows.h>

#define VOLATILE volatile

#define LONG_PATH_MAX (32 * 1024)

#define EVEAPI __fastcall

#ifdef __cplusplus
  #define DECLSPEC_UUID(X) __declspec(uuid(X))
#endif

#ifdef _DEBUG
  #define DebugPrint(FORMAT, ...) printf("[" __FUNCTION__ "]" FORMAT "\n", __VA_ARGS__)
#else
  #define DebugPrint __noop
#endif

#define MAKELONG64(L, H) (((ULONG32)(L)) | ((ULONG64)(H) << 32))

#define LRESULT_SUCCESS 0
#define LRESULT_ERROR  -1

#define HRESULT_FAILED(X)  (ERROR_SUCCESS != (X))
#define HRESULT_SUCCESS(X) (ERROR_SUCCESS == (X))

#include "rtlp.h"

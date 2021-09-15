#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern HANDLE RtlpProcessHeap;

VOID FORCEINLINE EVEAPI RtlpInit(VOID) {
  RtlpProcessHeap = GetProcessHeap();
}

PVOID FORCEINLINE EVEAPI RtlpAlloc(
  IN SIZE_T cbBlock
  )
{
  return HeapAlloc(RtlpProcessHeap, 0, cbBlock);
}

BOOL FORCEINLINE EVEAPI RtlpFree(
  IN PVOID pBlock
  )
{
  return HeapFree(RtlpProcessHeap, 0, pBlock);
}

#ifdef __cplusplus
}
#endif

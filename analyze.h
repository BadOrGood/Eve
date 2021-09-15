#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL AnalyzeStoppage;
extern BOOL VOLATILE AnalyzeStopped;

BOOL EVEAPI AnalyzeIsExecutable(
  IN PWSTR szFilePath
  );

HRESULT EVEAPI AnalyzeInit(VOID);

ULONG WINAPI AnalyzeFileThread(
  IN PWSTR szFilePath
  );

ULONG WINAPI AnalyzeFolder(
  IN VMEM PWSTR szFolderPath
  );

ULONG WINAPI AnlyzeFullScan(
  IN OPT PWSTR szPath
  );

ULONG WINAPI AnlyzeQuickScan(
  IN PVOID pParameter
  );

#ifdef _DEBUG
  VOID EVEAPI AnalyzeTest(VOID);
#endif

#ifdef __cplusplus
}
#endif

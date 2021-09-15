#include "ianalyze.h"
#include "analyze.h"

#define SZ_IANALYZE_PIPE L"\\\\.\\pipe\\{0B7C06EB-72D6-4BA1-B585-DC067A43A797}"

VOID EVEAPI IAnalyzeSendArgument(
  IN PWSTR szPath
  )
{
  ULONG  dwIo;
  HANDLE hPipe;

  if (!AnalyzeIsExecutable(szPath)) {
    return;
  }

  if (INVALID_HANDLE_VALUE == (hPipe = CreateFileW(SZ_IANALYZE_PIPE, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL))) {
    DebugPrint("failed to connect to %ls", SZ_IANALYZE_PIPE);

    return;
  }

  if (!WriteFile(hPipe, szPath, (DWORD)((wcslen(szPath) + 1) * sizeof(WCHAR)), &dwIo, NULL)) {
    DebugPrint("failed to write to %ls", SZ_IANALYZE_PIPE);
  }

  ReadFile(hPipe, &dwIo, 1, &dwIo, NULL);

  CloseHandle(hPipe);
}

static ULONG WINAPI IAnalyzeArgumentHandler(
  IN PVOID pParameter
  )
{
  UNREFERENCED_PARAMETER(pParameter);

  ULONG  dwIo;
  HANDLE hPipe;
  PWSTR  szPath;

  if (!(szPath = VirtualAlloc(NULL, LONG_PATH_MAX * sizeof(WCHAR), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
    DebugPrint("not enough virtual memory");

    return ERROR_SUCCESS;
  }

  while (TRUE) {
    if (INVALID_HANDLE_VALUE == (hPipe = CreateNamedPipeW(SZ_IANALYZE_PIPE, PIPE_ACCESS_DUPLEX, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 1, LONG_PATH_MAX * sizeof(WCHAR), INFINITE, NULL))) {
      DebugPrint("failed to create named pipe %ls", SZ_IANALYZE_PIPE);

      return ERROR_SUCCESS;
    }

    if (!ConnectNamedPipe(hPipe, NULL)) {
      _mm_pause();

      CloseHandle(hPipe);

      continue;
    }

    if (ReadFile(hPipe, szPath, LONG_PATH_MAX * sizeof(WCHAR), &dwIo, NULL)) {
      AnalyzeFileThread(szPath);
    } else {
      DebugPrint("failed to receive from %ls", SZ_IANALYZE_PIPE);
    }

    WriteFile(hPipe, &dwIo, 1, &dwIo, NULL);

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
  }

  return ERROR_SUCCESS;
}

VOID EVEAPI IAnalyzeInit(VOID) {
  CreateThread(NULL, 0, (PVOID)IAnalyzeArgumentHandler, NULL, 0, NULL);
}

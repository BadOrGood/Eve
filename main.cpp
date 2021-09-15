#include <gdiplus.h>
#include "gui.hpp"
#include "analyze.h"
#include "ianalyze.h"

using namespace Gdiplus;

static HRESULT EVEAPI GfxInit(VOID) {
  ULONG_PTR           dwGdiplusToken;
  GdiplusStartupInput GdiplusStartupInput;

  return GdiplusStartup(&dwGdiplusToken, &GdiplusStartupInput, NULL);
}

static BOOL EVEAPI InstanceLock(VOID) {
  #define SZ_MUTEX L"{8A2FDF25-C9E6-49D6-8059-6F3F620AD642}"

  HANDLE hMutex;

  if ((hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, SZ_MUTEX))) {
    CloseHandle(hMutex);

    return FALSE;
  }

  CreateMutexW(NULL, FALSE, SZ_MUTEX);

  #undef SZ_MUTEX

  return TRUE;
}

#ifdef _DEBUG
INT wmain(VOID) {
  PWSTR     szCommandLine = GetCommandLineW();
  HINSTANCE hInstance = GetModuleHandleW(NULL);
#else
INT WINAPI wWinMain(
  IN     HINSTANCE hInstance,
  IN OPT HINSTANCE hPrevInstance,
  IN     PWSTR     szCommandLine,
  IN     INT       nCommandShow
  )
{
#endif
  HRESULT hResult;
  INT     nNumberOfArguments;
  PWSTR * szArguments;

  RtlpInit();

  if (HRESULT_FAILED(hResult = AnalyzeInit())) {
    return hResult;
  }

  if (!InstanceLock()) {
    if ((szArguments = CommandLineToArgvW(szCommandLine, &nNumberOfArguments)) && nNumberOfArguments > 1) {
      for (ULONG_PTR i = 1; i != nNumberOfArguments; i++) {
        IAnalyzeSendArgument(szArguments[i]);
      }

      return ERROR_SUCCESS;
    }
  }

  if (HRESULT_FAILED(hResult = GfxInit())) {
    return hResult;
  }

  IAnalyzeInit();

  if (HRESULT_FAILED(hResult = Gui::Init(hInstance))) {
    return hResult;
  }

  if (HRESULT_FAILED(hResult = Gui::Run())) {
    return hResult;
  }

  return ERROR_SUCCESS;
}

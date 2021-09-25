#include "analyze.h"
#include "gui.hpp"

#define THREADS_MAX         8
#define EXTENSION_BLOB_SIZE 0x1000

typedef struct _EXTENSION_BLOB {
  struct _EXTENSION_BLOB * Next;
  ULONG dwCount;
  PWSTR sz[ANYSIZE_ARRAY];
} EXTENSION_BLOB, * PEXTENSION_BLOB;

BOOL AnalyzeStoppage = FALSE;
BOOL VOLATILE AnalyzeStopped = TRUE;

static PEXTENSION_BLOB ExtensionsBlob = NULL;
static PEXTENSION_BLOB ExtensionsBlobLast = NULL;

static PWSTR ExecutableExtensionsArray[] = {
  L"EXE",
  L"DLL",
  L"SYS"
};

BOOL EVEAPI AnalyzeIsExecutable(
  IN PWSTR szFilePath
  )
{
  PWSTR szExtensionPart = NULL;

  while ((szFilePath = wcschr(szFilePath, L'.'))) {
    szExtensionPart = ++szFilePath;
  }

  if (!szExtensionPart) {
    return FALSE;
  }

  for (ULONG_PTR i = 0; i != ARRAYSIZE(ExecutableExtensionsArray); i++) {
    if (wcslen(ExecutableExtensionsArray[i]) == _wcsicmp(szExtensionPart, ExecutableExtensionsArray[i])) {
      return TRUE;
    }
  }

  return FALSE;
}

static BOOL EVEAPI AnalyzeFileExtension(
  IN PWSTR szFilePath
  )
{
  #ifdef _DEBUG
    PWSTR $szFilePath = szFilePath;
  #endif

  PWSTR szPreLastExtension = NULL;
  PWSTR szLastExtension = NULL;

  while ((szFilePath = wcschr(szFilePath, L'.'))) {
    if (!szLastExtension) {
      szLastExtension = ++szFilePath;
    } else {
      szPreLastExtension = szLastExtension;
      szLastExtension = ++szFilePath;
    }
  }

  if (!szPreLastExtension) {
    return FALSE;
  }

  for (ULONG_PTR i = 0; i != ARRAYSIZE(ExecutableExtensionsArray); i++) {
    if (!_wcsicmp(szLastExtension, ExecutableExtensionsArray[i])) {
      PEXTENSION_BLOB LocalExtensionBlob = ExtensionsBlob;

      while (LocalExtensionBlob) {
        for (ULONG_PTR i = 0; i != LocalExtensionBlob->dwCount; i++) {
          if (!_wcsnicmp(szPreLastExtension, LocalExtensionBlob->sz[i], wcslen(LocalExtensionBlob->sz[i])))) {
          #ifdef _DEBUG
            DebugPrint("found hidden virus using extension exploits, %ls", $szFilePath);
          #endif
          
            return TRUE;
          }
        }

        LocalExtensionBlob = LocalExtensionBlob->Next;
      }
    }
  }

  return FALSE;
}

static ULONG EVEAPI AnalyzeGetImageChecksum(
  IN PVOID     pFileContent,
  IN SIZE_T    stFileContent,
  IN ULONG_PTR dwChecksumOffset
  )
{
  ULONG64 qwChecksum = 0;

  for(ULONG_PTR i = 0; i < stFileContent; i += 4) {
    if (i == dwChecksumOffset) {
      continue;
    }

    qwChecksum = (qwChecksum & 0xFFFFFFFF) + *((PDWORD32)((ULONG_PTR)pFileContent + i)) + (qwChecksum >> 32);

    if (qwChecksum > (ULONG32)-1) {
      qwChecksum = (qwChecksum & 0xFFFFFFFF) + (qwChecksum >> 32);
    }
  }

  qwChecksum = (qwChecksum & 0xFFFF) + (qwChecksum >> 16);
  qwChecksum = (qwChecksum) + (qwChecksum >> 16);
  qwChecksum = qwChecksum & 0xFFFF;
  qwChecksum += stFileContent;

  return (ULONG)qwChecksum;
}

static BOOL EVEAPI AnalyzeFile(
  IN PWSTR szFilePath
  )
{
  BOOL   bIsExecutable = FALSE;
  ULONG  dwFileSizeLow = 0;
  ULONG  dwFileSizeHigh = 0;
  ULONG  dwIo;
  HANDLE hFile;
  PVOID  pFileContent;

  if (!(hFile = CreateFileW(szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL))) {
    return FALSE;
  }

  if (AnalyzeFileExtension(szFilePath)) {
    CloseHandle(hFile);

    return TRUE;
  }

  dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);

  if (!dwFileSizeLow || dwFileSizeHigh) {
    DebugPrint("file to large or emptry, %ls %I64u KB", szFilePath, MAKELONG64(dwFileSizeLow, dwFileSizeHigh) / 1024);

    CloseHandle(hFile);
  
    return FALSE;
  }

  if (!(pFileContent = VirtualAlloc(NULL, (SIZE_T)dwFileSizeLow, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
    DebugPrint("not enough memory to load %ls", szFilePath);

    CloseHandle(hFile);

    return FALSE;
  }

  if (!(ReadFile(hFile, pFileContent, dwFileSizeLow, &dwIo, NULL))) {
    DebugPrint("failed to read from %ls", szFilePath);

    VirtualFree(pFileContent, 0, MEM_RELEASE);
    CloseHandle(hFile);

    return FALSE;
  }

  if (((PIMAGE_DOS_HEADER)pFileContent)->e_magic != IMAGE_DOS_SIGNATURE) {
    DebugPrint("invalid DOS magic, %ls", szFilePath);

    VirtualFree(pFileContent, 0, MEM_RELEASE);
    CloseHandle(hFile);

    return FALSE;
  }

  typedef union {
    struct {
      ULONG             dwSignature;
      IMAGE_FILE_HEADER FileHeader;
    } Shared;

    IMAGE_NT_HEADERS64 Nt64;
    IMAGE_NT_HEADERS32 Nt32;
  } * PIMAGE_NT_HEADER96;

  BOOL                  bIsAmd64;
  ULONG                 dwPeChecksum;
  ULONG_PTR             dwPeChecksumOffset;
  ULONG                 dwNumberOfSections;
  PIMAGE_SECTION_HEADER SectionHeader;
  PIMAGE_NT_HEADER96    NtHeader96 = (PVOID)((ULONG_PTR)pFileContent + ((PIMAGE_DOS_HEADER)pFileContent)->e_lfanew);

  if (NtHeader96->Shared.dwSignature != IMAGE_NT_SIGNATURE) {
    DebugPrint("invalid NT magic, %ls", szFilePath);

    VirtualFree(pFileContent, 0, MEM_RELEASE);
    CloseHandle(hFile);

    return FALSE;
  }

  if (NtHeader96->Shared.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 && NtHeader96->Shared.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
    VirtualFree(pFileContent, 0, MEM_RELEASE);
    CloseHandle(hFile);

    return FALSE;
  }

  dwNumberOfSections = NtHeader96->Shared.FileHeader.NumberOfSections;
  bIsAmd64 = NtHeader96->Shared.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ? TRUE : FALSE;
  SectionHeader = (PVOID)((ULONG_PTR)&NtHeader96->Nt64.OptionalHeader + (ULONG_PTR)NtHeader96->Shared.FileHeader.SizeOfOptionalHeader);

  if ((ULONG_PTR)SectionHeader >= (ULONG_PTR)pFileContent + dwFileSizeLow) {
    DebugPrint("section header address is greater than the image, %ls", szFilePath);

    VirtualFree(pFileContent, 0, MEM_RELEASE);
    CloseHandle(hFile);

    return FALSE;
  }

  if ((ULONG_PTR)SectionHeader + sizeof(IMAGE_SECTION_HEADER) * dwNumberOfSections > (ULONG_PTR)pFileContent + dwFileSizeLow) {
    DebugPrint("section header is larger than the image, %ls", szFilePath);

    VirtualFree(pFileContent, 0, MEM_RELEASE);
    CloseHandle(hFile);

    return FALSE;
  }

  if (bIsAmd64) {
    dwPeChecksum = NtHeader96->Nt64.OptionalHeader.CheckSum;
    dwPeChecksumOffset = ((ULONG_PTR)&NtHeader96->Nt64.OptionalHeader.CheckSum - (ULONG_PTR)pFileContent);
  } else {
    dwPeChecksum = NtHeader96->Nt32.OptionalHeader.CheckSum;
    dwPeChecksumOffset = ((ULONG_PTR)&NtHeader96->Nt32.OptionalHeader.CheckSum - (ULONG_PTR)pFileContent);
  }

  ULONG dwAnalyzeChecksum = AnalyzeGetImageChecksum(pFileContent, (SIZE_T)dwFileSizeLow, dwPeChecksumOffset);

  if (dwPeChecksum && dwPeChecksum != dwAnalyzeChecksum) {
    DebugPrint("found virus by PE-checksum %ls, 0x%08lX != 0x%08lX", szFilePath, dwPeChecksum, dwAnalyzeChecksum);

    VirtualFree(pFileContent, 0, MEM_RELEASE);
    CloseHandle(hFile);

    return TRUE;
  }

  if (dwNumberOfSections > 1 && SectionHeader[0].Characteristics & IMAGE_SCN_MEM_EXECUTE && SectionHeader[dwNumberOfSections - 1].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
    DebugPrint("found appended executable section, %ls", szFilePath);

    VirtualFree(pFileContent, 0, MEM_RELEASE);
    CloseHandle(hFile);

    return TRUE;
  }

  for (ULONG_PTR i = 0; i != dwNumberOfSections; i++) {
    if (!(SectionHeader[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) {
      continue;
    }

    LONG  dwEpilogueSize;
    PVOID pEpilogueSpace;

    if (i + 1 == dwNumberOfSections) {
      dwEpilogueSize = SectionHeader[i + 1].PointerToRawData - (SectionHeader[i].PointerToRawData + SectionHeader[i].Misc.VirtualSize);
    } else {
      dwEpilogueSize = SectionHeader[i].SizeOfRawData - SectionHeader[i].Misc.VirtualSize;
    }
    
    if (dwEpilogueSize <= 0) {
      continue;
    }

    pEpilogueSpace = (PVOID)((ULONG_PTR)pFileContent + SectionHeader[i].PointerToRawData + SectionHeader[i].Misc.VirtualSize);
    
    if ((ULONG_PTR)pEpilogueSpace >= ((ULONG_PTR)pFileContent + dwFileSizeLow) || ((ULONG_PTR)pEpilogueSpace + dwEpilogueSize) > (ULONG_PTR)pFileContent + dwFileSizeLow) {
      DebugPrint("section header address is greater than the image, %ls", szFilePath);

      VirtualFree(pFileContent, 0, MEM_RELEASE);
      CloseHandle(hFile);

      return FALSE;
    }

    for (ULONG_PTR i = 0; i != dwEpilogueSize; i++) {
      #define INT3_OPCODE 0xCC

      if (((PBYTE)pEpilogueSpace)[i] == INT3_OPCODE || !((PBYTE)pEpilogueSpace)[i]) {
        continue;
      }

      DebugPrint("found code cave in %ls", szFilePath);

      #undef INT3_OPCODE

      return TRUE;
    }
  }

  VirtualFree(pFileContent, 0, MEM_RELEASE);
  CloseHandle(hFile);

  return FALSE;
}

ULONG WINAPI AnalyzeFileThread(
  IN PWSTR szFilePath
  )
{
  if (AnalyzeFile(szFilePath)) {
    TastNotificationShow(L"Found Virus");

    DeleteFileW(szFilePath);
  }

  return ERROR_SUCCESS;
}

ULONG WINAPI AnalyzeFolder(
  IN VMEM PWSTR szFolderPath
  )
{
  PWSTR            PathsArray[THREADS_MAX];
  HANDLE           ThreadsArray[THREADS_MAX];
  ULONG            ThreadsArrayCount = 0;
  HANDLE           hFindHandle;
  WIN32_FIND_DATAW FindDataW;
  SIZE_T           dwFolderPathLenght = wcslen(szFolderPath) - 1;

  ZeroMemory(&ThreadsArray, sizeof(ThreadsArray));

  if (INVALID_HANDLE_VALUE != (hFindHandle = FindFirstFileW(szFolderPath, &FindDataW))) {
    do {
      if (AnalyzeStoppage) {
        DebugPrint("analyze stopped");

        break;
      }

      if (FindDataW.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
        CopyMemory(szFolderPath + dwFolderPathLenght, FindDataW.cFileName, (wcslen(FindDataW.cFileName) + 1) * sizeof(WCHAR));

        if (AnalyzeIsExecutable(szFolderPath)) {
          if (ThreadsArrayCount == ARRAYSIZE(ThreadsArray)) {
            WaitForMultipleObjects(ARRAYSIZE(ThreadsArray), ThreadsArray, FALSE, INFINITE);
          }

          for (ULONG_PTR i = 0; i != ARRAYSIZE(ThreadsArray); i++) {
            if (!ThreadsArray[i]) {
              SIZE_T cbFilePath = (wcslen(szFolderPath) + 1) * sizeof(WCHAR);
              PWSTR  szFilePath;

              if ((szFilePath = VirtualAlloc(NULL, cbFilePath, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
                CopyMemory(szFilePath, szFolderPath, cbFilePath);

                PathsArray[i] = szFilePath;

                if ((ThreadsArray[i] = CreateThread(NULL, 0, AnalyzeFileThread, szFilePath, 0, NULL))) {
                  ThreadsArrayCount++;
                } else {
                  VirtualFree(PathsArray[i], 0, MEM_RELEASE);\
                }
              }

              break;
            }
          }

          BOOL bWait = TRUE;

          while (bWait) {
            for (ULONG_PTR i = 0; i != ARRAYSIZE(ThreadsArray); i++) {
              if (!ThreadsArray[i]) {
                bWait = FALSE;
              }
            }

            for (ULONG_PTR i = 0; i != ARRAYSIZE(ThreadsArray); i++) {
              if (ThreadsArray[i]) {
                ULONG dwWaitStatus = WaitForSingleObject(ThreadsArray[i], 0);

                if (dwWaitStatus == WAIT_OBJECT_0 || dwWaitStatus == WAIT_FAILED) {
                  CloseHandle(ThreadsArray[i]);

                  ThreadsArrayCount--;
                  ThreadsArray[i] = NULL;
                  VirtualFree(PathsArray[i], 0, MEM_RELEASE);
                }
              }
            }
          }
        }
      }
    } while (FindNextFileW(hFindHandle, &FindDataW));

    FindClose(hFindHandle);
  }

  VirtualFree(szFolderPath, 0, MEM_RELEASE);

  AnalyzeStopped = TRUE;

  return ERROR_SUCCESS;
}

static VOID EVEAPI AnalyzeFolderStackLoop(
  IN PWSTR szPath
  )
{
  PWSTR            PathsArray[THREADS_MAX];
  HANDLE           ThreadsArray[THREADS_MAX];
  ULONG            ThreadsArrayCount = 0;
  SIZE_T           stPathLenght = wcslen(szPath);
  WIN32_FIND_DATAW FindDataW;
  HANDLE           hFindHandle;
  
  ZeroMemory(&ThreadsArray, sizeof(ThreadsArray));

  if (!(hFindHandle = FindFirstFileW(szPath, &FindDataW))) {
    return;
  }

  do {
    if (AnalyzeStoppage) {
      DebugPrint("analyze stopped");

      break;
    }

    if (*((PULONG32)FindDataW.cFileName) == MAKELONG('.', 0) || *((PULONG32)FindDataW.cFileName) == MAKELONG('.', '.') || FindDataW.cFileName[3] == 0) {
      continue;
    }

    SIZE_T stFileNameLenght = wcslen(FindDataW.cFileName);

    CopyMemory(szPath + stPathLenght - 1, FindDataW.cFileName, (stFileNameLenght + 1) * sizeof(WCHAR));

    if (FindDataW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      CopyMemory(szPath + stPathLenght - 1 + stFileNameLenght, L"\\*", 3 * sizeof(WCHAR));

      AnalyzeFolderStackLoop(szPath);
    }

    if ((FindDataW.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) && AnalyzeIsExecutable(szPath)) {
      for (ULONG_PTR i = 0; i != ARRAYSIZE(ThreadsArray); i++) {
        if (!ThreadsArray[i]) {
          SIZE_T cbFilePath = (stPathLenght + stFileNameLenght) * sizeof(WCHAR);
          PWSTR  szFilePath;

          if ((szFilePath = VirtualAlloc(NULL, cbFilePath, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
            CopyMemory(szFilePath, szPath, cbFilePath);

            PathsArray[i] = szFilePath;

            if ((ThreadsArray[i] = CreateThread(NULL, 0, AnalyzeFileThread, szFilePath, 0, NULL))) {
              ThreadsArrayCount++;
            } else {
              VirtualFree(PathsArray[i], 0, MEM_RELEASE);\
            }
          }

          break;
        }
      }

      BOOL bWait = TRUE;

      while (bWait) {
        for (ULONG_PTR i = 0; i != ARRAYSIZE(ThreadsArray); i++) {
          if (!ThreadsArray[i]) {
            bWait = FALSE;
          }
        }

        for (ULONG_PTR i = 0; i != ARRAYSIZE(ThreadsArray); i++) {
          if (ThreadsArray[i]) {
            ULONG dwWaitStatus = WaitForSingleObject(ThreadsArray[i], 0);

            if (dwWaitStatus == WAIT_OBJECT_0 || dwWaitStatus == WAIT_FAILED) {
              CloseHandle(ThreadsArray[i]);

              ThreadsArrayCount--;
              ThreadsArray[i] = NULL;
              VirtualFree(PathsArray[i], 0, MEM_RELEASE);
            }
          }
        }
      }
    }
  } while (FindNextFileW(hFindHandle, &FindDataW));

  FindClose(hFindHandle);
}

static VOID EVEAPI AnalyzeQuickScanRegistry(
  IN HKEY  hKey,
  IN PWSTR szSubKeyPath
  )
{
  HKEY    lhKey;
  HRESULT hResult;
  WCHAR   szValueName[MAX_PATH];
  WCHAR   szValueData[MAX_PATH];
  ULONG   dwIndex = 0;

  if (HRESULT_FAILED(RegOpenKeyW(hKey, szSubKeyPath, &lhKey))) {
    return;
  }

  do {
    ULONG dwRegType = RRF_RT_REG_SZ;
    DWORD dwValueNameLenght = ARRAYSIZE(szValueName);
    DWORD dwValueDataLenght = ARRAYSIZE(szValueData);

    if (HRESULT_SUCCESS(hResult = RegEnumValueW(lhKey, dwIndex++, szValueName, &dwValueNameLenght, NULL, &dwRegType, (PBYTE)szValueData, &dwValueDataLenght))) {
      AnalyzeFileThread(szValueData);
    }
  } while (hResult != ERROR_NO_MORE_ITEMS);

  RegCloseKey(hKey);
}

static VOID FORCEINLINE EVEAPI AnalyzeQuickScanRegistryWrapper(VOID) {
  AnalyzeQuickScanRegistry(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
  AnalyzeQuickScanRegistry(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
  AnalyzeQuickScanRegistry(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
  AnalyzeQuickScanRegistry(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
}

ULONG WINAPI AnlyzeFullScan(
  IN OPT PWSTR szPath
  )
{
  ULONG dwLogicalDrives;

  AnalyzeQuickScanRegistryWrapper();

  if ((szPath = VirtualAlloc(NULL, LONG_PATH_MAX * sizeof(WCHAR), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
    dwLogicalDrives = GetLogicalDrives();

    for (ULONG_PTR i = 0; i != 32; i++) {
      if (AnalyzeStoppage) {
        break;
      }

      if ((dwLogicalDrives >> i) & 1) {
        ((PULONG32)szPath)[0] = MAKELONG(L'A' + i, L':');
        ((PULONG32)szPath)[1] = MAKELONG(L'\\',    L'*');
        szPath[4] = 0;
      
        AnalyzeFolderStackLoop(szPath);
      }
    }

    VirtualFree(szPath, 0, MEM_RELEASE);
  }

  AnalyzeStopped = TRUE;

  return ERROR_SUCCESS;
}

ULONG WINAPI AnlyzeQuickScan(
  IN PVOID pParameter
  )
{
  UNREFERENCED_PARAMETER(pParameter);

  PWSTR szPath;

  AnalyzeQuickScanRegistryWrapper();

  if ((szPath = VirtualAlloc(NULL, LONG_PATH_MAX * sizeof(WCHAR), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
    SIZE_T dwUserProfileLenght;

    if ((dwUserProfileLenght = GetEnvironmentVariableW(L"USERPROFILE", szPath, LONG_PATH_MAX))) {
      CopyMemory(szPath + dwUserProfileLenght, L"\\*", 3 * sizeof(WCHAR));

      AnalyzeFolderStackLoop(szPath);
    }

    VirtualFree(szPath, 0, MEM_RELEASE);
  }

  AnalyzeStopped = TRUE;

  return ERROR_SUCCESS;
}

HRESULT EVEAPI AnalyzeInit(VOID) {
  HKEY    hKey;
  HRESULT hResult;
  WCHAR   szKeyName[MAX_PATH];
  ULONG   dwIndex = 0;

  if (HRESULT_FAILED(hResult = RegOpenKeyW(HKEY_CLASSES_ROOT, L"", &hKey))) {
    return hResult;
  }

  do {
    if (HRESULT_SUCCESS(hResult = RegEnumKeyW(hKey, dwIndex++, szKeyName, MAX_PATH)) && *szKeyName == L'.') {
      PWSTR           szNewKeyName = szKeyName + 1;
      PWSTR           szName;
      SIZE_T          cbName = (wcslen(szNewKeyName) + 1) * sizeof(WCHAR);
      PEXTENSION_BLOB LocalExtensionBlob;

      if (!(szName = RtlpAlloc(cbName))) {
        return ERROR_NOT_ENOUGH_MEMORY;
      }

      CopyMemory(szName, szNewKeyName, cbName);

      if (ExtensionsBlobLast) {
        if ((ExtensionsBlobLast->dwCount < (EXTENSION_BLOB_SIZE - sizeof(EXTENSION_BLOB)) / sizeof(PWSTR))) {
          ExtensionsBlobLast->sz[ExtensionsBlobLast->dwCount++] = szName;

          continue;
        }
      }
      
      if (!(LocalExtensionBlob = VirtualAlloc(NULL, EXTENSION_BLOB_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))) {
        return ERROR_NOT_ENOUGH_MEMORY;
      }

      LocalExtensionBlob->sz[LocalExtensionBlob->dwCount++] = szName;

      if (ExtensionsBlobLast) {
        ExtensionsBlobLast->Next = LocalExtensionBlob;
        ExtensionsBlobLast = LocalExtensionBlob;
      } else {
        ExtensionsBlob = LocalExtensionBlob;
        ExtensionsBlobLast = LocalExtensionBlob;
      }
    }
  } while (hResult != ERROR_NO_MORE_ITEMS);

  RegCloseKey(hKey);

  return ERROR_SUCCESS;
}

#ifdef _DEBUG
  VOID EVEAPI AnalyzeTest(VOID) {
    PWSTR szFileExtensionTestArray[] = {
      L"test.docx.exe",
      L"test.pdf.dll",
      L"test.3.sys",
      L"test.3.test",
      L"test..test"
    };

    DebugPrint("AnalyzeIsExecutable");

    for (ULONG_PTR i = 0; i != ARRAYSIZE(szFileExtensionTestArray); i++) {
      if (AnalyzeIsExecutable(szFileExtensionTestArray[i])) {
        DebugPrint("  %ls", szFileExtensionTestArray[i]);
      }
    }

    DebugPrint("AnalyzeFileExtension");

    for (ULONG_PTR i = 0; i != ARRAYSIZE(szFileExtensionTestArray); i++) {
      if (AnalyzeFileExtension(szFileExtensionTestArray[i])) {
        DebugPrint("  %ls", szFileExtensionTestArray[i]);
      }
    }

    DebugPrint("%lu", AnalyzeFile(L"C:\\Users\\BadOrGood\\Desktop\\test.exe"));
  }
#endif

#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>


//  Forward declarations:
static BOOL GetProcessIDByName(WCHAR *processToInfect, DWORD *processId);

static void printError(TCHAR* msg);

static BOOL InjectDllToProcess(DWORD processId, char *dllPath);

static BOOL FileExists(LPCTSTR szPath);

static void ConvertCharToWCHAR(char *argv1, char *argv2, WCHAR **wprocessName, WCHAR **wdllPath);


int main(int argc, char *argv[])
{
    WCHAR *wprocess_name, *wdll_path = NULL;
    DWORD process_id;


    if (argc != 3)
    {
        _tprintf(TEXT("ERROR: Usage: ProcessToInject.exe Full_Path_to_DLL\\ABC.DLL"));
        return(FALSE);
    }

    ConvertCharToWCHAR(argv[1], argv[2], &wprocess_name, &wdll_path);

    if (GetProcessIDByName(wprocess_name, &process_id))
    {
        _tprintf(TEXT("\nPROCCESS FOUND \n"));
        free(wprocess_name);
    }
    else
    {
        _tprintf(TEXT("\nERROR: PROCCESS NOT FOUND \n"));
        free(wprocess_name);
        free(wdll_path);
        return(FALSE);
    }

    //CheckDLLPATH()

    if (!FileExists(wdll_path))
    {
        _tprintf(TEXT("\nERROR: DLL NOT FOUND \n"));
        free(wdll_path);
        return(FALSE);
    }
    else
    {
        _tprintf(TEXT("\nDLL FOUND \n"));
    }

    if (!InjectDllToProcess(process_id, argv[2]))
    {
        _tprintf(TEXT("\nFAILED TO INJECT DLL \n"));
        free(wdll_path);
        return(FALSE);
    }

    return(TRUE);
}



static void ConvertCharToWCHAR(char *argv1, char *argv2, WCHAR **wprocessName, WCHAR **wdllPath)
{
    size_t len;


    len = strlen(argv1) + 1;
    *wprocessName = (WCHAR *)malloc(len * sizeof(WCHAR));
    mbstowcs_s(NULL, *wprocessName, len * sizeof(WCHAR), argv1, len * sizeof(WCHAR) - 1);

    len = strlen(argv2) + 1;
    *wdllPath = (WCHAR *)malloc(len * sizeof(WCHAR));
    mbstowcs_s(NULL, *wdllPath, len * sizeof(WCHAR), argv2, len * sizeof(WCHAR) - 1);
}



static BOOL FileExists(LPCTSTR szPath)
{
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}



static BOOL InjectDllToProcess(DWORD processId, char *dllPath)
{
#ifdef _WIN64
    DWORD64 dw_size;
#else
    DWORD   dw_size;
#endif
    HANDLE  h_process, h_remote_thread; 
    HMODULE h_module;
    LPVOID  lp_remote_mem; 
    FARPROC lp_load_library;
    BOOL    ret;


    //Step.1 Attach this Process to the running process
    h_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (h_process == NULL)
    {
        printError(TEXT("OpenProcess()"));
        return(FALSE);
    }

    dw_size = strlen(dllPath) * sizeof(char); // CHECK HACK

    //Step.2 Allocate Memory within the process
    lp_remote_mem = VirtualAllocEx(h_process, NULL, dw_size, MEM_COMMIT, PAGE_READWRITE);
    if (lp_remote_mem == NULL)
    {
        printError(TEXT("VirtualAllocEx()"));
        CloseHandle(h_process);
        return(FALSE);
    }


    //Step.3 Copy the DLL or the DLL Path into the processes memory and determine appropriate memory addresses
    ret = WriteProcessMemory(h_process, lp_remote_mem, dllPath, dw_size, NULL);
    if (ret == 0)
    {
        printError(TEXT("WriteProcessMemory()"));
        CloseHandle(h_process);
        return(FALSE);
    }

    h_module = GetModuleHandleA("kernel32.dll");
    if (h_module == NULL)
    {
        printError(TEXT("GetModuleHandleA()"));
        CloseHandle(h_process);
        return(FALSE);
    }

    lp_load_library = GetProcAddress(h_module, "LoadLibraryA");
    if (lp_load_library == NULL)
    {
        printError(TEXT("GetProcAddress()"));
        CloseHandle(h_process);
        CloseHandle(h_module);
        return(FALSE);
    }


    //Step.4 Instruct the process to Execute your DLL
    h_remote_thread = CreateRemoteThread(h_process, NULL, 0, (LPTHREAD_START_ROUTINE)lp_load_library, lp_remote_mem, 0, NULL);
    if (lp_load_library == NULL)
    {
        printError(TEXT("CreateRemoteThread()"));
        CloseHandle(h_process);
        CloseHandle(h_module);
        return(FALSE);
    }

    return(TRUE);
}



static BOOL GetProcessIDByName(WCHAR *processToInfect, DWORD *processId)
{
    HANDLE h_process_snap;
    PROCESSENTRY32 pe32;


    // Take a snapshot of all processes in the system.
    h_process_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (h_process_snap == INVALID_HANDLE_VALUE)
    {
        printError(TEXT("CreateToolhelp32Snapshot (of processes)"));
        return(FALSE);
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(h_process_snap, &pe32))
    {
        printError(TEXT("Process32First()")); // show cause of failure
        CloseHandle(h_process_snap);          // clean the snapshot object
        return(FALSE);
    }

    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do
    {
        if (wcscmp(processToInfect, pe32.szExeFile) == 0)
        {

            //_tprintf(TEXT("\n  Process ID        = 0x%08X"), pe32.th32ProcessID);
            *processId = pe32.th32ProcessID;
            CloseHandle(h_process_snap);
            return(TRUE);
        }
    } while (Process32Next(h_process_snap, &pe32));

    CloseHandle(h_process_snap);
    return(FALSE);
}



static void printError(TCHAR* msg)
{
    DWORD eNum;
    TCHAR sysMsg[256];
    TCHAR* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, eNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        sysMsg, 256, NULL);

    // Trim the end of the line and terminate it with a null
    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do { *p-- = 0; } while ((p >= sysMsg) &&
        ((*p == '.') || (*p < 33)));

    // Display the message
    _tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}
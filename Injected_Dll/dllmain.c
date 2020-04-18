// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ulReasonForCall,
                       LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);


    switch (ulReasonForCall)
    {
    case DLL_PROCESS_ATTACH:
        {
            while(1)
            {
                MessageBox(NULL, L"HELLO FROM DLL.", L"HELLO", MB_OK);
            }

            break;
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

